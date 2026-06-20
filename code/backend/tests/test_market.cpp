#include "TestFramework.hpp"

#include "market/MarketDataEngine.hpp"
#include "utils/CsvParser.hpp"
#include "utils/Statistics.hpp"

#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

using namespace std;

using namespace alphaforge;

TEST_CASE("stats.mean_and_stdev") {
    vector<double> xs{2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    const auto m = stats::mean(span<const double>{xs});
    CHECK(m.has_value());
    CHECK_NEAR(*m, 5.0, 1e-9);
    // Sample stdev of this classic set is sqrt(32/7).
    const auto sd = stats::stdev(span<const double>{xs});
    CHECK(sd.has_value());
    CHECK_NEAR(*sd, sqrt(32.0 / 7.0), 1e-9);
}

TEST_CASE("stats.variance_undefined_for_single") {
    vector<double> one{1.0};
    CHECK(!stats::variance(span<const double>{one}).has_value());
}

TEST_CASE("stats.simple_returns") {
    vector<double> px{100.0, 110.0, 99.0};
    const auto r = stats::simple_returns(span<const double>{px});
    CHECK(r.size() == 2);
    CHECK_NEAR(r[0], 0.10, 1e-12);
    CHECK_NEAR(r[1], -0.10, 1e-12);
}

TEST_CASE("stats.percentile_interpolation") {
    vector<double> xs{1.0, 2.0, 3.0, 4.0};
    const auto p = stats::percentile(span<const double>{xs}, 0.5);
    CHECK(p.has_value());
    CHECK_NEAR(*p, 2.5, 1e-9);
}

TEST_CASE("stats.covariance_and_correlation") {
    vector<double> a{1.0, 2.0, 3.0, 4.0};
    vector<double> b{2.0, 4.0, 6.0, 8.0};
    const auto corr = stats::correlation(span<const double>{a},
                                         span<const double>{b});
    CHECK(corr.has_value());
    CHECK_NEAR(*corr, 1.0, 1e-9);
}

TEST_CASE("csv.split_and_trim") {
    auto fields = CsvParser::split_line(" 2023-01-03 , 10 , 11 ,9, 10 , 1000 ");
    CHECK(fields.size() == 6);
    CHECK(fields[0] == "2023-01-03");
    CHECK(fields[5] == "1000");
}

TEST_CASE("csv.parses_valid_rejects_invalid") {
    const auto path = filesystem::temp_directory_path() / "af_test_data.csv";
    {
        ofstream out(path);
        out << "date,open,high,low,close,volume\n";
        out << "2023-01-03,100,105,99,104,1000\n";
        out << "2023-01-04,104,106,103,abc,1200\n";   // bad close
        out << "2023-01-05,104,103,105,104,1200\n";    // high<low invalid
        out << "2023-01-06,104,108,103,107,1500\n";
    }
    ParseReport report;
    auto bars = CsvParser::parse_file(path, report);
    CHECK(report.accepted == 2);
    CHECK(report.rejected == 2);
    CHECK(bars.size() == 2);
    CHECK(bars.front().date == "2023-01-03");
    filesystem::remove(path);
}

TEST_CASE("market.engine_load_and_query") {
    const auto path = filesystem::temp_directory_path() / "af_eng.csv";
    {
        ofstream out(path);
        out << "date,open,high,low,close,volume\n";
        out << "2023-01-05,10,11,9,10.5,100\n";
        out << "2023-01-03,9,10,8,9.5,100\n";   // out of order on purpose
        out << "2023-01-04,9.5,10.5,9,10,100\n";
    }
    MarketDataEngine engine;
    auto report = engine.load_symbol("TEST", path);
    CHECK(report.accepted == 3);
    CHECK(engine.has_symbol("TEST"));
    CHECK(engine.bar_count("TEST") == 3);
    // Sorted ascending after load.
    auto hist = engine.history("TEST");
    CHECK(hist.front().date == "2023-01-03");
    CHECK(hist.back().date == "2023-01-05");
    auto latest = engine.latest_bar("TEST");
    CHECK(latest.has_value());
    CHECK_NEAR(latest->close, 10.5, 1e-9);
    auto mid = engine.bar_on("TEST", "2023-01-04");
    CHECK(mid.has_value());
    CHECK_NEAR(mid->close, 10.0, 1e-9);
    CHECK(!engine.bar_on("TEST", "2023-02-01").has_value());
    filesystem::remove(path);
}
