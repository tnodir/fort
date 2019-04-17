PRAGMA user_version = 1;

PRAGMA journal_mode=WAL;

CREATE TABLE IF NOT EXISTS app(
  path TEXT PRIMARY KEY,
  file_descr TEXT,
  company_name TEXT,
  product_name TEXT,
  product_ver TEXT,
  icon_id INTEGER,
  persist BOOL DEFAULT 0,
  access_time DATE DEFAULT date('now')
) WITHOUT ROWID;

CREATE TABLE IF NOT EXISTS icon(
  icon_id INTEGER PRIMARY KEY,
  ref_count INTEGER NOT NULL DEFAULT 0,
  hash TEXT NOT NULL,
  icon BLOB NOT NULL
);
