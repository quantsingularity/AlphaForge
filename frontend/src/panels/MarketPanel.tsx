import { useEffect, useMemo, useState } from "react";
import {
  Area,
  AreaChart,
  CartesianGrid,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import { api } from "../api";
import { num, compact, dirClass, signedPct } from "../format";
import { Card, ChartTooltip, ErrorState, Loading, Metric } from "../ui";
import type { Bar, SymbolInfo } from "../types";

export function MarketPanel({ symbols }: { symbols: SymbolInfo[] }) {
  const [symbol, setSymbol] = useState(symbols[0]?.symbol ?? "");
  const [bars, setBars] = useState<Bar[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  useEffect(() => {
    if (!symbol) return;
    setLoading(true);
    setError("");
    api
      .market(symbol)
      .then((r) => setBars(r.bars))
      .catch((e: Error) => setError(e.message))
      .finally(() => setLoading(false));
  }, [symbol]);

  const stats = useMemo(() => {
    if (bars.length < 2) return null;
    const first = bars[0].close;
    const last = bars[bars.length - 1].close;
    const hi = Math.max(...bars.map((b) => b.high));
    const lo = Math.min(...bars.map((b) => b.low));
    const dayChange =
      bars.length >= 2
        ? bars[bars.length - 1].close / bars[bars.length - 2].close - 1
        : 0;
    return { first, last, hi, lo, dayChange, periodReturn: last / first - 1 };
  }, [bars]);

  const chartData = useMemo(
    () => bars.map((b) => ({ date: b.date, close: b.close })),
    [bars],
  );

  return (
    <div>
      <div className="panel-head">
        <div>
          <div className="eyebrow">Market Data</div>
          <h1>Price History</h1>
          <p>
            Daily OHLCV loaded and served by the C++ market data engine. Series
            are sorted and cached on the backend.
          </p>
        </div>
        <div style={{ minWidth: 180 }}>
          <label className="field">Instrument</label>
          <select value={symbol} onChange={(e) => setSymbol(e.target.value)}>
            {symbols.map((s) => (
              <option key={s.symbol} value={s.symbol}>
                {s.symbol} ({s.bars} bars)
              </option>
            ))}
          </select>
        </div>
      </div>

      {error && <ErrorState message={error} />}
      {loading ? (
        <Loading label="Fetching bars" />
      ) : stats ? (
        <>
          <div className="grid cols-4" style={{ marginBottom: 16 }}>
            <Metric label="Last" value={num(stats.last)} />
            <Metric
              label="Last Change"
              value={signedPct(stats.dayChange)}
              tone={dirClass(stats.dayChange)}
            />
            <Metric
              label="Period Return"
              value={signedPct(stats.periodReturn)}
              tone={dirClass(stats.periodReturn)}
            />
            <Metric
              label="Range"
              value={`${num(stats.lo)} - ${num(stats.hi)}`}
            />
          </div>

          <Card title={`${symbol} Close`}>
            <ResponsiveContainer width="100%" height={320}>
              <AreaChart
                data={chartData}
                margin={{ top: 6, right: 12, bottom: 0, left: 6 }}
              >
                <defs>
                  <linearGradient id="px" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="0%" stopColor="#f0a23c" stopOpacity={0.35} />
                    <stop offset="100%" stopColor="#f0a23c" stopOpacity={0} />
                  </linearGradient>
                </defs>
                <CartesianGrid stroke="#1c2233" vertical={false} />
                <XAxis
                  dataKey="date"
                  minTickGap={48}
                  tickLine={false}
                  axisLine={false}
                />
                <YAxis
                  domain={["auto", "auto"]}
                  width={56}
                  tickLine={false}
                  axisLine={false}
                  tickFormatter={(v: number) => num(v, 0)}
                />
                <Tooltip
                  content={(p) => (
                    <ChartTooltip {...p} formatter={(v) => num(v)} />
                  )}
                />
                <Area
                  type="monotone"
                  dataKey="close"
                  stroke="#f0a23c"
                  strokeWidth={1.6}
                  fill="url(#px)"
                  name="Close"
                />
              </AreaChart>
            </ResponsiveContainer>
          </Card>

          <Card title="Recent Bars" style={{ marginTop: 16 }}>
            <div style={{ maxHeight: 280, overflowY: "auto" }}>
              <table className="data">
                <thead>
                  <tr>
                    <th>Date</th>
                    <th>Open</th>
                    <th>High</th>
                    <th>Low</th>
                    <th>Close</th>
                    <th>Volume</th>
                  </tr>
                </thead>
                <tbody>
                  {[...bars]
                    .slice(-20)
                    .reverse()
                    .map((b) => (
                      <tr key={b.date}>
                        <td>{b.date}</td>
                        <td>{num(b.open)}</td>
                        <td>{num(b.high)}</td>
                        <td>{num(b.low)}</td>
                        <td>{num(b.close)}</td>
                        <td className="dim">{compact(b.volume)}</td>
                      </tr>
                    ))}
                </tbody>
              </table>
            </div>
          </Card>
        </>
      ) : null}
    </div>
  );
}
