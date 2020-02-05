CREATE TABLE zone(
  zone_id INTEGER PRIMARY KEY,
  enabled BOOLEAN NOT NULL,
  custom_url BOOLEAN NOT NULL,
  name TEXT NOT NULL,
  source_code TEXT NOT NULL,
  url TEXT,
  form_data TEXT,
  checksum TEXT,
  last_run INTEGER,
  last_success INTEGER
);

CREATE TABLE address_group(
  addr_group_id INTEGER PRIMARY KEY,
  order_index INTEGER NOT NULL,
  include_all BOOLEAN NOT NULL,
  exclude_all BOOLEAN NOT NULL,
  include_text TEXT NOT NULL,
  exclude_text TEXT NOT NULL
);

CREATE TABLE address_group_zone(
  addr_group_id INTEGER NOT NULL,
  zone_id INTEGER NOT NULL,
  include BOOLEAN NOT NULL
);

CREATE TABLE app_group(
  app_group_id INTEGER PRIMARY KEY,
  order_index INTEGER NOT NULL,
  enabled BOOLEAN NOT NULL,
  fragment_packet BOOLEAN NOT NULL,
  period_enabled BOOLEAN NOT NULL,
  limit_in_enabled BOOLEAN NOT NULL,
  limit_out_enabled BOOLEAN NOT NULL,
  speed_limit_in INTEGER NOT NULL,
  speed_limit_out INTEGER NOT NULL,
  name TEXT NOT NULL,
  block_text TEXT NOT NULL,
  allow_text TEXT NOT NULL,
  period_from TEXT NOT NULL,
  period_to TEXT NOT NULL
);

CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  app_group_id INTEGER NOT NULL DEFAULT 0,
  path TEXT UNIQUE NOT NULL,
  name TEXT,
  use_group_perm BOOLEAN NOT NULL DEFAULT 1,
  blocked BOOLEAN NOT NULL,
  creat_time INTEGER NOT NULL,
  end_time INTEGER
);

CREATE INDEX idx_app_app_group_id ON app(app_group_id);
CREATE UNIQUE INDEX uk_app_path ON app(path);
CREATE INDEX idx_app_name ON app(name);
CREATE INDEX idx_app_end_time ON app(end_time);

CREATE TABLE app_alert(
  app_id INTEGER PRIMARY KEY
);

CREATE TABLE task(
  task_id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  enabled BOOLEAN NOT NULL,
  interval_hours INTEGER NOT NULL,
  last_run INTEGER NOT NULL,
  last_success INTEGER NOT NULL,
  data BLOB
);
