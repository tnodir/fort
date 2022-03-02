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
  include_zones INTEGER NOT NULL DEFAULT 0,  -- zone indexes bit mask
  exclude_zones INTEGER NOT NULL DEFAULT 0,  -- zone indexes bit mask
  include_text TEXT NOT NULL,
  exclude_text TEXT NOT NULL
);

CREATE TABLE rule(
  rule_id INTEGER PRIMARY KEY,
  enabled BOOLEAN NOT NULL,
  block BOOLEAN NOT NULL,
  name TEXT NOT NULL,
  rule_flags INTEGER NOT NULL,  -- inbound/outbound, etc
  ip_proto INTEGER NOT NULL,
  local_port TEXT NOT NULL,
  remote_port TEXT NOT NULL,
  local_ip TEXT NOT NULL,
  remote_ip TEXT NOT NULL,
  remote_zone_id INTEGER,
  extra TEXT  -- TCP flags, ICMP types, etc
);

CREATE TABLE policy(
  policy_id INTEGER PRIMARY KEY,
  is_preset BOOLEAN NOT NULL,
  enabled BOOLEAN NOT NULL,
  block BOOLEAN NOT NULL,
  name TEXT NOT NULL
);

CREATE TABLE policy_rule(
  policy_rule_id INTEGER PRIMARY KEY,
  policy_id INTEGER NOT NULL,
  rule_id INTEGER NOT NULL,
  order_index INTEGER NOT NULL
);

CREATE TABLE policy_set(
  policy_set_id INTEGER PRIMARY KEY,
  policy_id INTEGER NOT NULL,
  sub_policy_id INTEGER NOT NULL,
  order_index INTEGER NOT NULL
);

CREATE TABLE policy_list(
  policy_list_id INTEGER PRIMARY KEY,
  type INTEGER NOT NULL,  -- preset_lib, preset_app, global_before_app, global_after_app
  policy_id INTEGER NOT NULL,
  order_index INTEGER NOT NULL
);

CREATE TABLE policy_menu(
  policy_menu_id INTEGER PRIMARY KEY,
  exclusive BOOLEAN NOT NULL,
  name TEXT NOT NULL
);

CREATE TABLE policy_menu_set(
  policy_menu_set_id INTEGER PRIMARY KEY,
  policy_menu_id INTEGER NOT NULL,
  policy_id INTEGER NOT NULL
);

CREATE TABLE app_group(
  app_group_id INTEGER PRIMARY KEY,
  order_index INTEGER NOT NULL,
  enabled BOOLEAN NOT NULL,
  apply_child BOOLEAN NOT NULL DEFAULT 0,
  log_conn BOOLEAN NOT NULL DEFAULT 1,
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
  period_to TEXT NOT NULL,
  policy_id INTEGER
);

CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  app_group_id INTEGER NOT NULL DEFAULT 0,
  path TEXT NOT NULL,
  name TEXT,
  use_group_perm BOOLEAN NOT NULL DEFAULT 1,
  apply_child BOOLEAN NOT NULL DEFAULT 0,
  blocked BOOLEAN NOT NULL,
  creat_time INTEGER NOT NULL,
  end_time INTEGER,
  policy_id INTEGER
);

CREATE INDEX app_app_group_id_idx ON app(app_group_id);
CREATE UNIQUE INDEX app_path_uk ON app(path);
CREATE INDEX app_name_idx ON app(name);
CREATE INDEX app_end_time_idx ON app(end_time);

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
