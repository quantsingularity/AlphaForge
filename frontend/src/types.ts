// Types mirroring the JSON returned by the AlphaForge C++ API.

export interface Health {
  status: string;
  version: string;
  symbols_loaded: number;
  threads: number;
}

export interface SymbolInfo {
  symbol: string;
  bars: number;
}

export interface Bar {
  date: string;
  open: number;
  high: number;
  low: number;
  close: number;
  volume: number;
}

export interface PositionView {
  symbol: string;
  quantity: number;
  avg_price: number;
  realized_pnl: number;
  mark?: number;
  market_value?: number;
  unrealized_pnl?: number;
}

export interface Weight {
  symbol: string;
  weight: number;
}

export interface Fill {
  symbol: string;
  side: string;
  quantity: number;
  price: number;
  cash_delta: number;
  timestamp: string;
}

export interface Portfolio {
  cash: number;
  initial_cash: number;
  holdings_value: number;
  total_value: number;
  realized_pnl: number;
  unrealized_pnl: number;
  gross_exposure: number;
  net_exposure: number;
  positions: PositionView[];
  weights: Weight[];
  blotter?: Fill[];
  pending_orders?: number;
  executed_orders?: number;
}

export interface RiskMetrics {
  symbol?: string;
  annualized_volatility: number;
  sharpe_ratio: number;
  sortino_ratio: number;
  max_drawdown: number;
  value_at_risk: number;
  conditional_var: number;
  beta: number | null;
  confidence?: number;
  samples?: number;
}

export interface SeriesPoint {
  label: string;
  value: number;
}

export interface Analytics {
  symbol: string;
  annualized_return: number;
  monthly_returns: SeriesPoint[];
  weekly_returns: SeriesPoint[];
  rolling_volatility: SeriesPoint[];
  rolling_sharpe: SeriesPoint[];
  window: number;
}

export interface EquityPoint {
  date: string;
  equity: number;
  target_weight: number;
}

export interface BacktestMetrics {
  total_return: number;
  cagr: number;
  annualized_volatility: number;
  sharpe_ratio: number;
  sortino_ratio: number;
  max_drawdown: number;
  win_rate: number;
  profit_factor: number;
  num_trades: number;
}

export interface BacktestResult {
  strategy: string;
  symbol: string;
  equity_curve: EquityPoint[];
  metrics: BacktestMetrics;
  benchmark: BacktestMetrics;
}

export interface OrderResponse {
  execution: {
    order_id: number;
    status?: string;
    reason?: string;
    fill_price?: number;
  };
  portfolio: Portfolio;
}

export interface AuthUser {
  id: string;
  name: string;
  email: string;
  created_at: string;
}

export interface AuthResponse {
  token: string;
  user: AuthUser;
}
