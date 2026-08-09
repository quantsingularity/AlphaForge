#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

using namespace std;

namespace alphaforge {

// Public facing user record. Never carries the password hash or salt so it
// is safe to serialise straight into an API response.
struct AuthUser {
    string id;
    string name;
    string email;
    string created_at;
};

// Outcome of a register or login attempt. `ok` is false when `error` should
// be shown to the caller instead of `user`/`token`.
struct AuthResult {
    bool ok{false};
    string error;
    AuthUser user;
    string token;
};

// Minimal, dependency free authentication layer for the REST API.
//
// Accounts are hashed with a salted, iterated SHA-256 (utils/Sha256.hpp) and
// mirrored to a JSON file on disk so they survive a server restart. Session
// tokens are bearer strings handed back on register/login and looked up on
// every authenticated request; they are intentionally kept in memory only,
// so restarting the server signs everyone out, matching the disposable,
// single process nature of the rest of the engine.
class AuthManager {
public:
    explicit AuthManager(string storage_path);

    AuthResult register_user(const string& name, const string& email,
                              const string& password);
    AuthResult login(const string& email, const string& password);
    bool logout(const string& token);

    // Resolves a bearer token to the user it belongs to, if the session is
    // still valid.
    [[nodiscard]] optional<AuthUser> user_for_token(const string& token) const;

    [[nodiscard]] size_t user_count() const;

private:
    struct StoredUser {
        string id;
        string name;
        string email;          // normalised (lowercased, trimmed) lookup key
        string display_email;  // original casing, shown back to the user
        string salt;
        string password_hash;
        string created_at;
    };

    void load();
    void save() const;

    [[nodiscard]] static string normalize_email(const string& email);
    [[nodiscard]] static AuthUser to_public(const StoredUser& u);
    [[nodiscard]] static string random_hex(size_t bytes);
    [[nodiscard]] static string hash_password(const string& password, const string& salt);

    [[nodiscard]] string issue_token(const string& user_id);

    mutable mutex mutex_;
    string storage_path_;
    unordered_map<string, StoredUser> users_by_email_;  // key: normalized email
    unordered_map<string, string> tokens_;               // token -> user id
    unsigned long next_id_{1};
};

} // namespace alphaforge
