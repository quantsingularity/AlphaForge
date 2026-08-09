import { useState, type FormEvent } from "react";
import { Link, Navigate, useLocation, useNavigate } from "react-router-dom";
import { useAuth } from "../auth/AuthContext";

export function SignIn() {
  const { user, ready, signIn } = useAuth();
  const navigate = useNavigate();
  const location = useLocation();

  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");

  if (ready && user) {
    const from =
      (location.state as { from?: string } | null)?.from ?? "/terminal";
    return <Navigate to={from} replace />;
  }

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setError("");
    if (!email || !password) {
      setError("Enter your email and password.");
      return;
    }
    setBusy(true);
    try {
      await signIn(email, password);
      navigate("/terminal", { replace: true });
    } catch (err) {
      setError((err as Error).message);
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="auth-shell">
      <div className="auth-side">
        <Link to="/" className="brand">
          <div className="spark">
            <span className="alpha">Alpha</span>
            <span className="forge">Forge</span>
          </div>
          <span className="tag">Quant Terminal</span>
        </Link>
        <h1>Welcome back to the desk.</h1>
        <p className="dim">
          Sign in to reach your portfolio, order ticket, risk dashboard, and
          strategy lab.
        </p>
        <ul className="auth-points">
          <li>Live portfolio marks against loaded market data</li>
          <li>Order routing through the C++ execution engine</li>
          <li>Parallel risk metrics across your full book</li>
        </ul>
      </div>

      <div className="auth-main">
        <div className="auth-card">
          <div className="eyebrow">Sign in</div>
          <h2>Access your terminal</h2>
          <p className="dim auth-sub">
            New to AlphaForge?{" "}
            <Link to="/sign-up" className="auth-link">
              Create an account
            </Link>
            .
          </p>

          <form onSubmit={submit} noValidate>
            <label className="field" htmlFor="signin-email">
              Email
            </label>
            <input
              id="signin-email"
              className="inp"
              type="email"
              autoComplete="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="you@firm.com"
            />

            <label className="field mt-16" htmlFor="signin-password">
              Password
            </label>
            <input
              id="signin-password"
              className="inp"
              type="password"
              autoComplete="current-password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder="Enter your password"
            />

            {error && <div className="toast err mt-16">{error}</div>}

            <button className="btn auth-submit mt-24" disabled={busy}>
              {busy ? "Signing in" : "Sign in"}
            </button>
          </form>
        </div>
      </div>
    </div>
  );
}
