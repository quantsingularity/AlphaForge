#pragma once

#include "market/Bar.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using namespace std;

namespace alphaforge {

// Parses OHLCV CSV files of the form:
//   date,open,high,low,close,volume
//   2023-01-03,128.41,129.95,127.43,129.62,12100000
//
// Header row is auto detected and skipped. Rows that fail validation are
// reported through the returned ParseReport rather than throwing, so a single
// malformed line does not abort a large file load.
struct ParseReport {
    size_t accepted{0};
    size_t rejected{0};
    vector<string> errors;  // human readable, capped to avoid blowup
};

class CsvParser {
public:
    // Returns parsed bars sorted ascending by date. The report is filled with
    // counts and a bounded list of row level errors.
    [[nodiscard]] static vector<Bar> parse_file(
        const filesystem::path& path, ParseReport& report);

    // Split a CSV line on commas, trimming surrounding whitespace. Exposed for
    // unit testing.
    [[nodiscard]] static vector<string> split_line(string_view line);

private:
    static constexpr size_t kMaxReportedErrors = 25;
};

} // namespace alphaforge
