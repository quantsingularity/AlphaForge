// Formatting helpers shared across panels.

export const pct = (x: number, digits = 2): string =>
  `${(x * 100).toFixed(digits)}%`;

export const signedPct = (x: number, digits = 2): string =>
  `${x >= 0 ? "+" : ""}${(x * 100).toFixed(digits)}%`;

export const num = (x: number, digits = 2): string =>
  x.toLocaleString("en-US", {
    minimumFractionDigits: digits,
    maximumFractionDigits: digits,
  });

export const money = (x: number): string =>
  x.toLocaleString("en-US", {
    style: "currency",
    currency: "USD",
    maximumFractionDigits: 0,
  });

export const compact = (x: number): string =>
  x.toLocaleString("en-US", { notation: "compact", maximumFractionDigits: 1 });

export const cls = (...parts: (string | false | undefined)[]): string =>
  parts.filter(Boolean).join(" ");

export const dirClass = (x: number): "up" | "down" | "" =>
  x > 0 ? "up" : x < 0 ? "down" : "";
