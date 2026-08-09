// Thin typed client over the AlphaForge REST API. All calls hit the same C++
// backend; in development Vite proxies /api to it, in production the backend
// serves this bundle directly so the relative paths just work.

import type {
  Analytics,
  AuthResponse,
  AuthUser,
  BacktestResult,
  Bar,
  Health,
  OrderResponse,
  Portfolio,
  RiskMetrics,
  SymbolInfo,
} from "./types";

const BASE = "/api";
const TOKEN_KEY = "alphaforge.token";

// The bearer token is kept in memory for the life of the tab and mirrored to
// localStorage so a page refresh does not sign the user out. AuthContext is
// the only caller that should invoke setAuthToken; everything else just
// benefits from it being attached automatically below.
let authToken: string | null =
  typeof window !== "undefined" ? window.localStorage.getItem(TOKEN_KEY) : null;

export function getAuthToken(): string | null {
  return authToken;
}

export function setAuthToken(token: string | null): void {
  authToken = token;
  if (typeof window === "undefined") return;
  if (token) {
    window.localStorage.setItem(TOKEN_KEY, token);
  } else {
    window.localStorage.removeItem(TOKEN_KEY);
  }
}

function authHeaders(): Record<string, string> {
  return authToken ? { Authorization: `Bearer ${authToken}` } : {};
}

async function getJson<T>(path: string): Promise<T> {
  const res = await fetch(`${BASE}${path}`, { headers: authHeaders() });
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
    headers: { "Content-Type": "application/json", ...authHeaders() },
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
  register: (name: string, email: string, password: string) =>
    postJson<AuthResponse>("/auth/register", { name, email, password }),
  login: (email: string, password: string) =>
    postJson<AuthResponse>("/auth/login", { email, password }),
  logout: () => postJson<{ ok: boolean }>("/auth/logout", {}),
  me: () => getJson<{ user: AuthUser }>("/auth/me"),
};
