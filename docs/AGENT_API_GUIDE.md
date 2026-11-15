# cpptdx Agent API Guide

本文供自动化 agent、数据采集任务和其他服务调用 cpptdx HTTP API。服务运行配置见 `config/application.yml`；默认地址是 `http://127.0.0.1:8022`。所有接口使用 `GET`，当前服务不要求登录参数。端口改变时，调用方应使用实际部署地址。

除特别说明外，行情列表响应是 JSON 数组，对象接口返回 JSON 对象；`/api/securities/count` 返回 `{market,count}`，ExHq `/instruments/count` 返回 JSON 数字。公司资料内容是纯文本，板块文件内容是二进制。

## 推荐调用流程

1. 调用 `GET /health`，确认服务可达；调用 `GET /api/storage/status`，确认 DuckDB 缓存状态。
2. 先从证券目录或 ExHq 市场目录取得代码、市场编号，再请求行情。不要仅凭代码前缀猜测 ExHq 市场。
3. 读取最新行情用远程接口；检查已缓存数据用对应 `/api/local/...` 接口。远程请求会在存储可用时写缓存，本地接口只查 DuckDB，不会回源或补齐缺页。
4. 检查 HTTP 状态和 JSON 内容。`200 []` 表示本次没有可用记录，不代表历史数据已经缓存。

`/health` 只检查 HTTP 服务进程，不验证行情节点连通性；要确认上游行情可用，应实际请求目标数据接口。

## HQ 常用接口

| 用途 | 请求 | 参数 |
| --- | --- | --- |
| 证券目录 | `/api/securities?market=1&start=0` | `market`: `0` 深圳 / `1` 上海；`start`: 页偏移，每页最多 1000 条 |
| 证券数量 | `/api/securities/count?market=1` | `market` 可省略，默认上海 |
| 个股 K 线 | `/api/klines?market=1&code=600000&cat=9&start=0&count=100` | 市场、证券代码、周期、偏移、条数 |
| 指数 K 线 | `/api/index/klines?market=1&code=000001&cat=9&start=0&count=100` | 参数同个股 K 线；使用 `/api/index/klines` 路由 |
| 日期范围日线 | `/api/kdata?code=600000&start_date=2026-01-01&end_date=2026-11-13` | 日期格式 `YYYY-MM-DD` 或 `YYYYMMDD`；按代码推断市场 |
| 批量快照 | `/api/snapshots?stocks=...` | `stocks` 是 URL 编码的 JSON 数组，每项含 `code` 和 `market` |
| 当日分时 | `/api/minute?market=1&code=600000` | 市场、代码 |
| 历史分时 | `/api/history/minute?market=1&code=600000&date=20261113` | 另需日期 `YYYYMMDD` |
| 当日逐笔 | `/api/transactions?market=1&code=600000&start=0&count=100` | `count` 最大 1800 |
| 历史逐笔 | `/api/history/transactions?market=1&code=600000&date=20261113&start=0&count=100` | 另需日期 `YYYYMMDD` |
| 除权除息 | `/api/xdxr?market=1&code=600000` | 返回该证券记录 |
| 财务数据 | `/api/finance?market=1&code=600000` | 返回单个对象 |
| 公司资料目录 | `/api/company/category?market=1&code=600000` | 返回资料文件及位置 |
| 公司资料内容 | `/api/company/content?market=1&code=600000&filename=...&start=0&length=65536` | 单次最多 1 MiB；文件名不得含路径 |
| 板块元信息 / 成分 | `/api/block/meta?blockfile=block_gn.dat`；`/api/block/parsed?blockfile=block_gn.dat` | 元数据和已解析成分为 JSON |
| 板块文件范围 | `/api/block/content?blockfile=block_gn.dat&start=0&size=65536` | 返回二进制；单次最多 1,000,000 字节 |

HQ 周期编号：`0` 5 分钟、`1` 15 分钟、`2` 30 分钟、`3` 小时、`4` 日线、`5` 周线、`6` 月线、`7` 1 分钟、`8` 1 分钟 K、`9` 日 K、`10` 季 K、`11` 年 K。K 线单次请求建议 `count <= 800`。

快照示例：

```bash
export CPPTDX_BASE_URL=http://127.0.0.1:8022
curl --get --data-urlencode 'stocks=[{"market":1,"code":"600000"},{"market":0,"code":"000001"}]' \
  "$CPPTDX_BASE_URL/api/snapshots"
```

快照响应为数组，包含 `market`、`code`、`price`、`last_close`、`open`、`high`、`low`、`vol`、`amount` 和盘口字段。当前 Round2 上游提供买卖一档；二至五档字段可能为 `0`。

K 线成功响应示例：

```json
[{"open":10.1,"close":10.4,"high":10.5,"low":10.0,"vol":1200000,
  "amount":12480000,"year":2026,"month":11,"day":13,
  "hour":0,"minute":0,"datetime":"2026-11-13 00:00"}]
```

## ExHq 常用接口

先请求 `/api/exhq/markets`，从返回结果中选择市场编号。ExHq 的 `market` 与 HQ 的 `0/1` 不同，必须使用 ExHq 目录中的编号。

| 用途 | 请求 |
| --- | --- |
| 市场目录 | `/api/exhq/markets` |
| 品种数量 / 列表 | `/api/exhq/instruments/count`；`/api/exhq/instruments?start=0&count=100` |
| 单品种报价 | `/api/exhq/quote?market={目录编号}&code={代码}` |
| 分类报价列表 | `/api/exhq/quote-list?market={目录编号}&category=2&start=0&count=80` |
| K 线 | `/api/exhq/bars?market={目录编号}&code={代码}&category=9&start=0&count=100` |
| 日期范围 K 线 | `/api/exhq/bars/range?market={目录编号}&code={代码}&start_date=20260101&end_date=20261113` |
| 分时 / 逐笔 | `/api/exhq/minute`、`/api/exhq/history/minute`、`/api/exhq/transactions`、`/api/exhq/history/transactions` |

ExHq 请求通常必须显式传 `market`；K 线周期 `category` 为 `0`–`11`，报价列表的 `category` 是分类值，目前支持 `2`（港股）和 `3`（期货）。品种列表默认 100、最多 1000 条；K 线最多 800 条；逐笔最多 1800 条。历史接口的 `date` 使用 `YYYYMMDD`。

## 本地缓存接口

可将远程路径加 `/api/local` 前缀读取 DuckDB，例如：

```text
/api/local/klines?market=1&code=600000&cat=9&start=0&count=100
/api/local/securities?market=1&start=0
/api/local/finance?market=1&code=600000
/api/local/snapshots?stocks=%5B%7B%22market%22%3A1%2C%22code%22%3A%22600000%22%7D%5D
/api/local/exhq/bars?market={目录编号}&code={代码}&category=9&start=0&count=100
```

本地缓存未命中时可能返回 `[]` 或 `404`。先请求对应远程接口填充数据，再读本地路由。远程缓存不是定时行情推送；快照只代表最近一次成功请求。

## 错误处理

| HTTP 状态 | 调用方处理 |
| --- | --- |
| `200` | 解析响应体；若数组为空，检查代码、市场、日期及上游节点，不要当成有行情 |
| `400` | 参数或格式错误；修正请求后再试，不要重复发送相同参数 |
| `404` | 请求的本地缓存记录不存在，或资源未缓存 |
| `502` | 部分接口检测到上游数据异常（例如板块文件不完整）；检查节点可用性，可切换经过验证的 HQ 节点 |
| `503` | 本地接口的 DuckDB 未启用，或 ExHq 客户端不可用；读取 `/api/storage/status` 并检查服务配置 |

部分行情上游异常会被底层 API 表示为空数组而非 HTTP 错误；必要时结合节点清单、证券目录和本地缓存状态判断。

HQ K 线和快照使用 Round2 协议。节点实测状态见 [`TDX_HOST_AVAILABILITY.md`](TDX_HOST_AVAILABILITY.md)，完整路由和数据字段说明见 [`API_CN.md`](API_CN.md)。
