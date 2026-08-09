import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
  type ReactNode,
} from "react";
import { api, getAuthToken, setAuthToken } from "../api";
import type { AuthUser } from "../types";

interface AuthState {
  user: AuthUser | null;
  ready: boolean;
  signIn: (email: string, password: string) => Promise<void>;
  signUp: (name: string, email: string, password: string) => Promise<void>;
  signOut: () => void;
}

const AuthContext = createContext<AuthState | null>(null);

// Wraps the whole app so every route can read the signed in user and every
// panel automatically sends the bearer token once one exists. On first
// mount, a token found in localStorage is validated against /api/auth/me so
// a stale or revoked session does not silently pass as signed in.
export function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<AuthUser | null>(null);
  const [ready, setReady] = useState(false);

  useEffect(() => {
    const token = getAuthToken();
    if (!token) {
      setReady(true);
      return;
    }
    api
      .me()
      .then((res) => setUser(res.user))
      .catch(() => setAuthToken(null))
      .finally(() => setReady(true));
  }, []);

  const signIn = useCallback(async (email: string, password: string) => {
    const res = await api.login(email, password);
    setAuthToken(res.token);
    setUser(res.user);
  }, []);

  const signUp = useCallback(
    async (name: string, email: string, password: string) => {
      const res = await api.register(name, email, password);
      setAuthToken(res.token);
      setUser(res.user);
    },
    [],
  );

  const signOut = useCallback(() => {
    api.logout().catch(() => {
      // Best effort. The client side session is cleared regardless so the
      // user is never stuck signed in against their will because of a
      // transient network error.
    });
    setAuthToken(null);
    setUser(null);
  }, []);

  const value = useMemo(
    () => ({ user, ready, signIn, signUp, signOut }),
    [user, ready, signIn, signUp, signOut],
  );

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export function useAuth(): AuthState {
  const ctx = useContext(AuthContext);
  if (!ctx) {
    throw new Error("useAuth must be used inside an AuthProvider");
  }
  return ctx;
}
