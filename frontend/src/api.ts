// Thin typed client over the AlphaForge REST API. All calls hit the same C++
// backend; in development Vite proxies /api to it, in production the backend
// serves this bundle directly so the relative paths just work.

import type {
  Analytics,
  BacktestResult,
  Bar,
  Health,
  OrderResponse,
  Portfolio,
  RiskMetrics,
  SymbolInfo,
} from "./types";

const BASE = "/api";

async function getJson<T>(path: string): Promise<T> {
  const res = await fetch(`${BASE}${path}`);
  if (!res.ok) {
    const body = await res.json().catch(() => ({}));
    throw new Error(
      (body as { error?: string }).error || `request failed (${res.status})`,
    );
  }
  return res.json() as Promise<T>;
}

async function postJson<T>(path: string, payload: unknown): Promise<T> {
  const res = await fetch(`${BASE}${path}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  if (!res.ok) {
    const body = await res.json().catch(() => ({}));
    throw new Error(
      (body as { error?: string }).error || `request failed (${res.status})`,
    );
  }
  return res.json() as Promise<T>;
}

export interface OrderRequest {
  symbol: string;
  side: "buy" | "sell";
  type: "market" | "limit" | "stop";
  quantity: number;
  limit_price?: number | null;
  stop_price?: number | null;
}

export interface BacktestRequest {
  symbol: string;
  strategy: string;
  initial_capital?: number;
  transaction_cost_bps?: number;
  slippage_bps?: number;
}

export const api = {
  health: () => getJson<Health>("/health"),
  strategies: () => getJson<{ strategies: string[] }>("/strategies"),
  symbols: () => getJson<{ symbols: SymbolInfo[] }>("/symbols"),
  market: (symbol: string, limit?: number) =>
    getJson<{ symbol: string; bars: Bar[] }>(
      `/market/${symbol}${limit ? `?limit=${limit}` : ""}`,
    ),
  portfolio: () => getJson<Portfolio>("/portfolio"),
  risk: (symbol: string, benchmark?: string, confidence = 0.95) =>
    getJson<RiskMetrics>(
      `/risk/${symbol}?confidence=${confidence}${benchmark ? `&benchmark=${benchmark}` : ""}`,
    ),
  riskBatch: (confidence = 0.95) =>
    getJson<{ confidence: number; risk: RiskMetrics[] }>(
      `/risk?confidence=${confidence}`,
    ),
  analytics: (symbol: string, window = 20) =>
    getJson<Analytics>(`/analytics/${symbol}?window=${window}`),
  order: (req: OrderRequest) => postJson<OrderResponse>("/order", req),
  undo: () => postJson<{ undone: boolean; portfolio: Portfolio }>("/undo", {}),
  backtest: (req: BacktestRequest) =>
    postJson<BacktestResult>("/backtest", req),
};
