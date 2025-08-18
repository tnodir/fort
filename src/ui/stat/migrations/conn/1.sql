CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  conf_app_id INTEGER,
  path TEXT NOT NULL,
  name TEXT,
  creat_time INTEGER NOT NULL
);

CREATE UNIQUE INDEX app_path_uk ON app(path);
CREATE INDEX app_conf_app_id_idx ON app(conf_app_id);

CREATE TABLE conn(
  conn_id INTEGER PRIMARY KEY,
  app_id INTEGER NOT NULL,
  conn_time INTEGER NOT NULL,
  process_id INTEGER NOT NULL,
  reason INTEGER NOT NULL,
  blocked BOOLEAN NOT NULL,
  inherited BOOLEAN NOT NULL,
  inbound BOOLEAN NOT NULL,
  ip_proto INTEGER NOT NULL,
  local_port INTEGER NOT NULL,
  remote_port INTEGER NOT NULL,
  local_ip INTEGER,
  remote_ip INTEGER,
  local_ip6 BLOB,
  remote_ip6 BLOB,
  zone_id INTEGER,
  rule_id INTEGER
);

CREATE INDEX conn_app_id_idx ON conn(app_id);
