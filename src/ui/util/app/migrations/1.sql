PRAGMA user_version = 3;

CREATE TABLE app(
  path TEXT PRIMARY KEY,
  file_descr TEXT,
  company_name TEXT,
  product_name TEXT,
  product_ver TEXT,
  file_mod_time INTEGER,
  icon_id INTEGER,
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
