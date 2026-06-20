# AlphaForge

A production-grade quantitative trading and research platform. The core engine is
modern C++20, exposed through both an interactive terminal CLI and a JSON REST API.
That API drives a modern React and TypeScript web terminal. Market data, portfolio
accounting, order execution, risk, analytics and backtesting are all implemented
from first principles in C++.

AlphaForge is built as a portfolio piece for quantitative engineering roles. The
emphasis is on correct, well-tested financial logic and a clean architecture rather
than on breadth of half-finished features. Every simplifying assumption is stated
plainly in the Limitations table rather than hidden.

|                 |                                                                 |
| --------------- | --------------------------------------------------------------- |
| **Language**    | C++20 (engine), TypeScript + React (web)                        |
| **Interfaces**  | Interactive CLI, REST API, web terminal                         |
| **Modules**     | Market Data, Portfolio, Execution, Risk, Analytics, Backtesting |
| **Strategies**  | BuyAndHold, MovingAverageCrossover, Momentum, MeanReversion     |
| **Data source** | Real OHLCV from Yahoo Finance via yfinance                      |
| **Tests**       | 22 unit tests, all passing via CTest                            |
| **Build**       | CMake (backend), Vite (frontend), Docker (full stack)           |

## Capabilities

| Module      | What it does                                                                                                                 |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------- |
| Market Data | Loads and validates daily OHLCV from CSV with bounded error reporting; sorts and caches series; notifies observers           |
| Portfolio   | Average-cost position accounting, realized and unrealized PnL, gross and net exposure, position weights, deterministic undo  |
| Execution   | Order management with market, limit and stop types, a marketable fill model, and a reversible trade history                  |
| Risk        | Annualized volatility, Sharpe, Sortino, maximum drawdown, historical VaR and CVaR, and beta against a benchmark              |
| Analytics   | Calendar-aware weekly and monthly returns, CAGR, rolling volatility and rolling Sharpe                                       |
| Backtesting | Event-driven simulation with transaction costs and slippage, four strategies, each measured against a buy-and-hold benchmark |
| Interfaces  | All of the above over a REST API and a live web terminal with charts                                                         |

## Architecture

```
                      +---------------------------+
                      |  React + TypeScript UI     |
                      |  (Vite, Recharts)          |
                      +-------------+-------------+
                                    | fetch /api/*  (JSON over HTTP)
                                    v
                      +---------------------------+
                      |  REST API server           |
                      |  (cpp-httplib, nlohmann)   |
                      +-------------+-------------+
                                    | direct calls
                                    v
   +------------------------------------------------------------------+
   |                      AlphaForge engine (C++20)                    |
   |                                                                   |
   |  MarketData   Portfolio   Execution   Risk   Analytics   Backtest |
   |  Utilities: Statistics, ThreadPool, CsvParser, Logger, Timer      |
   +------------------------------------------------------------------+
                                    ^
                                    | same engine, different front end
                      +---------------------------+
                      |  Interactive CLI terminal  |
                      +---------------------------+
```

The engine compiles once into a static library. The CLI, the REST server, the test
runner and the benchmark harness all link against that single library, so there is
exactly one implementation of every calculation.

## Project Structure

```
AlphaForge/
├── code/
│   └── backend/                # C++20 engine, CLI, REST server, tests
│       ├── include/            # Public headers, grouped by module
│       │   ├── common/         # Types, concepts
│       │   ├── core/           # Application facade, Config singleton
│       │   ├── market/         # Bars, market data engine, observer interfaces
│       │   ├── portfolio/      # Positions, portfolio accounting
│       │   ├── execution/      # Orders, order manager
│       │   ├── risk/           # Risk engine (vol, Sharpe, drawdown, VaR/CVaR, beta)
│       │   ├── analytics/      # Returns, rolling volatility and Sharpe, CAGR
│       │   ├── backtesting/    # Strategies, factory, event-driven backtester
│       │   └── utils/          # Statistics, ThreadPool, CsvParser, Logger, Timer
│       ├── src/                # Engine implementation (compiled into a static lib)
│       ├── app/                # Entry points: CLI, REST server, JSON support
│       ├── tests/              # Unit tests and a tiny test framework
│       ├── benchmarks/         # Micro benchmark harness
│       ├── data/               # Market data CSVs (fetched via yfinance, not committed)
│       ├── third_party/        # Vendored cpp-httplib and nlohmann/json
│       └── CMakeLists.txt
├── frontend/                   # React + TypeScript + Vite web terminal
├── scripts/                    # Build, run, and data-fetch helpers
├── infrastructure/             # Dockerfile and docker-compose
├── .github/workflows/          # Continuous integration
└── README.md
```

## Design patterns

Each pattern is used where it earns its place, not for decoration.

| Pattern    | Where                                             | Why                                                                                                  |
| ---------- | ------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| Factory    | `StrategyFactory`                                 | Build strategies from an enum or name so the CLI, API and tests construct them identically           |
| Strategy   | `Strategy` interface and its four implementations | Swap trading logic behind one interface the backtester depends on                                    |
| Observer   | `MarketDataEngine` to `IMarketDataObserver`       | `Portfolio` tracks latest marks; subscribers held as `weak_ptr` so a dead observer is never notified |
| Repository | `IMarketDataRepository`                           | Abstract read access to bar history behind a stable query interface                                  |
| Singleton  | `Config`                                          | Hold run-wide parameters behind a mutex-guarded accessor                                             |
| Facade     | `Application`                                     | Wire the engines together so both front ends share one setup                                         |

## Modern C++ used

| Feature                               | Where it appears                                                                              |
| ------------------------------------- | --------------------------------------------------------------------------------------------- |
| Concepts                              | Constrain the statistics routines to floating-point ranges                                    |
| `std::span`                           | Non-owning views passed throughout the engine                                                 |
| `std::optional`                       | "May not exist" results such as the mean of an empty range or a missing bar                   |
| `std::ranges` + projections           | Sort and search bars by date without a custom comparator                                      |
| Smart pointers                        | `shared_ptr` for shared engines, `unique_ptr` for the order manager, `weak_ptr` for observers |
| `ThreadPool` + `std::future`          | Cross-sectional risk batch computed in parallel                                               |
| `std::format`, `std::source_location` | Back the thread-safe logger                                                                   |

## Strategies

| Strategy               | Logic                                                                         | Default parameters                 |
| ---------------------- | ----------------------------------------------------------------------------- | ---------------------------------- |
| BuyAndHold             | Always fully invested; the benchmark every other strategy is measured against | none                               |
| MovingAverageCrossover | Long when the fast SMA is above the slow SMA, flat otherwise                  | fast 20, slow 50                   |
| Momentum               | Long when the trailing return over the lookback is positive, flat otherwise   | lookback 63                        |
| MeanReversion          | Long when price is sufficiently below its moving average, flat on reversion   | window 20, entry z 1.0, exit z 0.0 |

Signals at bar `i` are held into bar `i+1`, so there is no look-ahead bias.

## Prerequisites

| Tool          | Version              | Purpose                         |
| ------------- | -------------------- | ------------------------------- |
| C++ compiler  | GCC 13+ or Clang 16+ | Build the engine (full C++20)   |
| CMake         | 3.16+                | Configure and build the backend |
| Node.js + npm | 18+                  | Build the web terminal          |
| Python + pip  | 3.9+                 | Fetch market data with yfinance |

## Market data (do this first)

The platform runs on real daily OHLCV pulled from Yahoo Finance with yfinance.
Prices are split and dividend adjusted so return series are continuous across
corporate actions. No data ships in the repository; fetch it before the first run.

```
pip install -r requirements.txt
python scripts/fetch_data.py --period 3y
```

| Flag                | Default                    | Meaning                                                       |
| ------------------- | -------------------------- | ------------------------------------------------------------- |
| `--tickers`         | AAPL MSFT SPY NVDA JPM TLT | Symbols to download                                           |
| `--period`          | `3y`                       | Lookback when no start date is given (`1y`, `5y`, `max`, ...) |
| `--start` / `--end` | none                       | Explicit date range `YYYY-MM-DD` (overrides period)           |
| `--out`             | `code/backend/data`        | Output directory                                              |

The fetch needs network access to `query1.finance.yahoo.com` and
`query2.finance.yahoo.com`. On a restricted network those hosts must be on the
egress allowlist. The C++ engine is source-agnostic: any CSV with a
`date,open,high,low,close,volume` header loads, so another provider can be
substituted by writing files in the same format.

## Building

### Backend

```
cd code/backend
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

This produces four binaries in `code/backend/build`:

| Binary              | Purpose                        |
| ------------------- | ------------------------------ |
| `alphaforge_cli`    | Interactive terminal dashboard |
| `alphaforge_server` | REST API and static web host   |
| `alphaforge_tests`  | Unit test runner               |
| `alphaforge_bench`  | Benchmark harness              |

Run the tests with `ctest --output-on-failure` from the build directory, or run
`./alphaforge_tests` directly.

### Frontend

```
cd frontend
npm install
npm run build        # outputs to frontend/dist
```

### Run everything

| Goal                 | Command                                                  | Result                                                                                                          |
| -------------------- | -------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| Full stack           | `scripts/run_stack.sh`                                   | Builds both halves if needed, fetches data when missing, serves API and web terminal at `http://localhost:8080` |
| Backend only + tests | `scripts/build_and_test.sh`                              | Builds the engine and runs CTest                                                                                |
| Frontend dev server  | `cd frontend && npm run dev`                             | Vite on port 5173, proxying `/api` to the backend                                                               |
| Docker               | `docker compose -f infrastructure/docker-compose.yml up` | Builds and serves the whole stack in a container                                                                |

For frontend hot reload, run the API and the Vite dev server side by side:

```
code/backend/build/alphaforge_server 8080 code/backend/data &
cd frontend && npm run dev
```

## REST API

All responses are JSON. Errors return an `{ "error": "..." }` body with an
appropriate status code.

| Method | Path                                        | Description                                            |
| ------ | ------------------------------------------- | ------------------------------------------------------ |
| GET    | `/api/health`                               | Status, version, symbols loaded, thread count          |
| GET    | `/api/strategies`                           | Available strategy names                               |
| GET    | `/api/symbols`                              | Loaded symbols and bar counts                          |
| GET    | `/api/market/{symbol}?limit=N`              | Recent bars for a symbol                               |
| GET    | `/api/portfolio`                            | Full portfolio snapshot and blotter                    |
| POST   | `/api/order`                                | Submit an order, filled against the latest close       |
| POST   | `/api/undo`                                 | Reverse the most recent trade                          |
| GET    | `/api/risk/{symbol}?confidence=&benchmark=` | Risk metrics for one symbol                            |
| GET    | `/api/risk?confidence=`                     | Risk for all symbols, computed in parallel             |
| GET    | `/api/analytics/{symbol}?window=N`          | Returns and rolling risk                               |
| POST   | `/api/backtest`                             | Run a strategy and return the equity curve and metrics |

### Request bodies

| Endpoint        | Field                        | Example                         | Notes                   |
| --------------- | ---------------------------- | ------------------------------- | ----------------------- |
| `/api/order`    | `symbol`                     | `"AAPL"`                        | Must be loaded          |
|                 | `side`                       | `"buy"` or `"sell"`             |                         |
|                 | `type`                       | `"market"`, `"limit"`, `"stop"` | Limit/stop need a price |
|                 | `quantity`                   | `100`                           | Positive                |
|                 | `limit_price` / `stop_price` | `172.50`                        | Optional, by type       |
| `/api/backtest` | `symbol`                     | `"NVDA"`                        | Must be loaded          |
|                 | `strategy`                   | `"MovingAverageCrossover"`      | One of the four         |
|                 | `transaction_cost_bps`       | `5`                             | Optional                |
|                 | `slippage_bps`               | `2`                             | Optional                |

## Limitations and simplifications

| Area            | Simplification                                                                                                                                                    |
| --------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Fills           | Marketable model against a single reference price (latest close); no intrabar matching, partial fills or live order book                                          |
| Dates           | ISO `YYYY-MM-DD` strings sorted lexicographically; correct for daily data but not a full trading calendar                                                         |
| VaR / CVaR      | Historical (empirical quantile), not parametric or Monte Carlo                                                                                                    |
| `win_rate`      | Fraction of positive-return days among days the strategy held a nonzero position                                                                                  |
| `profit_factor` | Gross gains over gross losses, capped at 99.99 when there are no losing periods                                                                                   |
| Parallelism     | Risk batch uses a thread pool; on a single-core machine there is no wall-clock speedup, and the benchmark reports hardware concurrency so numbers are not misread |
| Data            | Must be fetched before the first run; the repository ships no data so results always come from real, freshly pulled prices                                        |

## Testing and verification

| Area                | What is covered                                                                                     |
| ------------------- | --------------------------------------------------------------------------------------------------- |
| Statistics          | Mean, variance, stdev, returns, percentile, covariance, correlation                                 |
| CSV parsing         | Field splitting and trimming, header detection, rejection of malformed rows                         |
| Market data         | Load, sort, query latest and by-date lookups                                                        |
| Position accounting | Average cost on add, realized PnL on reduce, long-to-short flip                                     |
| Portfolio           | Cash and valuation, undo restores exact prior state                                                 |
| Order manager       | Marketable fills, rejection of non-marketable limits, undo                                          |
| Risk                | Drawdown, VaR vs CVaR ordering, beta of a scaled series, volatility annualization                   |
| Backtesting         | Zero-cost buy-and-hold equals the price return; costs reduce returns; factory builds every strategy |

The REST API was exercised end to end against real data, and the built frontend is
served by the same C++ server that answers the API.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
