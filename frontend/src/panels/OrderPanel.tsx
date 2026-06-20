import { useState } from "react";
import { api, type OrderRequest } from "../api";
import { money } from "../format";
import { Card } from "../ui";
import type { OrderResponse, SymbolInfo } from "../types";

export function OrderPanel({
  symbols,
  onTraded,
}: {
  symbols: SymbolInfo[];
  onTraded: () => void;
}) {
  const [symbol, setSymbol] = useState(symbols[0]?.symbol ?? "");
  const [side, setSide] = useState<"buy" | "sell">("buy");
  const [type, setType] = useState<"market" | "limit" | "stop">("market");
  const [quantity, setQuantity] = useState("100");
  const [limitPrice, setLimitPrice] = useState("");
  const [stopPrice, setStopPrice] = useState("");
  const [busy, setBusy] = useState(false);
  const [result, setResult] = useState<OrderResponse | null>(null);
  const [error, setError] = useState("");

  const submit = async () => {
    setError("");
    setResult(null);
    const qty = Number(quantity);
    if (!symbol || !Number.isFinite(qty) || qty <= 0) {
      setError("Enter a valid symbol and positive quantity.");
      return;
    }
    const req: OrderRequest = { symbol, side, type, quantity: qty };
    if (type === "limit") req.limit_price = Number(limitPrice);
    if (type === "stop") req.stop_price = Number(stopPrice);

    setBusy(true);
    try {
      const res = await api.order(req);
      setResult(res);
      onTraded();
    } catch (e) {
      setError((e as Error).message);
    } finally {
      setBusy(false);
    }
  };

  const undo = async () => {
    setBusy(true);
    setError("");
    try {
      await api.undo();
      onTraded();
      setResult(null);
    } catch (e) {
      setError((e as Error).message);
    } finally {
      setBusy(false);
    }
  };

  const exec = result?.execution;
  const filled = exec?.status === "FILLED";

  return (
    <div>
      <div className="panel-head">
        <div>
          <div className="eyebrow">Execution</div>
          <h1>Order Ticket</h1>
          <p>
            Orders route to the C++ order manager and fill against the latest
            close using a marketable fill model. Limit and stop orders fill only
            when the reference price satisfies their condition.
          </p>
        </div>
      </div>

      <div className="grid cols-2">
        <Card title="New Order">
          <div className="grid cols-2">
            <div>
              <label className="field">Instrument</label>
              <select
                value={symbol}
                onChange={(e) => setSymbol(e.target.value)}
              >
                {symbols.map((s) => (
                  <option key={s.symbol} value={s.symbol}>
                    {s.symbol}
                  </option>
                ))}
              </select>
            </div>
            <div>
              <label className="field">Order Type</label>
              <select
                value={type}
                onChange={(e) => setType(e.target.value as typeof type)}
              >
                <option value="market">Market</option>
                <option value="limit">Limit</option>
                <option value="stop">Stop</option>
              </select>
            </div>
          </div>

          <div className="grid cols-2 mt-16">
            <div>
              <label className="field">Quantity</label>
              <input
                className="inp"
                value={quantity}
                onChange={(e) => setQuantity(e.target.value)}
                inputMode="numeric"
              />
            </div>
            <div>
              <label className="field">
                {type === "limit"
                  ? "Limit Price"
                  : type === "stop"
                    ? "Stop Price"
                    : "Reference"}
              </label>
              <input
                className="inp"
                value={
                  type === "limit"
                    ? limitPrice
                    : type === "stop"
                      ? stopPrice
                      : "latest close"
                }
                onChange={(e) =>
                  type === "limit"
                    ? setLimitPrice(e.target.value)
                    : setStopPrice(e.target.value)
                }
                disabled={type === "market"}
                placeholder={type === "market" ? "" : "0.00"}
              />
            </div>
          </div>

          <div className="row mt-24">
            <button
              className={`btn ${side === "buy" ? "buy" : "ghost"}`}
              onClick={() => setSide("buy")}
            >
              Buy
            </button>
            <button
              className={`btn ${side === "sell" ? "sell" : "ghost"}`}
              onClick={() => setSide("sell")}
            >
              Sell
            </button>
            <div className="spacer" />
            <button className="btn ghost" onClick={undo} disabled={busy}>
              Undo Last
            </button>
            <button className="btn" onClick={submit} disabled={busy}>
              {busy ? "Working" : `Submit ${side.toUpperCase()}`}
            </button>
          </div>

          {error && <div className="toast err mt-16">{error}</div>}
          {exec && (
            <div className={`toast ${filled ? "ok" : "err"} mt-16`}>
              Order #{exec.order_id} {exec.status}
              {filled && exec.fill_price != null
                ? ` at ${exec.fill_price.toFixed(2)}`
                : exec.reason
                  ? ` (${exec.reason})`
                  : ""}
            </div>
          )}
        </Card>

        <Card title="Account After Fill">
          {result ? (
            <div className="grid cols-2">
              <div className="metric">
                <div className="label">Cash</div>
                <div className="value">{money(result.portfolio.cash)}</div>
              </div>
              <div className="metric">
                <div className="label">Total Value</div>
                <div className="value">
                  {money(result.portfolio.total_value)}
                </div>
              </div>
              <div className="metric">
                <div className="label">Gross Exposure</div>
                <div className="value">
                  {money(result.portfolio.gross_exposure)}
                </div>
              </div>
              <div className="metric">
                <div className="label">Positions</div>
                <div className="value">{result.portfolio.positions.length}</div>
              </div>
            </div>
          ) : (
            <div className="dim mono" style={{ fontSize: 12 }}>
              Submit an order to see the resulting account state. The portfolio
              dashboard updates automatically.
            </div>
          )}
        </Card>
      </div>
    </div>
  );
}
