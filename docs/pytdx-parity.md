# pytdx HQ API Parity

This project follows the standard `pytdx.hq.TdxHq_API` surface. Implement each API at the protocol/client layer first, then expose the same parameters and result fields over HTTP, and persist historical or reference data when applicable. Keep market IDs, category IDs, paging semantics, and field meanings aligned with pytdx.

## Standard HQ APIs

| pytdx API | Current C++ support | Next work |
| --- | --- | --- |
| `get_security_bars` | `get_security_bars`, `get_security_klines`, `/api/klines` | Implemented with pytdx bar paging and DuckDB upserts |
| `get_index_bars` | `get_index_bars`, `get_index_klines`, `/api/index/klines`, `/api/local/index/klines` | Implemented with the index-bars command and a separate DuckDB table |
| `get_security_quotes` | `get_security_quotes`, `get_security_snapshots`, `/api/snapshots` | Supports market/code pairs and JSON list form over HTTP |
| `get_security_count` | Client method, `/api/securities/count`, `/api/local/securities/count` | Remote total and count of locally cached records |
| `get_security_list` | Client method, `/api/securities`, `/api/local/securities` | Remote pages are cached by server offset in DuckDB |
| `get_minute_time_data` | Client method, `/api/minute`, `/api/local/minute` | Implemented with ordered DuckDB rows |
| `get_history_minute_time_data` | Client method, `/api/history/minute`, `/api/local/history/minute` | Implemented with date-keyed DuckDB rows |
| `get_transaction_data` | Client method, `/api/transactions`, `/api/local/transactions` | Implemented with paged DuckDB rows |
| `get_history_transaction_data` | Client method, `/api/history/transactions`, `/api/local/history/transactions` | Implemented with date-keyed DuckDB rows |
| `get_company_info_category` | Client method and remote/local company category routes | Implemented and cached in DuckDB |
| `get_company_info_content` | Client method and remote/local company content routes | Implemented with bounded length, filename validation, and cached ranges |
| `get_xdxr_info` | Client method, `/api/xdxr`, `/api/local/xdxr` | Implemented with date/category-keyed DuckDB storage |
| `get_finance_info` | Client method, `/api/finance`, `/api/local/finance` | Corrected pytdx field names and cached JSON records |
| `get_block_info_meta` | Client method, `/api/block/meta`, `/api/local/block/meta` | Fetch block-file size/hash; local metadata is cached by the parsed-file download |
| `get_block_info` | Client method, `/api/block/content`, `/api/local/block/content` | Fetch raw byte ranges from the block file |
| `get_and_parse_block_info` | Client method, `/api/block/parsed`, `/api/local/block/parsed` | Downloads in bounded chunks, parses flat constituents, caches the file in DuckDB |

`get_k_data` is implemented as a multi-page daily-bar convenience method. `/api/kdata` downloads the date range and caches those bars; `/api/local/kdata` serves the same date range from DuckDB.

## Extended ExHq APIs

ExHq uses a separately configured TCP session (`exhq.hosts`) and its own 92-byte setup handshake. `get_markets`, `get_instrument_count`, `get_instrument_info`, `get_instrument_quote`, `get_instrument_quote_list`, `get_minute_time_data`, `get_history_minute_time_data`, `get_instrument_bars`, `get_history_instrument_bars_range`, `get_transaction_data`, and `get_history_transaction_data` have protocol/client methods and matching HTTP routes. Metadata and fetched records are cached in DuckDB; local APIs mirror each remote route under `/api/local/exhq/`. Quote-list parsing follows pytdx's supported categories (Hong Kong and futures).

## Local Data Store

Keep protocol access independent from persistence. Route fetched results through a storage interface with idempotent upserts and keys that include market, code, category/data type, and source timestamp or date. DuckDB is the initial single-file analytical store, accessed through its C API. `security_list`, `klines`, `index_klines`, `security_snapshots`, `minute_records`, `transactions`, `xdxr_records`, `finance_info`, `company_info_catalog`, `company_info_content`, `block_files`, `exhq_markets`, `exhq_instruments`, and `exhq_data` are persisted; other local services can query the same file directly.

## Maintenance Notes

Keep protocol request layouts, output field names, and paging behavior aligned with pytdx. New remote fetch handlers should upsert results and have a matching local read route where their results are cacheable. Treat scheduled synchronization and retention policies as separate deployment features; no background refresh is started by the server.
