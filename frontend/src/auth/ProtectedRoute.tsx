import type { ReactNode } from "react";
import { Navigate, useLocation } from "react-router-dom";
import { useAuth } from "./AuthContext";

// Guards a route behind a signed in session. While the initial /api/auth/me
// check is in flight, render nothing but a quiet loading state rather than
// flashing the sign in page before redirecting back.
export function ProtectedRoute({ children }: { children: ReactNode }) {
  const { user, ready } = useAuth();
  const location = useLocation();

  if (!ready) {
    return <div className="route-loading mono">Checking session...</div>;
  }
  if (!user) {
    return (
      <Navigate to="/sign-in" replace state={{ from: location.pathname }} />
    );
  }
  return <>{children}</>;
}
