import { useEffect, useMemo, useState } from "react";
import { Link } from "react-router-dom";
import { api } from "../api";
import { useAuth } from "../auth/AuthContext";
import { num, signedPct, cls } from "../format";
import { MarketPanel } from "../panels/MarketPanel";
import { PortfolioPanel } from "../panels/PortfolioPanel";
import { OrderPanel } from "../panels/OrderPanel";
import { BacktestPanel } from "../panels/BacktestPanel";
import { RiskPanel } from "../panels/RiskPanel";
import { AnalyticsPanel } from "../panels/AnalyticsPanel";
import type { Health, SymbolInfo } from "../types";

type Tab = "market" | "portfolio" | "order" | "backtest" | "risk" | "analytics";

interface NavItem {
  id: Tab;
  label: string;
  glyph: string;
  group: string;
}

const NAV: NavItem[] = [
  { id: "market", label: "Market Data", glyph: "$", group: "Markets" },
  { id: "analytics", label: "Analytics", glyph: "~", group: "Markets" },
  { id: "portfolio", label: "Portfolio", glyph: "#", group: "Trading" },
  { id: "order", label: "Order Ticket", glyph: ">", group: "Trading" },
  { id: "risk", label: "Risk", glyph: "!", group: "Research" },
  { id: "backtest", label: "Strategy Lab", glyph: "*", group: "Research" },
];

interface TickEntry {
  symbol: string;
  last: number;
  change: number;
}

function Clock() {
  const [now, setNow] = useState(new Date());
  useEffect(() => {
    const t = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(t);
  }, []);
  return <span>{now.toUTCString().slice(17, 25)} UTC</span>;
}

function TickerTape({ ticks }: { ticks: TickEntry[] }) {
  if (ticks.length === 0) return <div className="ticker" />;
  const doubled = [...ticks, ...ticks];
  return (
    <div className="ticker">
      <div className="ticker-track">
        {doubled.map((t, i) => (
          <span className="tick" key={i}>
            <span className="sym">{t.symbol}</span>
            <span className="px">{num(t.last)}</span>
            <span className={t.change >= 0 ? "up" : "down"}>
              {signedPct(t.change)}
            </span>
          </span>
        ))}
      </div>
    </div>
  );
}

function initials(name: string): string {
  const parts = name.trim().split(/\s+/).filter(Boolean);
  if (parts.length === 0) return "?";
  if (parts.length === 1) return parts[0].slice(0, 2).toUpperCase();
  return (parts[0][0] + parts[parts.length - 1][0]).toUpperCase();
}

function AccountMenu() {
  const { user, signOut } = useAuth();
  const [open, setOpen] = useState(false);
  if (!user) return null;

  return (
    <div className="account-menu">
      <button
        className="account-trigger"
        onClick={() => setOpen((v) => !v)}
        aria-expanded={open}
      >
        <span className="avatar">{initials(user.name)}</span>
        <span className="account-name">{user.name}</span>
      </button>
      {open && (
        <div className="account-dropdown" onMouseLeave={() => setOpen(false)}>
          <div className="account-email mono">{user.email}</div>
          <Link to="/" className="account-item" onClick={() => setOpen(false)}>
            Homepage
          </Link>
          <button
            className="account-item danger"
            onClick={() => {
              setOpen(false);
              signOut();
            }}
          >
            Sign out
          </button>
        </div>
      )}
    </div>
  );
}

export function Dashboard() {
  const [health, setHealth] = useState<Health | null>(null);
  const [symbols, setSymbols] = useState<SymbolInfo[]>([]);
  const [ticks, setTicks] = useState<TickEntry[]>([]);
  const [tab, setTab] = useState<Tab>("market");
  const [pfVersion, setPfVersion] = useState(0);
  const [ready, setReady] = useState(false);
  const [fatal, setFatal] = useState("");

  useEffect(() => {
    Promise.all([api.health(), api.symbols()])
      .then(([h, s]) => {
        setHealth(h);
        setSymbols(s.symbols);
        setReady(true);
        return Promise.all(
          s.symbols.map((sym) =>
            api
              .market(sym.symbol, 2)
              .then((r) => {
                const b = r.bars;
                const last = b[b.length - 1]?.close ?? 0;
                const prev = b[b.length - 2]?.close ?? last;
                return {
                  symbol: sym.symbol,
                  last,
                  change: prev !== 0 ? last / prev - 1 : 0,
                };
              })
              .catch(() => ({ symbol: sym.symbol, last: 0, change: 0 })),
          ),
        );
      })
      .then((t) => t && setTicks(t))
      .catch((e: Error) => setFatal(e.message));
  }, []);

  const grouped = useMemo(() => {
    const map = new Map<string, NavItem[]>();
    for (const item of NAV) {
      if (!map.has(item.group)) map.set(item.group, []);
      map.get(item.group)!.push(item);
    }
    return [...map.entries()];
  }, []);

  const onTraded = () => setPfVersion((v) => v + 1);

  return (
    <div className="shell">
      <div className="statusbar">
        <Link to="/" className="brand">
          <div className="spark">
            <span className="alpha">Alpha</span>
            <span className="forge">Forge</span>
          </div>
          <span className="tag">Quant Terminal</span>
        </Link>
        <div className="status-items">
          <span>
            <span className={cls("status-dot", !health && "off")} />
            {health ? "CONNECTED" : "OFFLINE"}
          </span>
          {health && <span>v{health.version}</span>}
          {health && <span>{health.symbols_loaded} SYMBOLS</span>}
          {health && <span>{health.threads} THREADS</span>}
          <Clock />
          <AccountMenu />
        </div>
      </div>

      <TickerTape ticks={ticks} />

      <nav className="sidebar">
        {grouped.map(([group, items]) => (
          <div key={group}>
            <div className="nav-group-label">{group}</div>
            {items.map((item) => (
              <button
                key={item.id}
                className={cls("nav-item", tab === item.id && "active")}
                onClick={() => setTab(item.id)}
              >
                <span className="glyph">{item.glyph}</span>
                {item.label}
              </button>
            ))}
          </div>
        ))}
      </nav>

      <main className="workspace">
        {fatal ? (
          <div className="toast err">
            Cannot reach the AlphaForge API: {fatal}. Start the backend server
            and reload.
          </div>
        ) : !ready ? (
          <div className="loading">Connecting to AlphaForge engine...</div>
        ) : symbols.length === 0 ? (
          <div className="empty">
            <div style={{ color: "var(--ember)", fontSize: 15 }}>
              No market data loaded
            </div>
            <div>The engine is running but no instruments are loaded.</div>
            <div className="faint">
              Fetch real data, then restart the server:
            </div>
            <div
              className="mono"
              style={{
                background: "var(--ink-900)",
                border: "1px solid var(--line)",
                borderRadius: 6,
                padding: "10px 14px",
                color: "var(--text-dim)",
                marginTop: 4,
              }}
            >
              python scripts/fetch_data.py --period 3y
            </div>
          </div>
        ) : (
          <>
            {tab === "market" && <MarketPanel symbols={symbols} />}
            {tab === "analytics" && <AnalyticsPanel symbols={symbols} />}
            {tab === "portfolio" && <PortfolioPanel version={pfVersion} />}
            {tab === "order" && (
              <OrderPanel symbols={symbols} onTraded={onTraded} />
            )}
            {tab === "risk" && <RiskPanel symbols={symbols} />}
            {tab === "backtest" && <BacktestPanel symbols={symbols} />}
          </>
        )}
      </main>
    </div>
  );
}
