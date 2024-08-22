CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  path TEXT NOT NULL,
  alt_path TEXT,
  file_descr TEXT,
  company_name TEXT,
  product_name TEXT,
  product_ver TEXT,
  file_mod_time INTEGER,
  icon_id INTEGER
);

CREATE UNIQUE INDEX app_path_uk ON app(path);

CREATE TABLE icon(
  icon_id INTEGER PRIMARY KEY,
  ref_count INTEGER NOT NULL,
  hash INTEGER NOT NULL,
  image BLOB NOT NULL
);

CREATE INDEX icon_hash_idx ON icon(hash);
