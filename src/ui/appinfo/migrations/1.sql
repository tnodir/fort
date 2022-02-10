CREATE TABLE app(
  path TEXT PRIMARY KEY,
  alt_path TEXT,
  file_descr TEXT,
  company_name TEXT,
  product_name TEXT,
  product_ver TEXT,
  file_mod_time INTEGER,
  icon_id INTEGER,
  access_time DATETIME
) WITHOUT ROWID;

CREATE INDEX app_access_time_idx ON app(access_time);

CREATE TABLE icon(
  icon_id INTEGER PRIMARY KEY,
  ref_count INTEGER NOT NULL,
  hash INTEGER NOT NULL,
  image BLOB NOT NULL
);

CREATE INDEX icon_hash_idx ON icon(hash);
