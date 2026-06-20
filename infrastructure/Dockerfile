# Multi-stage build: fetch data, compile the frontend, compile the backend, then
# ship a small runtime image that serves both.

# Real market data from Yahoo Finance. Needs network access to Yahoo during the
# build. If your build network blocks it, fetch on the host instead and mount
# the data directory into the runtime image.
FROM python:3.12-slim AS data
WORKDIR /src
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt
COPY scripts/fetch_data.py scripts/fetch_data.py
RUN mkdir -p code/backend/data && \
    python scripts/fetch_data.py --period 3y --out code/backend/data || \
    echo "WARNING: data fetch failed at build time; mount data at runtime."

FROM node:20-slim AS web
WORKDIR /web
COPY frontend/package.json frontend/package-lock.json* ./
RUN npm install
COPY frontend/ ./
RUN npm run build

FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y g++ cmake make && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY code/backend/ code/backend/
RUN cmake -S code/backend -B code/backend/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build code/backend/build -j

FROM ubuntu:24.04 AS runtime
RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /src/code/backend/build/alphaforge_server ./alphaforge_server
COPY --from=data /src/code/backend/data ./data
COPY --from=web /web/dist ./web
EXPOSE 8080
CMD ["./alphaforge_server", "8080", "data", "web"]
