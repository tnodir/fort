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
  deleted BOOLEAN,  -- to reuse the rule_id
  block BOOLEAN NOT NULL,
  report BOOLEAN NOT NULL,
  log BOOLEAN NOT NULL,
  name TEXT NOT NULL,
  rule_flags INTEGER NOT NULL,  -- inbound/outbound, local port is equal to remote port, etc
  ip_proto INTEGER NOT NULL,
  local_port TEXT NOT NULL,
  remote_port TEXT NOT NULL,
  local_ip TEXT NOT NULL,
  remote_ip TEXT NOT NULL,
  local_zones INTEGER NOT NULL DEFAULT 0,  -- zone indexes bit mask
  remote_zones INTEGER NOT NULL DEFAULT 0,  -- zone indexes bit mask
  mod_time INTEGER NOT NULL,
  extra TEXT,  -- TCP flags, ICMP types, etc
  description TEXT
);

CREATE INDEX rule_deleted_idx ON rule(deleted);

CREATE TABLE policy(
  policy_id INTEGER PRIMARY KEY,
  policy_type INTEGER NOT NULL,  -- preset_lib, preset_app, global_before_app, global_after_app
  enabled BOOLEAN NOT NULL,
  deleted BOOLEAN,  -- to reuse the policy_id
  mod_time INTEGER NOT NULL,
  name TEXT,
  description TEXT
);

CREATE INDEX policy_policy_type_idx ON policy(policy_type);
CREATE INDEX policy_deleted_idx ON policy(deleted);

CREATE TABLE policy_rule(
  policy_rule_id INTEGER PRIMARY KEY,
  policy_id INTEGER NOT NULL,
  rule_id INTEGER NOT NULL,
  order_index INTEGER NOT NULL
);

CREATE INDEX policy_rule_policy_id_order_index_idx ON policy_rule(policy_id, order_index);

CREATE TABLE policy_set(
  policy_set_id INTEGER PRIMARY KEY,
  policy_id INTEGER NOT NULL,
  sub_policy_id INTEGER NOT NULL,
  order_index INTEGER NOT NULL
);

CREATE INDEX policy_set_policy_id_order_index_idx ON policy_set(policy_id, order_index);

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

CREATE INDEX policy_menu_set_policy_id_idx ON policy_menu_set(policy_id);

CREATE TABLE app_group(
  app_group_id INTEGER PRIMARY KEY,
  order_index INTEGER NOT NULL,
  enabled BOOLEAN NOT NULL,
  apply_child BOOLEAN NOT NULL DEFAULT 0,
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
