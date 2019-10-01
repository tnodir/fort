PRAGMA user_version = 3;

PRAGMA journal_mode = WAL;

CREATE TABLE app(
  path TEXT PRIMARY KEY,
  file_descr TEXT,
  company_name TEXT,
  product_name TEXT,
  product_ver TEXT,
  icon_id INTEGER,
  file_mod_time DATETIME,
  access_time DATETIME
) WITHOUT ROWID;

CREATE INDEX idx_app_access_time ON app(access_time);

CREATE TABLE icon(
  icon_id INTEGER PRIMARY KEY,
  ref_count INTEGER NOT NULL,
  hash INTEGER NOT NULL,
  image BLOB NOT NULL
);

CREATE INDEX idx_icon_hash ON icon(hash);
