# cpptdx - TDX Stock Market Data API Server

C++17 project providing a TDX (TongDaXin) stock market data API server using cpp-httplib and Boost ASIO.

## Dependencies

```bash
apt install -y libyaml-cpp-dev libboost-all-dev libasio-dev zlib1g-dev
```

## Build

```bash
mkdir _build
cd _build
cmake ..
make
```

## Run

```bash
./tdxhq_server
```

Server starts on port 8022 by default.

## API Endpoints

- `GET /health` - Health check
- `GET /api/snapshots?stocks=[{"code":"600000","market":1},...]` - Get stock snapshots
- `GET /api/securities?market=1&start=0` - Get security list
- `GET /api/klines?cat=4&market=1&code=600000&start=0&count=100` - Get K-lines
- `GET /api/finance?market=1&code=600000` - Get finance info

## Configuration

Edit `config/application.yml` to change server port and TDX server hosts.

## Testing

```bash
cd _build && make run_tests
./tests/run_tests
```
