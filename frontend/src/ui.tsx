import type { ReactNode } from "react";
import { cls } from "./format";

export function Card({
  title,
  children,
  style,
}: {
  title?: string;
  children: ReactNode;
  style?: React.CSSProperties;
}) {
  return (
    <div className="card" style={style}>
      {title && <div className="card-title">{title}</div>}
      {children}
    </div>
  );
}

export function Metric({
  label,
  value,
  sub,
  tone,
}: {
  label: string;
  value: string;
  sub?: string;
  tone?: "up" | "down" | "";
}) {
  return (
    <div className="metric">
      <div className="label">{label}</div>
      <div className={cls("value", tone)}>{value}</div>
      {sub && <div className="sub">{sub}</div>}
    </div>
  );
}

export function Loading({ label = "Loading" }: { label?: string }) {
  return <div className="loading">{label}...</div>;
}

export function ErrorState({ message }: { message: string }) {
  return <div className="toast err">{message}</div>;
}

export function Empty({ children }: { children: ReactNode }) {
  return <div className="empty">{children}</div>;
}

// recharts' Tooltip content callback passes a richly generic props object. We
// only read a few fields, so we accept a loose shape and normalize internally
// rather than mirror recharts' internal generics.
interface LooseTooltipProps {
  active?: boolean;
  label?: unknown;
  payload?: Array<{ name?: unknown; value?: unknown; color?: string }>;
  formatter?: (v: number) => string;
}

export function ChartTooltip(props: LooseTooltipProps) {
  const { active, label, payload, formatter } = props;
  if (!active || !payload || payload.length === 0) return null;
  const fmt = formatter || ((v: number) => v.toFixed(2));
  return (
    <div className="af-tooltip">
      <div className="t-date">{String(label ?? "")}</div>
      {payload.map((p, i) => (
        <div key={i} style={{ color: p.color }}>
          {String(p.name ?? "")}:{" "}
          {typeof p.value === "number" ? fmt(p.value) : String(p.value ?? "")}
        </div>
      ))}
    </div>
  );
}
