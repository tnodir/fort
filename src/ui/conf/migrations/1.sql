CREATE TABLE zone(
  zone_id INTEGER PRIMARY KEY,
  enabled BOOLEAN NOT NULL,
  custom_url BOOLEAN NOT NULL,
  name TEXT NOT NULL,
  source_code TEXT NOT NULL,
  url TEXT,
  form_data TEXT,
  address_count INTEGER,
  text_inline TEXT,
  text_checksum TEXT,
  bin_checksum TEXT,
  source_modtime INTEGER,
  last_run INTEGER,
  last_success INTEGER
);

CREATE TABLE address_group(
  addr_group_id INTEGER PRIMARY KEY,
  order_index INTEGER NOT NULL,
  include_all BOOLEAN NOT NULL,
  exclude_all BOOLEAN NOT NULL,
  include_zones INTEGER NOT NULL DEFAULT 0,  -- zone ids bit mask
  exclude_zones INTEGER NOT NULL DEFAULT 0,  -- zone ids bit mask
  include_text TEXT NOT NULL,
  exclude_text TEXT NOT NULL
);

CREATE TABLE app_group(
  app_group_id INTEGER PRIMARY KEY,
  order_index INTEGER NOT NULL,
  enabled BOOLEAN NOT NULL,
  apply_child BOOLEAN NOT NULL DEFAULT 0,
  lan_only BOOLEAN NOT NULL DEFAULT 0,
  log_blocked BOOLEAN NOT NULL DEFAULT 1,
  log_conn BOOLEAN NOT NULL DEFAULT 1,
  period_enabled BOOLEAN NOT NULL,
  limit_in_enabled BOOLEAN NOT NULL,
  limit_out_enabled BOOLEAN NOT NULL,
  speed_limit_in INTEGER NOT NULL,
  speed_limit_out INTEGER NOT NULL,
  limit_packet_loss INTEGER NOT NULL DEFAULT 0,
  limit_latency INTEGER NOT NULL DEFAULT 0,
  limit_bufsize_in INTEGER NOT NULL DEFAULT 150000,
  limit_bufsize_out INTEGER NOT NULL DEFAULT 150000,
  name TEXT NOT NULL,
  kill_text TEXT,
  block_text TEXT NOT NULL,
  allow_text TEXT NOT NULL,
  period_from TEXT NOT NULL,
  period_to TEXT NOT NULL
);

CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  app_group_id INTEGER NOT NULL DEFAULT 0,
  origin_path TEXT,
  path TEXT,
  name TEXT,
  notes TEXT,
  is_wildcard BOOLEAN NOT NULL DEFAULT 0,
  use_group_perm BOOLEAN NOT NULL DEFAULT 1,
  apply_child BOOLEAN NOT NULL DEFAULT 0,
  kill_child BOOLEAN NOT NULL DEFAULT 0,
  lan_only BOOLEAN NOT NULL DEFAULT 0,
  parked BOOLEAN NOT NULL DEFAULT 0,
  log_blocked BOOLEAN NOT NULL DEFAULT 1,
  log_conn BOOLEAN NOT NULL DEFAULT 1,
  blocked BOOLEAN NOT NULL,
  kill_process BOOLEAN NOT NULL DEFAULT 0,
  accept_zones INTEGER NOT NULL DEFAULT 0,  -- zone ids bit mask
  reject_zones INTEGER NOT NULL DEFAULT 0,  -- zone ids bit mask
  rule_id INTEGER,
  creat_time INTEGER NOT NULL,
  end_action INTEGER NOT NULL DEFAULT 0,
  end_time INTEGER
);

CREATE INDEX app_app_group_id_idx ON app(app_group_id);
CREATE UNIQUE INDEX app_path_uk ON app(path);
CREATE INDEX app_name_idx ON app(name);
CREATE INDEX app_rule_idx ON app(rule_id);
CREATE INDEX app_end_time_idx ON app(end_time);

CREATE TABLE app_alert(
  app_id INTEGER PRIMARY KEY
);

CREATE TABLE rule(
  rule_id INTEGER PRIMARY KEY,
  enabled BOOLEAN NOT NULL,
  blocked BOOLEAN NOT NULL,
  exclusive BOOLEAN NOT NULL,
  name TEXT NOT NULL,
  notes TEXT,
  rule_text TEXT NOT NULL,
  rule_type INTEGER NOT NULL, -- app rules, global before/after apps, preset rules
  accept_zones INTEGER NOT NULL DEFAULT 0,  -- zone ids bit mask
  reject_zones INTEGER NOT NULL DEFAULT 0,  -- zone ids bit mask
  mod_time INTEGER NOT NULL
);

CREATE INDEX rule_rule_type_name_idx ON rule(rule_type, lower(name));

CREATE TABLE rule_set(
  rule_set_id INTEGER PRIMARY KEY,
  rule_id INTEGER NOT NULL,
  sub_rule_id INTEGER NOT NULL,
  order_index INTEGER NOT NULL
);

CREATE INDEX rule_set_rule_id_idx ON rule_set(rule_id);
CREATE INDEX rule_set_sub_rule_id_idx ON rule_set(sub_rule_id);

CREATE TABLE task(
  task_id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  enabled BOOLEAN NOT NULL,
  run_on_startup BOOLEAN NOT NULL DEFAULT 0,
  interval_hours INTEGER NOT NULL,
  last_run INTEGER NOT NULL,
  last_success INTEGER NOT NULL,
  data BLOB
);
