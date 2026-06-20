import { useEffect, useMemo, useState } from "react";
import {
  Bar,
  BarChart,
  CartesianGrid,
  Cell,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import { api } from "../api";
import { num, pct, signedPct, dirClass } from "../format";
import { Card, ChartTooltip, ErrorState, Loading, Metric } from "../ui";
import type { Analytics, SymbolInfo } from "../types";

export function AnalyticsPanel({ symbols }: { symbols: SymbolInfo[] }) {
  const [symbol, setSymbol] = useState(symbols[0]?.symbol ?? "");
  const [window, setWindow] = useState("20");
  const [data, setData] = useState<Analytics | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  useEffect(() => {
    if (!symbol) return;
    setLoading(true);
    setError("");
    api
      .analytics(symbol, Number(window) || 20)
      .then(setData)
      .catch((e: Error) => setError(e.message))
      .finally(() => setLoading(false));
  }, [symbol, window]);

  const vol = useMemo(
    () =>
      data?.rolling_volatility.map((p) => ({
        date: p.label,
        value: p.value,
      })) ?? [],
    [data],
  );
  const sharpe = useMemo(
    () =>
      data?.rolling_sharpe.map((p) => ({ date: p.label, value: p.value })) ??
      [],
    [data],
  );
  const monthly = useMemo(
    () =>
      data?.monthly_returns.map((p) => ({ label: p.label, value: p.value })) ??
      [],
    [data],
  );

  return (
    <div>
      <div className="panel-head">
        <div>
          <div className="eyebrow">Analytics</div>
          <h1>Performance Analytics</h1>
          <p>
            Calendar-aware returns and rolling risk. Monthly and weekly buckets
            group by real ISO dates rather than a fixed stride.
          </p>
        </div>
        <div className="row">
          <div style={{ minWidth: 150 }}>
            <label className="field">Instrument</label>
            <select value={symbol} onChange={(e) => setSymbol(e.target.value)}>
              {symbols.map((s) => (
                <option key={s.symbol} value={s.symbol}>
                  {s.symbol}
                </option>
              ))}
            </select>
          </div>
          <div style={{ width: 110 }}>
            <label className="field">Window</label>
            <input
              className="inp"
              value={window}
              onChange={(e) => setWindow(e.target.value)}
            />
          </div>
        </div>
      </div>

      {error && <ErrorState message={error} />}
      {loading ? (
        <Loading label="Computing analytics" />
      ) : data ? (
        <>
          <div className="grid cols-3" style={{ marginBottom: 16 }}>
            <Metric
              label="Annualized Return"
              value={signedPct(data.annualized_return)}
              tone={dirClass(data.annualized_return)}
            />
            <Metric label="Months Observed" value={String(monthly.length)} />
            <Metric label="Rolling Window" value={`${data.window} days`} />
          </div>

          <div className="grid cols-2">
            <Card title={`Rolling Volatility (${data.window}d, annualized)`}>
              <ResponsiveContainer width="100%" height={220}>
                <LineChart
                  data={vol}
                  margin={{ top: 6, right: 10, bottom: 0, left: 4 }}
                >
                  <CartesianGrid stroke="#1c2233" vertical={false} />
                  <XAxis
                    dataKey="date"
                    minTickGap={56}
                    tickLine={false}
                    axisLine={false}
                  />
                  <YAxis
                    width={48}
                    tickLine={false}
                    axisLine={false}
                    tickFormatter={(v: number) => pct(v, 0)}
                  />
                  <Tooltip
                    content={(p) => (
                      <ChartTooltip {...p} formatter={(v) => pct(v)} />
                    )}
                  />
                  <Line
                    type="monotone"
                    dataKey="value"
                    stroke="#5b8def"
                    strokeWidth={1.5}
                    dot={false}
                    name="Vol"
                  />
                </LineChart>
              </ResponsiveContainer>
            </Card>

            <Card title={`Rolling Sharpe (${data.window}d)`}>
              <ResponsiveContainer width="100%" height={220}>
                <LineChart
                  data={sharpe}
                  margin={{ top: 6, right: 10, bottom: 0, left: 4 }}
                >
                  <CartesianGrid stroke="#1c2233" vertical={false} />
                  <XAxis
                    dataKey="date"
                    minTickGap={56}
                    tickLine={false}
                    axisLine={false}
                  />
                  <YAxis
                    width={40}
                    tickLine={false}
                    axisLine={false}
                    tickFormatter={(v: number) => num(v, 1)}
                  />
                  <Tooltip
                    content={(p) => (
                      <ChartTooltip {...p} formatter={(v) => num(v)} />
                    )}
                  />
                  <Line
                    type="monotone"
                    dataKey="value"
                    stroke="#44d6a0"
                    strokeWidth={1.5}
                    dot={false}
                    name="Sharpe"
                  />
                </LineChart>
              </ResponsiveContainer>
            </Card>
          </div>

          <Card title="Monthly Returns" style={{ marginTop: 16 }}>
            <ResponsiveContainer width="100%" height={240}>
              <BarChart
                data={monthly}
                margin={{ top: 6, right: 10, bottom: 0, left: 4 }}
              >
                <CartesianGrid stroke="#1c2233" vertical={false} />
                <XAxis
                  dataKey="label"
                  minTickGap={20}
                  tickLine={false}
                  axisLine={false}
                />
                <YAxis
                  width={48}
                  tickLine={false}
                  axisLine={false}
                  tickFormatter={(v: number) => pct(v, 0)}
                />
                <Tooltip
                  cursor={{ fill: "#ffffff08" }}
                  content={(p) => (
                    <ChartTooltip {...p} formatter={(v) => signedPct(v)} />
                  )}
                />
                <Bar dataKey="value" name="Return" radius={[2, 2, 0, 0]}>
                  {monthly.map((m, i) => (
                    <Cell key={i} fill={m.value >= 0 ? "#44d6a0" : "#ff5d73"} />
                  ))}
                </Bar>
              </BarChart>
            </ResponsiveContainer>
          </Card>
        </>
      ) : null}
    </div>
  );
}
