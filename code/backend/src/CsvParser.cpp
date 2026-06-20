#include "utils/CsvParser.hpp"

#include "utils/Logger.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <ranges>
#include <sstream>

using namespace std;

namespace alphaforge {

namespace {

[[nodiscard]] string trim(string_view sv) {
    const auto begin = sv.find_first_not_of(" \t\r\n");
    if (begin == string_view::npos) {
        return {};
    }
    const auto end = sv.find_last_not_of(" \t\r\n");
    return string(sv.substr(begin, end - begin + 1));
}

[[nodiscard]] bool looks_like_header(const vector<string>& cols) {
    if (cols.empty()) {
        return false;
    }
    // If the second field does not parse as a number, treat the row as a header.
    if (cols.size() < 2) {
        return true;
    }
    double value{};
    const string& field = cols[1];
    const auto* first = field.data();
    const auto* last  = field.data() + field.size();
    auto [ptr, ec] = from_chars(first, last, value);
    return ec != errc{} || ptr != last;
}

[[nodiscard]] bool parse_double(const string& s, double& out) {
    const auto* first = s.data();
    const auto* last  = s.data() + s.size();
    auto [ptr, ec] = from_chars(first, last, out);
    return ec == errc{} && ptr == last;
}

[[nodiscard]] bool parse_ll(const string& s, long long& out) {
    const auto* first = s.data();
    const auto* last  = s.data() + s.size();
    auto [ptr, ec] = from_chars(first, last, out);
    return ec == errc{} && ptr == last;
}

} // namespace

vector<string> CsvParser::split_line(string_view line) {
    vector<string> fields;
    size_t start = 0;
    while (true) {
        const auto comma = line.find(',', start);
        if (comma == string_view::npos) {
            fields.push_back(trim(line.substr(start)));
            break;
        }
        fields.push_back(trim(line.substr(start, comma - start)));
        start = comma + 1;
    }
    return fields;
}

vector<Bar> CsvParser::parse_file(const filesystem::path& path,
                                       ParseReport& report) {
    vector<Bar> bars;
    ifstream in(path);
    if (!in) {
        report.errors.push_back("cannot open file: " + path.string());
        return bars;
    }

    string line;
    size_t line_no = 0;
    bool header_checked = false;

    while (getline(in, line)) {
        ++line_no;
        if (line.empty()) {
            continue;
        }
        auto cols = split_line(line);

        if (!header_checked) {
            header_checked = true;
            if (looks_like_header(cols)) {
                continue;  // skip header row
            }
        }

        if (cols.size() < 6) {
            ++report.rejected;
            if (report.errors.size() < kMaxReportedErrors) {
                report.errors.push_back("line " + to_string(line_no) +
                                        ": expected 6 columns, got " +
                                        to_string(cols.size()));
            }
            continue;
        }

        Bar bar;
        bar.date = cols[0];
        bool ok = parse_double(cols[1], bar.open) &&
                  parse_double(cols[2], bar.high) &&
                  parse_double(cols[3], bar.low) &&
                  parse_double(cols[4], bar.close) &&
                  parse_ll(cols[5], bar.volume);

        if (!ok || !bar.valid()) {
            ++report.rejected;
            if (report.errors.size() < kMaxReportedErrors) {
                report.errors.push_back("line " + to_string(line_no) +
                                        ": invalid OHLCV record");
            }
            continue;
        }

        bars.push_back(move(bar));
        ++report.accepted;
    }

    ranges::sort(bars, {}, &Bar::date);
    return bars;
}

} // namespace alphaforge
