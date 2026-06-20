import { defineConfig, loadEnv } from "vite";
import react from "@vitejs/plugin-react";

// During development the Vite dev server proxies /api to the C++ backend so the
// frontend and backend can run on separate ports without CORS friction. In
// production the same backend serves the built assets directly.
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, ".", "");
  return {
    plugins: [react()],
    server: {
      port: 5173,
      proxy: {
        "/api": {
          target: env.ALPHAFORGE_API || "http://127.0.0.1:8080",
          changeOrigin: true,
        },
      },
    },
    build: {
      outDir: "dist",
      sourcemap: false,
    },
  };
});
