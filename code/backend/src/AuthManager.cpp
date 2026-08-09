#include "auth/AuthManager.hpp"

#include "utils/Logger.hpp"
#include "utils/Sha256.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <format>
#include <random>

using namespace std;
using nlohmann::json;

namespace alphaforge {

namespace {

const unsigned kHashRounds = 20000;

string now_iso8601() {
    const auto now = chrono::floor<chrono::seconds>(chrono::system_clock::now());
    return format("{:%Y-%m-%dT%H:%M:%SZ}", now);
}

string trim(const string& s) {
    const auto first = s.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    const auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
}

bool looks_like_email(const string& email) {
    const auto at = email.find('@');
    if (at == string::npos || at == 0 || at == email.size() - 1) return false;
    const auto dot = email.find('.', at);
    if (dot == string::npos || dot == email.size() - 1) return false;
    return email.find(' ') == string::npos;
}

// Constant time-ish comparison so a login attempt cannot learn how many
// leading characters of the stored hash it matched via response timing.
bool secure_equal(const string& a, const string& b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    }
    return diff == 0;
}

}  // namespace

AuthManager::AuthManager(string storage_path) : storage_path_(move(storage_path)) {
    load();
}

string AuthManager::normalize_email(const string& email) {
    string out = trim(email);
    ranges::transform(out, out.begin(), [](unsigned char c) { return tolower(c); });
    return out;
}

string AuthManager::random_hex(size_t bytes) {
    static random_device rd;
    static mt19937_64 gen(rd());
    uniform_int_distribution<int> dist(0, 255);

    static const char* kHex = "0123456789abcdef";
    string out;
    out.reserve(bytes * 2);
    for (size_t i = 0; i < bytes; ++i) {
        const int byte = dist(gen);
        out.push_back(kHex[(byte >> 4) & 0x0f]);
        out.push_back(kHex[byte & 0x0f]);
    }
    return out;
}

string AuthManager::hash_password(const string& password, const string& salt) {
    string h = sha256_hex(salt + ":" + password);
    for (unsigned i = 0; i < kHashRounds; ++i) {
        h = sha256_hex(h + salt);
    }
    return h;
}

AuthUser AuthManager::to_public(const StoredUser& u) {
    return AuthUser{u.id, u.name, u.display_email, u.created_at};
}

string AuthManager::issue_token(const string& user_id) {
    string token = random_hex(32);
    tokens_[token] = user_id;
    return token;
}

AuthResult AuthManager::register_user(const string& name, const string& email,
                                       const string& password) {
    const string clean_name = trim(name);
    const string clean_email = trim(email);
    const string normalized = normalize_email(clean_email);

    AuthResult result;
    if (clean_name.size() < 2) {
        result.error = "name must be at least 2 characters";
        return result;
    }
    if (!looks_like_email(normalized)) {
        result.error = "enter a valid email address";
        return result;
    }
    if (password.size() < 8) {
        result.error = "password must be at least 8 characters";
        return result;
    }

    scoped_lock lock(mutex_);
    if (users_by_email_.contains(normalized)) {
        result.error = "an account with that email already exists";
        return result;
    }

    StoredUser user;
    user.id = to_string(next_id_++);
    user.name = clean_name;
    user.email = normalized;
    user.display_email = clean_email;
    user.salt = random_hex(16);
    user.password_hash = hash_password(password, user.salt);
    user.created_at = now_iso8601();

    users_by_email_[normalized] = user;
    save();

    result.ok = true;
    result.user = to_public(user);
    result.token = issue_token(user.id);
    Logger::instance().info("registered account " + user.email);
    return result;
}

AuthResult AuthManager::login(const string& email, const string& password) {
    const string normalized = normalize_email(email);

    AuthResult result;
    scoped_lock lock(mutex_);
    const auto it = users_by_email_.find(normalized);
    if (it == users_by_email_.end()) {
        result.error = "invalid email or password";
        return result;
    }
    const StoredUser& user = it->second;
    const string candidate = hash_password(password, user.salt);
    if (!secure_equal(candidate, user.password_hash)) {
        result.error = "invalid email or password";
        return result;
    }

    result.ok = true;
    result.user = to_public(user);
    result.token = issue_token(user.id);
    return result;
}

bool AuthManager::logout(const string& token) {
    scoped_lock lock(mutex_);
    return tokens_.erase(token) > 0;
}

optional<AuthUser> AuthManager::user_for_token(const string& token) const {
    if (token.empty()) return nullopt;
    scoped_lock lock(mutex_);
    const auto tok_it = tokens_.find(token);
    if (tok_it == tokens_.end()) return nullopt;
    const string& user_id = tok_it->second;
    for (const auto& [_, user] : users_by_email_) {
        if (user.id == user_id) return to_public(user);
    }
    return nullopt;
}

size_t AuthManager::user_count() const {
    scoped_lock lock(mutex_);
    return users_by_email_.size();
}

void AuthManager::load() {
    scoped_lock lock(mutex_);
    ifstream in(storage_path_);
    if (!in) return;

    json doc;
    try {
        in >> doc;
    } catch (...) {
        Logger::instance().warn("failed to parse " + storage_path_ + ", starting fresh");
        return;
    }
    if (!doc.is_array()) return;

    for (const auto& j : doc) {
        StoredUser user;
        user.id = j.value("id", "");
        user.name = j.value("name", "");
        user.email = j.value("email", "");
        user.display_email = j.value("display_email", user.email);
        user.salt = j.value("salt", "");
        user.password_hash = j.value("password_hash", "");
        user.created_at = j.value("created_at", "");
        if (user.id.empty() || user.email.empty()) continue;

        try {
            next_id_ = max(next_id_, stoul(user.id) + 1);
        } catch (...) {
        }
        users_by_email_[user.email] = move(user);
    }
    Logger::instance().info("loaded " + to_string(users_by_email_.size()) +
                            " account(s) from " + storage_path_);
}

void AuthManager::save() const {
    json arr = json::array();
    for (const auto& [_, user] : users_by_email_) {
        arr.push_back(json{{"id", user.id},
                           {"name", user.name},
                           {"email", user.email},
                           {"display_email", user.display_email},
                           {"salt", user.salt},
                           {"password_hash", user.password_hash},
                           {"created_at", user.created_at}});
    }

    filesystem::path path{storage_path_};
    if (path.has_parent_path()) {
        error_code ec;
        filesystem::create_directories(path.parent_path(), ec);
    }
    ofstream out(storage_path_, ios::trunc);
    if (!out) {
        Logger::instance().error("failed to persist accounts to " + storage_path_);
        return;
    }
    out << arr.dump(2);
}

} // namespace alphaforge
