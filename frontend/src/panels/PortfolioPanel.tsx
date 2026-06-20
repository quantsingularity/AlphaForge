import { useEffect, useState } from "react";
import { Cell, Pie, PieChart, ResponsiveContainer, Tooltip } from "recharts";
import { api } from "../api";
import { money, num, dirClass, pct } from "../format";
import { Card, ErrorState, Loading, Metric } from "../ui";
import type { Portfolio } from "../types";

const SLICE = [
  "#f0a23c",
  "#5b8def",
  "#44d6a0",
  "#ff5d73",
  "#a78bfa",
  "#e7ebf3",
];

export function PortfolioPanel({ version }: { version: number }) {
  const [pf, setPf] = useState<Portfolio | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  useEffect(() => {
    setLoading(true);
    api
      .portfolio()
      .then(setPf)
      .catch((e: Error) => setError(e.message))
      .finally(() => setLoading(false));
  }, [version]);

  if (loading && !pf) return <Loading label="Loading portfolio" />;
  if (error) return <ErrorState message={error} />;
  if (!pf) return null;

  const pnl = pf.total_value - pf.initial_cash;
  const pnlPct = pf.initial_cash !== 0 ? pnl / pf.initial_cash : 0;
  const pieData = pf.weights.map((w) => ({
    name: w.symbol,
    value: Math.abs(w.weight),
  }));
  const cashWeight = pf.total_value !== 0 ? pf.cash / pf.total_value : 0;
  if (cashWeight > 0) pieData.push({ name: "Cash", value: cashWeight });

  return (
    <div>
      <div className="panel-head">
        <div>
          <div className="eyebrow">Portfolio</div>
          <h1>Account Dashboard</h1>
          <p>
            Live valuation from the execution engine. Positions are average-cost
            and marked to the latest close.
          </p>
        </div>
      </div>

      <div className="grid cols-4" style={{ marginBottom: 16 }}>
        <Metric label="Total Value" value={money(pf.total_value)} />
        <Metric
          label="Total P&L"
          value={money(pnl)}
          sub={pct(pnlPct)}
          tone={dirClass(pnl)}
        />
        <Metric label="Cash" value={money(pf.cash)} />
        <Metric label="Holdings" value={money(pf.holdings_value)} />
      </div>
      <div className="grid cols-4" style={{ marginBottom: 16 }}>
        <Metric
          label="Realized P&L"
          value={money(pf.realized_pnl)}
          tone={dirClass(pf.realized_pnl)}
        />
        <Metric
          label="Unrealized P&L"
          value={money(pf.unrealized_pnl)}
          tone={dirClass(pf.unrealized_pnl)}
        />
        <Metric label="Gross Exposure" value={money(pf.gross_exposure)} />
        <Metric label="Net Exposure" value={money(pf.net_exposure)} />
      </div>

      <div className="grid cols-2">
        <Card title="Open Positions">
          {pf.positions.length === 0 ? (
            <div className="dim mono" style={{ fontSize: 12 }}>
              No open positions. Use the Order Ticket to trade.
            </div>
          ) : (
            <table className="data">
              <thead>
                <tr>
                  <th>Symbol</th>
                  <th>Qty</th>
                  <th>Avg</th>
                  <th>Mark</th>
                  <th>uP&L</th>
                </tr>
              </thead>
              <tbody>
                {pf.positions.map((p) => (
                  <tr key={p.symbol}>
                    <td>{p.symbol}</td>
                    <td>{num(p.quantity, 0)}</td>
                    <td>{num(p.avg_price)}</td>
                    <td>{p.mark != null ? num(p.mark) : "-"}</td>
                    <td className={dirClass(p.unrealized_pnl ?? 0)}>
                      {p.unrealized_pnl != null ? money(p.unrealized_pnl) : "-"}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </Card>

        <Card title="Allocation">
          {pieData.length === 0 ? (
            <div className="dim mono" style={{ fontSize: 12 }}>
              Fully in cash.
            </div>
          ) : (
            <ResponsiveContainer width="100%" height={240}>
              <PieChart>
                <Pie
                  data={pieData}
                  dataKey="value"
                  nameKey="name"
                  innerRadius={52}
                  outerRadius={92}
                  paddingAngle={2}
                  stroke="#0e121a"
                >
                  {pieData.map((_, i) => (
                    <Cell key={i} fill={SLICE[i % SLICE.length]} />
                  ))}
                </Pie>
                <Tooltip
                  formatter={(v: number, n: string) => [pct(v), n]}
                  contentStyle={{
                    background: "#171d2c",
                    border: "1px solid #262d40",
                    borderRadius: 6,
                    fontFamily: "var(--font-mono)",
                    fontSize: 12,
                  }}
                />
              </PieChart>
            </ResponsiveContainer>
          )}
        </Card>
      </div>

      {pf.blotter && pf.blotter.length > 0 && (
        <Card title="Trade Blotter" style={{ marginTop: 16 }}>
          <table className="data">
            <thead>
              <tr>
                <th>Symbol</th>
                <th>Side</th>
                <th>Qty</th>
                <th>Price</th>
                <th>Cash Delta</th>
              </tr>
            </thead>
            <tbody>
              {[...pf.blotter].reverse().map((f, i) => (
                <tr key={i}>
                  <td>{f.symbol}</td>
                  <td className={f.side === "BUY" ? "up" : "down"}>{f.side}</td>
                  <td>{num(f.quantity, 0)}</td>
                  <td>{num(f.price)}</td>
                  <td className={dirClass(f.cash_delta)}>
                    {money(f.cash_delta)}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </Card>
      )}
    </div>
  );
}
