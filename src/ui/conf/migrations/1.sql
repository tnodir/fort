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
  last_success INTEGER,
  mod_time INTEGER
);

CREATE TABLE address_group(
  addr_group_id INTEGER PRIMARY KEY,
  include_all BOOLEAN NOT NULL,
  exclude_all BOOLEAN NOT NULL,
  include_zones INTEGER NOT NULL DEFAULT 0, -- zone ids bit mask
  exclude_zones INTEGER NOT NULL DEFAULT 0, -- zone ids bit mask
  include_text TEXT NOT NULL,
  exclude_text TEXT NOT NULL
);

CREATE TABLE speed_limit(
  limit_id INTEGER PRIMARY KEY,
  enabled BOOLEAN NOT NULL,
  inbound BOOLEAN NOT NULL,
  name TEXT NOT NULL,
  kbps INTEGER NOT NULL, -- kilobits per second
  packet_loss INTEGER NOT NULL, -- percent
  latency INTEGER NOT NULL, -- milliseconds
  bufsize INTEGER NOT NULL,
  mod_time INTEGER NOT NULL
);

CREATE TABLE app_group(
  app_group_id INTEGER PRIMARY KEY,
  enabled BOOLEAN NOT NULL,
  period_enabled BOOLEAN NOT NULL,
  name TEXT NOT NULL,
  notes TEXT,
  period_from TEXT,
  period_to TEXT,
  mod_time INTEGER NOT NULL
);

CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  app_group_id INTEGER NOT NULL DEFAULT 0,
  origin_path TEXT,
  path TEXT,
  name TEXT,
  notes TEXT,
  is_wildcard BOOLEAN NOT NULL DEFAULT 0,
  apply_parent BOOLEAN NOT NULL DEFAULT 0,
  apply_child BOOLEAN NOT NULL DEFAULT 0,
  apply_spec_child BOOLEAN NOT NULL DEFAULT 0,
  kill_child BOOLEAN NOT NULL DEFAULT 0,
  lan_only BOOLEAN NOT NULL DEFAULT 0,
  parked BOOLEAN NOT NULL DEFAULT 0,
  log_allowed_conn BOOLEAN NOT NULL DEFAULT 1,
  log_blocked_conn BOOLEAN NOT NULL DEFAULT 1,
  blocked BOOLEAN NOT NULL,
  kill_process BOOLEAN NOT NULL DEFAULT 0,
  accept_zones INTEGER NOT NULL DEFAULT 0, -- zone ids bit mask
  reject_zones INTEGER NOT NULL DEFAULT 0, -- zone ids bit mask
  rule_id INTEGER,
  in_limit_id INTEGER,
  out_limit_id INTEGER,
  creat_time INTEGER NOT NULL,
  end_action INTEGER NOT NULL DEFAULT 0,
  end_time INTEGER
);

CREATE INDEX app_app_group_id_idx ON app(app_group_id);
CREATE UNIQUE INDEX app_path_uk ON app(path);
CREATE INDEX app_name_idx ON app(lower(name));
CREATE INDEX app_rule_id_idx ON app(rule_id);
CREATE INDEX app_in_limit_id_idx ON app(in_limit_id);
CREATE INDEX app_out_limit_id_idx ON app(out_limit_id);
CREATE INDEX app_end_time_idx ON app(end_time);

CREATE TABLE app_alert(
  app_id INTEGER PRIMARY KEY
);

CREATE TABLE rule(
  rule_id INTEGER PRIMARY KEY,
  enabled BOOLEAN NOT NULL,
  blocked BOOLEAN NOT NULL,
  exclusive BOOLEAN NOT NULL,
  terminate BOOLEAN NOT NULL DEFAULT 0,
  term_blocked BOOLEAN NOT NULL DEFAULT 1,
  name TEXT NOT NULL,
  notes TEXT,
  rule_text TEXT NOT NULL,
  rule_type INTEGER NOT NULL, -- app rules, global before/after apps, preset rules
  accept_zones INTEGER NOT NULL DEFAULT 0, -- zone ids bit mask
  reject_zones INTEGER NOT NULL DEFAULT 0, -- zone ids bit mask
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
  delay_startup BOOLEAN NOT NULL DEFAULT 0,
  max_retries INTEGER NOT NULL DEFAULT 0,
  retry_seconds INTEGER NOT NULL DEFAULT 0,
  interval_hours INTEGER NOT NULL,
  last_run INTEGER NOT NULL,
  last_success INTEGER NOT NULL,
  data BLOB
);
