import { Link } from "react-router-dom";
import { useAuth } from "../auth/AuthContext";

const FEATURES = [
  {
    glyph: "$",
    title: "Market Data Engine",
    body: "Loads OHLCV history straight from CSV into memory and serves it over a symbol indexed REST API, ready for charting or replay.",
  },
  {
    glyph: "~",
    title: "Analytics",
    body: "Rolling volatility, rolling Sharpe, and weekly and monthly return breakdowns computed on demand for any instrument and window.",
  },
  {
    glyph: "#",
    title: "Portfolio Accounting",
    body: "A position and cash ledger that marks to market on every tick, tracking realized and unrealized profit and loss and exposure.",
  },
  {
    glyph: ">",
    title: "Order Execution",
    body: "Market, limit, and stop orders route through an order manager and fill against the latest close with a transparent fill model.",
  },
  {
    glyph: "!",
    title: "Risk Engine",
    body: "Value at risk, conditional VaR, drawdown, and beta computed per symbol or in parallel across the entire book on a thread pool.",
  },
  {
    glyph: "*",
    title: "Strategy Lab",
    body: "Backtest moving average, momentum, and mean reversion strategies with configurable transaction cost and slippage assumptions.",
  },
];

const STATS = [
  { value: "C++20", label: "Core Engine" },
  { value: "6", label: "Trading Modules" },
  { value: "REST", label: "JSON API Layer" },
  { value: "Multi thread", label: "Risk Computation" },
];

function PreviewWindow() {
  return (
    <div className="preview-window" aria-hidden="true">
      <div className="preview-titlebar">
        <span className="preview-dot" />
        <span className="preview-dot" />
        <span className="preview-dot" />
        <span className="preview-path mono">alphaforge / terminal</span>
      </div>
      <div className="preview-body">
        <div className="preview-metrics">
          <div className="metric">
            <div className="label">Total Value</div>
            <div className="value">$1,042,318</div>
            <div className="sub up">+4.23%</div>
          </div>
          <div className="metric">
            <div className="label">Sharpe Ratio</div>
            <div className="value">1.84</div>
            <div className="sub">confidence 95%</div>
          </div>
          <div className="metric">
            <div className="label">Max Drawdown</div>
            <div className="value down">-8.61%</div>
            <div className="sub">trailing 3y</div>
          </div>
          <div className="metric">
            <div className="label">Gross Exposure</div>
            <div className="value">$612,904</div>
            <div className="sub">14 positions</div>
          </div>
        </div>
        <div className="preview-rows mono">
          <div className="preview-row">
            <span>AAPL</span>
            <span className="dim">184.22</span>
            <span className="up">+1.12%</span>
          </div>
          <div className="preview-row">
            <span>MSFT</span>
            <span className="dim">412.09</span>
            <span className="up">+0.64%</span>
          </div>
          <div className="preview-row">
            <span>SPY</span>
            <span className="dim">548.71</span>
            <span className="down">-0.18%</span>
          </div>
          <div className="preview-row">
            <span>NVDA</span>
            <span className="dim">121.55</span>
            <span className="up">+2.37%</span>
          </div>
        </div>
      </div>
    </div>
  );
}

export function Home() {
  const { user } = useAuth();

  return (
    <div className="landing">
      <header className="land-nav">
        <Link to="/" className="brand">
          <div className="spark">
            <span className="alpha">Alpha</span>
            <span className="forge">Forge</span>
          </div>
          <span className="tag">Quant Terminal</span>
        </Link>
        <nav className="land-links">
          <a href="#platform">Platform</a>
          <a href="#engine">Engine</a>
          <a href="#architecture">Architecture</a>
        </nav>
        <div className="land-actions">
          {user ? (
            <Link to="/terminal" className="btn">
              Open Terminal
            </Link>
          ) : (
            <>
              <Link to="/sign-in" className="btn ghost">
                Sign in
              </Link>
              <Link to="/sign-up" className="btn">
                Get started
              </Link>
            </>
          )}
        </div>
      </header>

      <section className="hero">
        <div className="hero-copy">
          <div className="badge mono">C++20 EXECUTION CORE</div>
          <h1>
            A quant trading terminal built on a real execution engine, not a
            spreadsheet.
          </h1>
          <p>
            AlphaForge pairs a multithreaded C++ market data, portfolio, risk,
            and backtesting engine with a REST API and a terminal grade
            interface. Load your own price history, size positions, run
            strategies, and see the risk numbers update in real time.
          </p>
          <div className="hero-actions">
            {user ? (
              <Link to="/terminal" className="btn">
                Open Terminal
              </Link>
            ) : (
              <>
                <Link to="/sign-up" className="btn">
                  Create free account
                </Link>
                <Link to="/sign-in" className="btn ghost">
                  Sign in
                </Link>
              </>
            )}
          </div>
          <div className="hero-note faint mono">
            No credit card required. Bring your own market data.
          </div>
        </div>
        <div className="hero-visual">
          <PreviewWindow />
        </div>
      </section>

      <section className="stats-strip">
        {STATS.map((s) => (
          <div className="stat" key={s.label}>
            <div className="stat-value mono">{s.value}</div>
            <div className="stat-label">{s.label}</div>
          </div>
        ))}
      </section>

      <section className="land-section" id="platform">
        <div className="section-head">
          <div className="eyebrow">Platform</div>
          <h2>Six engines, one terminal.</h2>
          <p>
            Every panel in the terminal is backed by an actual engine component
            running in the C++ process, not a mock. Switch tabs and the same
            data flows through the same execution and risk logic.
          </p>
        </div>
        <div className="feature-grid">
          {FEATURES.map((f) => (
            <div className="feature-card" key={f.title}>
              <div className="feature-glyph mono">{f.glyph}</div>
              <h3>{f.title}</h3>
              <p>{f.body}</p>
            </div>
          ))}
        </div>
      </section>

      <section className="land-section" id="engine">
        <div className="section-head">
          <div className="eyebrow">Engine</div>
          <h2>Built for correctness under load.</h2>
        </div>
        <div className="grid cols-3 engine-grid">
          <div className="card">
            <div className="card-title">Thread Pool</div>
            <p className="dim">
              Risk metrics across the full instrument book are computed in
              parallel on a dedicated thread pool, so a portfolio of many
              symbols does not slow the terminal down.
            </p>
          </div>
          <div className="card">
            <div className="card-title">Trade Safety</div>
            <p className="dim">
              Portfolio mutations and order execution are serialized behind a
              single trade mutex, so concurrent requests can never leave the
              ledger in a half updated state.
            </p>
          </div>
          <div className="card">
            <div className="card-title">Cost Modeling</div>
            <p className="dim">
              The backtester applies configurable transaction cost and slippage
              in basis points, so strategy results reflect conditions closer to
              live trading.
            </p>
          </div>
        </div>
      </section>

      <section className="land-section" id="architecture">
        <div className="section-head">
          <div className="eyebrow">Architecture</div>
          <h2>From CSV to terminal, in three layers.</h2>
        </div>
        <div className="arch-steps">
          <div className="arch-step">
            <div className="arch-index mono">01</div>
            <h3>Engine core</h3>
            <p className="dim">
              Market data, portfolio, execution, risk, analytics, and
              backtesting logic live in a single C++20 static library shared by
              the CLI, the server, and the test suite.
            </p>
          </div>
          <div className="arch-step">
            <div className="arch-index mono">02</div>
            <h3>REST API</h3>
            <p className="dim">
              A lightweight HTTP layer exposes the engine as JSON endpoints for
              health, symbols, market data, portfolio, orders, risk, analytics,
              backtests, and account authentication.
            </p>
          </div>
          <div className="arch-step">
            <div className="arch-index mono">03</div>
            <h3>Terminal UI</h3>
            <p className="dim">
              A React and TypeScript frontend renders the same JSON as a live,
              tabbed trading terminal, with the production build served directly
              by the C++ backend.
            </p>
          </div>
        </div>
      </section>

      <section className="cta-banner">
        <div>
          <h2>Start forging your edge.</h2>
          <p className="dim">
            Create an account, point the engine at your own price history, and
            open the terminal.
          </p>
        </div>
        {user ? (
          <Link to="/terminal" className="btn">
            Open Terminal
          </Link>
        ) : (
          <Link to="/sign-up" className="btn">
            Create free account
          </Link>
        )}
      </section>

      <footer className="land-footer">
        <div className="brand">
          <div className="spark">
            <span className="alpha">Alpha</span>
            <span className="forge">Forge</span>
          </div>
        </div>
        <div className="footer-links">
          <Link to="/sign-in">Sign in</Link>
          <Link to="/sign-up">Create account</Link>
          <a
            href="https://github.com/quantsingularity/AlphaForge"
            target="_blank"
            rel="noreferrer"
          >
            Source
          </a>
        </div>
        <div className="faint mono footer-note">
          Research tooling. Not investment advice.
        </div>
      </footer>
    </div>
  );
}
