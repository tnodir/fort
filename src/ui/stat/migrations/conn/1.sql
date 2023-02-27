CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  path TEXT NOT NULL,
  creat_time INTEGER NOT NULL
);

CREATE UNIQUE INDEX app_path_uk ON app(path);

CREATE TABLE conn_traffic(
  conn_id INTEGER PRIMARY KEY,
  app_id INTEGER NOT NULL,
  conn_time INTEGER NOT NULL,
  process_id INTEGER NOT NULL,
  inbound BOOLEAN NOT NULL,
  inherited BOOLEAN NOT NULL,
  ip_proto INTEGER NOT NULL,
  local_port INTEGER NOT NULL,
  remote_port INTEGER NOT NULL,
  local_ip INTEGER,
  remote_ip INTEGER,
  local_ip6 BLOB,
  remote_ip6 BLOB,
  --
  end_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
);

CREATE INDEX conn_traffic_app_id_idx ON conn_traffic(app_id);

CREATE TABLE conn_flow(
  conn_id INTEGER PRIMARY KEY,
  flow_id INTEGER NOT NULL
) WITHOUT ROWID;
