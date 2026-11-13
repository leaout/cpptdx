# cpptdx 中文使用与 HTTP API 说明

cpptdx 是一个 C++17 行情服务，提供通达信 HQ、扩展 ExHq 协议访问，以及 DuckDB 本地缓存。HTTP API 默认监听 `8022`。远程路由访问 TDX 服务并在 DuckDB 可用时缓存结果；`/api/local/` 路由只查本地数据，不会访问行情服务器。

## 构建与启动

Ubuntu/WSL 中安装 README 所列依赖后，在仓库根目录执行：

```bash
cmake -S . -B _build -DDUCKDB_ROOT=/path/to/libduckdb
cmake --build _build -j
./_build/tdxhq_server
```

如果没有配置 DuckDB C API，服务仍可构建，但本地存储接口不可用。运行配置见 `config/application.yml`：`server.port` 是 HTTP 端口，`tdx.hosts` 是 HQ TCP 节点（通常端口 `7709`），`exhq.hosts` 是 ExHq 节点（通常端口 `7727`），`database.path` 是 DuckDB 文件路径。远程节点必须由部署者配置为可用节点。

## 通用约定

| 参数/约定 | 说明 |
| --- | --- |
| `market` | HQ 市场：`0`=深圳，`1`=上海。未传时通常默认 `0`；证券数量接口默认 `1`。ExHq 使用服务器市场编号，先通过 `/api/exhq/markets` 查询。 |
| `cat` / `category` | HQ K 线周期编号见下表。ExHq 的 K 线周期编号同为 `0`–`11`；ExHq 报价列表中的 `category` 表示品种分类，目前支持 `2`（港股）和 `3`（期货）。 |
| `start` | 从零开始的记录偏移量。K 线和部分 ExHq 路由的历史顺序遵循各自 pytdx 方法。 |
| `count` | 请求记录数。省略时采用接口默认值；请求数会受服务端上限约束。 |
| 日期 | HQ 日期范围支持 `YYYY-MM-DD` 或 `YYYYMMDD`；ExHq 历史接口使用 `YYYYMMDD`。 |
| 本地缓存 | 远程结果会按接口支持情况写入 DuckDB。缓存未命中时，本地查询返回空数组、空结果或 `404`；它不会隐式回源。 |
| 错误响应 | 参数错误通常为 `400`；DuckDB 或客户端不可用为 `503`；本地无此记录可能为 `404`；上游数据异常可能返回空结果或 `502`。检查 HTTP 状态和响应体，不要把 `200 []` 当成已有行情数据。 |

### HQ K 线周期

| `cat` | 周期 | `cat` | 周期 |
| ---: | --- | ---: | --- |
| 0 | 5 分钟 | 6 | 月 K |
| 1 | 15 分钟 | 7 | 1 分钟 |
| 2 | 30 分钟 | 8 | 1 分钟 K |
| 3 | 1 小时 | 9 | 日 K |
| 4 | 日线 | 10 | 季 K |
| 5 | 周 K | 11 | 年 K |

实际 C++ 枚举中 `4` 是 `kDay`，`9` 是 `kDayK`，两者都保留以匹配项目与 pytdx 的既有编号。

## HQ 接口

“远程”路由访问 HQ；对应本地路由只读取已经缓存的数据。所有 JSON 数组以 `[]` 表示无记录。

| 功能 | 远程接口 | 本地接口 | 参数与说明 |
| --- | --- | --- | --- |
| 服务健康 | `GET /health` | — | 返回 `{"status":"ok"}`。 |
| 存储状态 | `GET /api/storage/status` | — | 返回 DuckDB 是否可用及后端类型。 |
| 证券总数 | `GET /api/securities/count` | `GET /api/local/securities/count` | 可选 `market`；返回 `{market,count}`。本地数量是已缓存记录数，不代表上游总数。 |
| 证券列表 | `GET /api/securities` | `GET /api/local/securities` | 可选 `market`、`start`；一次读取一个最多 1000 条的页。远程返回的页写入缓存。 |
| 个股 K 线 | `GET /api/klines` | `GET /api/local/klines` | `market`、`code`、`cat`、`start`、`count`。例如 `?market=1&code=600000&cat=9&start=0&count=100`。 |
| 指数 K 线 | `GET /api/index/klines` | `GET /api/local/index/klines` | 参数同个股 K 线，读取指数数据及独立缓存表。 |
| 日期范围日线 | `GET /api/kdata` | `GET /api/local/kdata` | `code`、`start_date`、`end_date`；本地接口另接受 `market`。例如 `?code=600000&start_date=2026-01-01&end_date=2026-11-12`。远程接口按页拉取日线并缓存。 |
| 最新快照 | `GET /api/snapshots` | `GET /api/local/snapshots` | `stocks` 为 URL 编码的 JSON 数组，每项形如 `{"market":1,"code":"600000"}`。 |
| 当日分时 | `GET /api/minute` | `GET /api/local/minute` | `market`、`code`。 |
| 历史分时 | `GET /api/history/minute` | `GET /api/local/history/minute` | `market`、`code`、`date`。 |
| 当日逐笔成交 | `GET /api/transactions` | `GET /api/local/transactions` | `market`、`code`、`start`、`count`。 |
| 历史逐笔成交 | `GET /api/history/transactions` | `GET /api/local/history/transactions` | `market`、`code`、`date`、`start`、`count`。 |
| 公司资料目录 | `GET /api/company/category` | `GET /api/local/company/category` | `market`、`code`。目录项包含名称、文件名、起始位置和长度。 |
| 公司资料内容 | `GET /api/company/content` | `GET /api/local/company/content` | `market`、`code`、`filename`、`start`、`length`；内容以 UTF-8 文本返回，单次长度最多 1 MiB。 |
| 除权除息 | `GET /api/xdxr` | `GET /api/local/xdxr` | `market`、`code`；返回该证券记录。 |
| 财务数据 | `GET /api/finance` | `GET /api/local/finance` | `market`、`code`；返回单个 JSON 对象，本地未命中时为 `404`。 |
| 板块文件元数据 | `GET /api/block/meta` | `GET /api/local/block/meta` | `blockfile`，如 `block_gn.dat`；包含文件大小与哈希。 |
| 板块文件字节范围 | `GET /api/block/content` | `GET /api/local/block/content` | `blockfile`、`start`、`size`；返回 `application/octet-stream`，范围长度最多 1,000,000 字节。 |
| 板块成分解析 | `GET /api/block/parsed` | `GET /api/local/block/parsed` | `blockfile`；远程接口分块下载、解析并缓存成分列表。 |

快照请求示例（需对 `stocks` 整个值进行 URL 编码）：

```text
GET /api/snapshots?stocks=%5B%7B%22market%22%3A1%2C%22code%22%3A%22600000%22%7D%5D
```

也可以交给 curl 编码参数：

```bash
curl --get --data-urlencode 'stocks=[{"market":1,"code":"600000"}]' \
  'http://127.0.0.1:8022/api/snapshots'
```

## ExHq 接口

ExHq 有独立连接和市场编号。先请求市场/品种目录，再使用返回的 `market` 与 `code` 访问行情。远程路由结果会缓存；本地路由是在同一路径前加 `/api/local`。

| 功能 | 远程接口 | 本地接口 | 参数与说明 |
| --- | --- | --- | --- |
| 市场目录 | `GET /api/exhq/markets` | `GET /api/local/exhq/markets` | 返回市场编号、名称和分类。 |
| 品种总数 | `GET /api/exhq/instruments/count` | `GET /api/local/exhq/instruments/count` | 返回数字；本地值只计已缓存的品种。 |
| 品种列表 | `GET /api/exhq/instruments` | `GET /api/local/exhq/instruments` | `start`、`count`；远程默认 100 条，单次最多 1000 条。 |
| 单品种报价 | `GET /api/exhq/quote` | `GET /api/local/exhq/quote` | `market`、`code`；返回盘口报价对象。 |
| 报价列表 | `GET /api/exhq/quote-list` | `GET /api/local/exhq/quote-list` | `market`、`category`、`start`、`count`；`category` 目前支持 2 或 3。 |
| K 线 | `GET /api/exhq/bars` | `GET /api/local/exhq/bars` | `market`、`code`、`category`、`start`、`count`。 |
| 日期范围 K 线 | `GET /api/exhq/bars/range` | `GET /api/local/exhq/bars/range` | `market`、`code`、`start_date`、`end_date`，日期为 `YYYYMMDD`。 |
| 当日分时 | `GET /api/exhq/minute` | `GET /api/local/exhq/minute` | `market`、`code`。 |
| 历史分时 | `GET /api/exhq/history/minute` | `GET /api/local/exhq/history/minute` | `market`、`code`、`date`。 |
| 当日逐笔 | `GET /api/exhq/transactions` | `GET /api/local/exhq/transactions` | `market`、`code`、`start`、`count`。 |
| 历史逐笔 | `GET /api/exhq/history/transactions` | `GET /api/local/exhq/history/transactions` | `market`、`code`、`date`、`start`、`count`。 |

示例：

```text
GET /api/exhq/markets
GET /api/exhq/instruments?start=0&count=20
GET /api/exhq/quote?market=42&code=IMCI
GET /api/exhq/bars?market=42&code=IMCI&category=9&start=0&count=100
```

`/api/exhq/bars` 的 `category` 使用上面的周期编号；`/api/exhq/quote-list` 的 `category` 则是报价列表类型编号，两者含义不同。

## 常见返回字段

接口成功时通常返回 JSON；板块字节范围和公司资料内容是文本或二进制。对象字段按 pytdx 常用字段名输出，下面列出调用方经常读取的部分：

| 数据类型 | 常用字段 |
| --- | --- |
| 证券列表项 | `code`、`name`、`decimal_point`、`volunit`、`pre_close` |
| 快照 | `market`、`code`、`price`、`last_close`、`open`、`high`、`low`、`vol`、`datetime` |
| HQ K 线 | `open`、`close`、`high`、`low`、`vol`、`amount`、`year`、`month`、`day`、`hour`、`minute`、`datetime` |
| HQ 分时 | `price`、`vol` |
| HQ 逐笔 | `time`、`price`、`vol`、`buyorsell` |
| ExHq 报价 | `market`、`code`、`price`、`pre_close`、`open`、`high`、`low`、`bid1`–`bid5`、`ask1`–`ask5` 及对应委托量 |
| ExHq K 线 | `open`、`high`、`low`、`close`、`position`、`trade`、`amount`、`datetime` |
| ExHq 分时 | `hour`、`minute`、`price`、`avg_price`、`volume`、`open_interest` |
| ExHq 逐笔 | `date`、`hour`、`minute`、`price`、`volume`、`direction`、`nature_name` |

示例响应（数字仅说明 JSON 结构）：

```json
[
  {
    "open": 10.2,
    "close": 10.35,
    "high": 10.4,
    "low": 10.15,
    "vol": 123456,
    "amount": 1278900,
    "datetime": "2026-11-12 00:00",
    "year": 2026,
    "month": 11,
    "day": 12,
    "hour": 0,
    "minute": 0
  }
]
```

## DuckDB 本地数据

默认数据库为 `data/cpptdx.duckdb`。数据表包括 `security_list`、`klines`、`index_klines`、`security_snapshots`、`minute_records`、`transactions`、`xdxr_records`、`finance_info`、`company_info_catalog`、`company_info_content`、`block_files`、`exhq_markets`、`exhq_instruments` 和 `exhq_data`。其他服务可直接打开同一 DuckDB 文件读取；写入期间应避免并发修改同一数据库文件。

本地接口不会自动补抓数据。需要先请求对应远程接口填充缓存。对于快照和当日行情等实时数据，缓存记录代表最近一次成功获取的数据，不是后台持续刷新；服务未实现定时同步。

## 排查建议

- `503`：确认 DuckDB 已启用，或对应 HQ/ExHq 客户端已连接；检查 `config/application.yml` 的节点与端口。
- 本地接口返回 `[]` 或 `404`：先调用对应远程接口，再查询本地缓存。空结果也可能表示上游无数据。
- HQ K 线遇到上游短包时，解析器会拒绝不完整记录并返回空数组，避免将损坏数据当成有效行情。
- URL 中的 JSON、中文文件名和特殊字符需要进行百分号编码。可以使用 `curl --get --data-urlencode` 构造查询参数。
