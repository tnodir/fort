PRAGMA user_version = 1;

PRAGMA journal_mode = WAL;

CREATE TABLE IF NOT EXISTS address_group(
  addr_group_id INTEGER PRIMARY KEY,
  order INTEGER NOT NULL,
  include_all BOOLEAN NOT NULL,
  exclude_all BOOLEAN NOT NULL,
  include_text TEXT NOT NULL,
  exclude_text TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS app_group(
  app_group_id INTEGER PRIMARY KEY,
  order INTEGER NOT NULL,
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

CREATE TABLE IF NOT EXISTS app(
  app_id INTEGER PRIMARY KEY,
  path TEXT UNIQUE NOT NULL,
  blocked BOOLEAN NOT NULL,
  creat_time INTEGER NOT NULL,
  end_time INTEGER
);

CREATE INDEX idx_app_end_time ON app(end_time);

CREATE TABLE IF NOT EXISTS app_alert(
  app_id INTEGER PRIMARY KEY
);
