import { useState, type FormEvent } from "react";
import { Link, Navigate, useNavigate } from "react-router-dom";
import { useAuth } from "../auth/AuthContext";

export function SignUp() {
  const { user, ready, signUp } = useAuth();
  const navigate = useNavigate();

  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [confirm, setConfirm] = useState("");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");

  if (ready && user) {
    return <Navigate to="/terminal" replace />;
  }

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setError("");
    if (name.trim().length < 2) {
      setError("Enter your full name.");
      return;
    }
    if (!email.includes("@")) {
      setError("Enter a valid email address.");
      return;
    }
    if (password.length < 8) {
      setError("Password must be at least 8 characters.");
      return;
    }
    if (password !== confirm) {
      setError("Passwords do not match.");
      return;
    }
    setBusy(true);
    try {
      await signUp(name, email, password);
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
        <h1>Set up your desk in under a minute.</h1>
        <p className="dim">
          Create an account to open the terminal, load market data, and start
          trading against the engine.
        </p>
        <ul className="auth-points">
          <li>Free to create, no card required</li>
          <li>Your credentials are hashed and salted before storage</li>
          <li>Bring your own price history with the fetch script</li>
        </ul>
      </div>

      <div className="auth-main">
        <div className="auth-card">
          <div className="eyebrow">Create account</div>
          <h2>Get started</h2>
          <p className="dim auth-sub">
            Already have an account?{" "}
            <Link to="/sign-in" className="auth-link">
              Sign in
            </Link>
            .
          </p>

          <form onSubmit={submit} noValidate>
            <label className="field" htmlFor="signup-name">
              Full name
            </label>
            <input
              id="signup-name"
              className="inp"
              type="text"
              autoComplete="name"
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="Ada Lovelace"
            />

            <label className="field mt-16" htmlFor="signup-email">
              Email
            </label>
            <input
              id="signup-email"
              className="inp"
              type="email"
              autoComplete="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="you@firm.com"
            />

            <div className="grid cols-2 mt-16">
              <div>
                <label className="field" htmlFor="signup-password">
                  Password
                </label>
                <input
                  id="signup-password"
                  className="inp"
                  type="password"
                  autoComplete="new-password"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  placeholder="At least 8 characters"
                />
              </div>
              <div>
                <label className="field" htmlFor="signup-confirm">
                  Confirm password
                </label>
                <input
                  id="signup-confirm"
                  className="inp"
                  type="password"
                  autoComplete="new-password"
                  value={confirm}
                  onChange={(e) => setConfirm(e.target.value)}
                  placeholder="Repeat password"
                />
              </div>
            </div>

            {error && <div className="toast err mt-16">{error}</div>}

            <button className="btn auth-submit mt-24" disabled={busy}>
              {busy ? "Creating account" : "Create account"}
            </button>
          </form>
        </div>
      </div>
    </div>
  );
}
