import { useEffect, useState } from "react";
import { api } from "../api";
import { num, pct, dirClass } from "../format";
import { Card, ErrorState, Loading, Metric } from "../ui";
import type { RiskMetrics, SymbolInfo } from "../types";

export function RiskPanel({ symbols }: { symbols: SymbolInfo[] }) {
  const [batch, setBatch] = useState<RiskMetrics[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  const [symbol, setSymbol] = useState(symbols[0]?.symbol ?? "");
  const [benchmark, setBenchmark] = useState(
    symbols.find((s) => s.symbol === "SPY")?.symbol ?? symbols[0]?.symbol ?? "",
  );
  const [detail, setDetail] = useState<RiskMetrics | null>(null);
  const [detailErr, setDetailErr] = useState("");

  useEffect(() => {
    api
      .riskBatch()
      .then((r) => setBatch(r.risk))
      .catch((e: Error) => setError(e.message))
      .finally(() => setLoading(false));
  }, []);

  useEffect(() => {
    if (!symbol) return;
    setDetailErr("");
    api
      .risk(symbol, benchmark !== symbol ? benchmark : undefined)
      .then(setDetail)
      .catch((e: Error) => setDetailErr(e.message));
  }, [symbol, benchmark]);

  return (
    <div>
      <div className="panel-head">
        <div>
          <div className="eyebrow">Risk</div>
          <h1>Risk Analytics</h1>
          <p>
            Volatility, Sharpe, Sortino, drawdown and historical VaR/CVaR. The
            cross- sectional table is computed in parallel on the backend thread
            pool.
          </p>
        </div>
      </div>

      <Card title="Cross-Sectional Risk (parallel batch)">
        {error && <ErrorState message={error} />}
        {loading ? (
          <Loading label="Computing risk" />
        ) : (
          <table className="data">
            <thead>
              <tr>
                <th>Symbol</th>
                <th>Ann. Vol</th>
                <th>Sharpe</th>
                <th>Sortino</th>
                <th>Max DD</th>
                <th>VaR 95%</th>
                <th>CVaR 95%</th>
              </tr>
            </thead>
            <tbody>
              {batch.map((r) => (
                <tr key={r.symbol}>
                  <td>{r.symbol}</td>
                  <td>{pct(r.annualized_volatility)}</td>
                  <td className={dirClass(r.sharpe_ratio)}>
                    {num(r.sharpe_ratio)}
                  </td>
                  <td className={dirClass(r.sortino_ratio)}>
                    {num(r.sortino_ratio)}
                  </td>
                  <td className="down">{pct(r.max_drawdown)}</td>
                  <td>{pct(r.value_at_risk)}</td>
                  <td>{pct(r.conditional_var)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </Card>

      <Card title="Single Instrument Detail" style={{ marginTop: 16 }}>
        <div className="grid cols-2" style={{ marginBottom: 16 }}>
          <div>
            <label className="field">Instrument</label>
            <select value={symbol} onChange={(e) => setSymbol(e.target.value)}>
              {symbols.map((s) => (
                <option key={s.symbol} value={s.symbol}>
                  {s.symbol}
                </option>
              ))}
            </select>
          </div>
          <div>
            <label className="field">Beta Benchmark</label>
            <select
              value={benchmark}
              onChange={(e) => setBenchmark(e.target.value)}
            >
              {symbols.map((s) => (
                <option key={s.symbol} value={s.symbol}>
                  {s.symbol}
                </option>
              ))}
            </select>
          </div>
        </div>
        {detailErr && <ErrorState message={detailErr} />}
        {detail && (
          <div className="grid cols-4">
            <Metric
              label="Ann. Volatility"
              value={pct(detail.annualized_volatility)}
            />
            <Metric
              label="Sharpe"
              value={num(detail.sharpe_ratio)}
              tone={dirClass(detail.sharpe_ratio)}
            />
            <Metric
              label="Sortino"
              value={num(detail.sortino_ratio)}
              tone={dirClass(detail.sortino_ratio)}
            />
            <Metric label="Max Drawdown" value={pct(detail.max_drawdown)} />
            <Metric label="VaR 95%" value={pct(detail.value_at_risk)} />
            <Metric label="CVaR 95%" value={pct(detail.conditional_var)} />
            <Metric
              label={`Beta vs ${benchmark}`}
              value={detail.beta != null ? num(detail.beta) : "n/a"}
            />
            <Metric label="Samples" value={String(detail.samples ?? 0)} />
          </div>
        )}
      </Card>
    </div>
  );
}
