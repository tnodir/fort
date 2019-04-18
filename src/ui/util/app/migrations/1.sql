PRAGMA user_version = 1;

PRAGMA journal_mode=WAL;

CREATE TABLE IF NOT EXISTS app(
  path TEXT PRIMARY KEY,
  file_descr TEXT,
  company_name TEXT,
  product_name TEXT,
  product_ver TEXT,
  icon_id INTEGER,
  access_time DATETIME DEFAULT datetime('now')
) WITHOUT ROWID;

CREATE INDEX idx_app_access_time ON app(access_time);

CREATE TABLE IF NOT EXISTS icon(
  icon_id INTEGER PRIMARY KEY,
  ref_count INTEGER NOT NULL DEFAULT 1,
  hash INTEGER NOT NULL,
  image BLOB NOT NULL
);

CREATE INDEX idx_icon_hash ON icon(hash);
