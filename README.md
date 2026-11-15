# cpptdx - TDX Stock Market Data API Server

中文使用说明与完整 HTTP 接口参数见 [中文文档](docs/API_CN.md)；供自动化 agent 调用的快速契约见 [Agent API Guide](docs/AGENT_API_GUIDE.md)。

C++17 project providing a TDX (TongDaXin) stock market data API server using cpp-httplib and Boost ASIO.

## Dependencies

```bash
apt install -y libyaml-cpp-dev libboost-all-dev libasio-dev zlib1g-dev
```

DuckDB local persistence uses the DuckDB C API. Install the `libduckdb` package and configure with `-DDUCKDB_ROOT=/path/to/libduckdb`; without it the server builds but persistence is disabled. See [DuckDB installation packages](https://duckdb.org/docs/2.0/installation).

## Build

```bash
mkdir _build
cd _build
cmake .. -DDUCKDB_ROOT=/path/to/libduckdb
make
```

Fetched security pages, bars, snapshots, intraday records, corporate actions, finance records, company information, block files, and ExHq metadata/market records are upserted into `data/cpptdx.duckdb` by default. Set `database.path` in `config/application.yml` to change the file. Other local services can open the same DuckDB file and query `security_list`, `klines`, `index_klines`, `security_snapshots`, `minute_records`, `transactions`, `xdxr_records`, `finance_info`, `company_info_catalog`, `company_info_content`, `block_files`, `exhq_markets`, `exhq_instruments`, and `exhq_data`.

## Run

```bash
./tdxhq_server
```

Server starts on port 8022 by default.

## API Endpoints

- `GET /health` - Health check
- `GET /api/securities/count?market=1` - Get the security count (`market`: 0=SZ, 1=SH)
- `GET /api/local/securities/count?market=1` - Count locally cached security records
- `GET /api/storage/status` - Check local storage availability
- `GET /api/local/klines?cat=4&market=1&code=600000&start=0&count=100` - Read cached K-lines (newest first)
- `GET /api/local/index/klines?cat=9&market=1&code=000001&start=0&count=100` - Read cached index K-lines
- `GET /api/local/kdata?market=1&code=600000&start_date=2026-01-01&end_date=2026-11-11` - Read cached daily bars within a date range
- `GET /api/snapshots?stocks=[{"code":"600000","market":1},...]` - Get stock snapshots
- `GET /api/local/snapshots?stocks=[{"code":"600000","market":1},...]` - Read cached latest stock snapshots
- `GET /api/securities?market=1&start=0` - Get one raw TDX security-list page (`start` is the server offset)
- `GET /api/local/securities?market=1&start=0` - Read one locally cached 1000-record security page
- `GET /api/klines?cat=4&market=1&code=600000&start=0&count=100` - Get K-lines
- `GET /api/index/klines?cat=9&market=1&code=000001&start=0&count=100` - Get index K-lines
- `GET /api/kdata?code=600000&start_date=2026-01-01&end_date=2026-11-11` - Download daily bars within a date range and cache them
- `GET /api/minute?market=1&code=600000` - Get current-day minute prices
- `GET /api/history/minute?market=1&code=600000&date=20261111` - Get minute prices for a date
- `GET /api/transactions?market=1&code=600000&start=0&count=100` - Get current-day transactions
- `GET /api/history/transactions?market=1&code=600000&date=20261110&start=0&count=100` - Get transactions for a date
- `GET /api/company/category?market=1&code=600000` - Get company-info file catalog
- `GET /api/company/content?market=1&code=600000&filename=600000.txt&start=0&length=1000` - Read company-info text range
- `GET /api/local/company/category?market=1&code=600000` - Read cached company-info catalog
- `GET /api/local/company/content?market=1&code=600000&filename=600000.txt&start=0&length=1000` - Read cached text range
- `GET /api/xdxr?market=1&code=600000` - Get and cache corporate actions
- `GET /api/local/xdxr?market=1&code=600000` - Read cached corporate actions
- `GET /api/local/minute?market=1&code=600000` - Read cached current-day minute data
- `GET /api/local/history/minute?market=1&code=600000&date=20261110` - Read cached historical minute data
- `GET /api/local/transactions?market=1&code=600000&start=0&count=100` - Read cached current-day transactions
- `GET /api/local/history/transactions?market=1&code=600000&date=20261110&start=0&count=100` - Read cached historical transactions
- `GET /api/finance?market=1&code=600000` - Get finance info
- `GET /api/local/finance?market=1&code=600000` - Read cached finance info
- `GET /api/exhq/markets` - Read ExHq market catalog
- `GET /api/exhq/instruments/count` - Read ExHq instrument count
- `GET /api/exhq/instruments?start=0&count=100` - Read one ExHq instrument page
- `GET /api/local/exhq/markets` - Read cached ExHq market metadata
- `GET /api/local/exhq/instruments/count` - Count locally cached ExHq instruments
- `GET /api/local/exhq/instruments?start=0&count=100` - Read cached ExHq instrument pages
- `GET /api/exhq/quote?market=47&code=IF1709` - Read one ExHq five-level quote
- `GET /api/exhq/quote-list?market=29&category=3&start=0&count=80` - Read a futures or Hong Kong quote page
- `GET /api/exhq/bars?category=4&market=47&code=IFL0&start=0&count=100` - Read ExHq bars
- `GET /api/exhq/bars/range?market=74&code=BABA&start_date=20261001&end_date=20261031` - Read date-range ExHq bars
- `GET /api/exhq/minute?market=47&code=IFL0` and `/api/exhq/history/minute?market=47&code=IFL0&date=20261110` - Read ExHq minute bars
- `GET /api/exhq/transactions?market=47&code=IFL0&start=0&count=100` and `/api/exhq/history/transactions?market=47&code=IFL0&date=20261110&start=0&count=100` - Read ExHq transactions
- Local ExHq cache routes mirror these paths under `/api/local/exhq/`.
- `GET /api/block/meta?blockfile=block_gn.dat` - Read HQ block file metadata
- `GET /api/block/content?blockfile=block_gn.dat&start=0&size=30000` - Read a raw HQ block file range
- `GET /api/block/parsed?blockfile=block_gn.dat` - Download, parse, and cache HQ block constituents
- `GET /api/local/block/meta?blockfile=block_gn.dat`, `/api/local/block/content?...`, and `/api/local/block/parsed?blockfile=block_gn.dat` - Read cached HQ block file data

The ExHq client uses an independent server list and session. Add approved ExHq endpoints under `exhq.hosts` in `config/application.yml` (typically port `7727`). The ExHq interface mirrors the pytdx quote, quote-list, bar, minute, history, and transaction methods. Quote-list categories 2 and 3 (Hong Kong and futures) follow pytdx parser support.

## Configuration

Edit `config/application.yml` to change server port and TDX server hosts.

## Testing

```bash
cd _build && make run_tests
./tests/run_tests
```
