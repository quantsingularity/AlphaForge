# Market data

This directory holds daily OHLCV CSV files, one per instrument, named
`SYMBOL.csv`, with the schema:

```
date,open,high,low,close,volume
2022-01-03,177.83,182.88,177.71,182.01,104487900
```

The data is not committed. Populate it with real, split and dividend adjusted
prices from Yahoo Finance before running the platform:

```
pip install -r requirements.txt
python scripts/fetch_data.py --period 3y
```

Each CSV must be sorted or unsorted by date (the engine sorts on load) and may
include a header row (auto-detected).
