# Databases

Moved out of `AGENTS.md`: read it when working on this area.

## SQLite wrapper and migrations

No Qt SQL — a hand-rolled SQLite wrapper in `src/ui/3rdparty/sqlite/` (`SqliteDb`, `SqliteStmt`, `DbQuery`) over the amalgamation in `src/3rdparty/sqlite/`. Separate DBs for conf, traffic stats, connections, the app-info cache and the Hosts Zones' names; schema versions are applied from `.sql` migrations embedded as Qt resources (`ui/conf/migrations/`, `ui/stat/migrations/*/`, `ui/appinfo/migrations/`, `ui/dns/migrations/`).

## Transactions

A transaction is begun and ended by `SqliteDb`'s functions (or `SqliteUtilBase`'s), never by the SQL text (`"BEGIN;"`, `"COMMIT;"`): a reading one by `beginTransaction()`, a writing one always by `beginWriteTransaction()` (`BEGIN IMMEDIATE`: a deferred one, upgraded to a write while another connection writes, fails at once with `SQLITE_BUSY`), ended by `commitTransaction()` or `endTransaction(ok)`.
