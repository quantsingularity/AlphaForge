#!/usr/bin/env python3
"""Fetch real daily OHLCV market data from Yahoo Finance via yfinance.

Writes one CSV per ticker into the data directory in the exact schema the
AlphaForge C++ engine expects:

    date,open,high,low,close,volume
    2022-01-03,177.83,182.88,177.71,182.01,104487900

Prices are split and dividend adjusted (auto_adjust=True) so the return series
used by the risk and backtesting modules is continuous across corporate actions.

Requirements:
    pip install -r requirements.txt   (yfinance, pandas)

Network:
    This downloads from Yahoo Finance (query1/query2.finance.yahoo.com). Those
    hosts must be reachable from wherever you run it. In restricted networks add
    them to the egress allowlist.

Examples:
    python scripts/fetch_data.py
    python scripts/fetch_data.py --period 5y --tickers AAPL MSFT SPY
    python scripts/fetch_data.py --start 2018-01-01 --end 2024-01-01
"""

import argparse
import os
import sys

DEFAULT_TICKERS = ["AAPL", "MSFT", "SPY", "NVDA", "JPM", "TLT"]


def fetch_one(ticker, period, start, end):
    import yfinance as yf

    t = yf.Ticker(ticker)
    if start:
        df = t.history(start=start, end=end, interval="1d", auto_adjust=True)
    else:
        df = t.history(period=period, interval="1d", auto_adjust=True)
    return df


def write_csv(df, path):
    cols = ["Open", "High", "Low", "Close", "Volume"]
    missing = [c for c in cols if c not in df.columns]
    if missing:
        raise ValueError(f"missing columns from source: {missing}")

    with open(path, "w", newline="") as f:
        f.write("date,open,high,low,close,volume\n")
        for idx, row in df.iterrows():
            date = idx.strftime("%Y-%m-%d")
            o = float(row["Open"])
            h = float(row["High"])
            low = float(row["Low"])
            c = float(row["Close"])
            v = int(row["Volume"]) if row["Volume"] == row["Volume"] else 0
            # Skip rows with any non-finite price (occasional bad Yahoo rows).
            if any(x != x for x in (o, h, low, c)):
                continue
            f.write(f"{date},{o:.2f},{h:.2f},{low:.2f},{c:.2f},{v}\n")


def main():
    parser = argparse.ArgumentParser(description="Fetch real OHLCV data via yfinance")
    parser.add_argument("--out", default="code/backend/data", help="output directory")
    parser.add_argument(
        "--period",
        default="3y",
        help="lookback period when no start date is given (e.g. 1y, 3y, 5y, max)",
    )
    parser.add_argument(
        "--start", default=None, help="start date YYYY-MM-DD (overrides period)"
    )
    parser.add_argument("--end", default=None, help="end date YYYY-MM-DD")
    parser.add_argument(
        "--tickers", nargs="+", default=DEFAULT_TICKERS, help="ticker symbols"
    )
    args = parser.parse_args()

    try:
        import yfinance  # noqa: F401
    except ImportError:
        print(
            "yfinance is not installed. Run: pip install -r requirements.txt",
            file=sys.stderr,
        )
        return 2

    os.makedirs(args.out, exist_ok=True)

    written = 0
    failed = []
    for ticker in args.tickers:
        try:
            df = fetch_one(ticker, args.period, args.start, args.end)
            if df is None or df.empty:
                print(f"  {ticker}: no data returned", file=sys.stderr)
                failed.append(ticker)
                continue
            path = os.path.join(args.out, f"{ticker}.csv")
            write_csv(df, path)
            print(f"  {ticker}: wrote {len(df)} bars to {path}")
            written += 1
        except Exception as exc:  # network, symbol, or parsing failure
            print(f"  {ticker}: failed ({type(exc).__name__}: {exc})", file=sys.stderr)
            failed.append(ticker)

    print(f"\nDone: {written} symbols written, {len(failed)} failed.")
    if failed:
        print(f"Failed: {', '.join(failed)}", file=sys.stderr)
        if written == 0:
            print(
                "No data was written. If every ticker failed with a host or "
                "connection error, Yahoo Finance is unreachable from this "
                "network. Allow query1.finance.yahoo.com and "
                "query2.finance.yahoo.com, or run this on an unrestricted "
                "network.",
                file=sys.stderr,
            )
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
