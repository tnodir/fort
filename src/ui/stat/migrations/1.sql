CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  path TEXT NOT NULL,
  creat_time INTEGER NOT NULL
);

CREATE UNIQUE INDEX app_path_uk ON app(path);

CREATE TABLE traffic_app(
  app_id INTEGER PRIMARY KEY,
  traf_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE traffic_app_hour(
  app_id INTEGER NOT NULL,
  traf_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL,
  PRIMARY KEY (app_id, traf_time)
) WITHOUT ROWID;

CREATE TABLE traffic_app_day(
  app_id INTEGER NOT NULL,
  traf_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL,
  PRIMARY KEY (app_id, traf_time)
) WITHOUT ROWID;

CREATE TABLE traffic_app_month(
  app_id INTEGER NOT NULL,
  traf_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL,
  PRIMARY KEY (app_id, traf_time)
) WITHOUT ROWID;

CREATE TABLE traffic_hour(
  traf_time INTEGER PRIMARY KEY,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE traffic_day(
  traf_time INTEGER PRIMARY KEY,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE traffic_month(
  traf_time INTEGER PRIMARY KEY,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE conn(
  conn_id INTEGER PRIMARY KEY,
  app_id INTEGER NOT NULL,
  conn_time INTEGER NOT NULL,
  process_id INTEGER NOT NULL,
  inbound BOOLEAN NOT NULL,
  inherited BOOLEAN NOT NULL DEFAULT 0,
  blocked BOOLEAN NOT NULL,
  ip_proto INTEGER NOT NULL,
  local_port INTEGER NOT NULL,
  remote_port INTEGER NOT NULL,
  local_ip INTEGER NOT NULL,
  remote_ip INTEGER NOT NULL
);

CREATE INDEX conn_app_id_idx ON conn(app_id);

CREATE TABLE conn_block(
  id INTEGER PRIMARY KEY,
  conn_id INTEGER NOT NULL,
  block_reason INTEGER NOT NULL
);

CREATE UNIQUE INDEX conn_block_conn_id_uk ON conn_block(conn_id);

CREATE TABLE conn_traffic(
  id INTEGER PRIMARY KEY,
  conn_id INTEGER NOT NULL,
  end_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
);

CREATE UNIQUE INDEX conn_traffic_conn_id_uk ON conn_traffic(conn_id);

CREATE TABLE conn_flow(
  conn_id INTEGER PRIMARY KEY,
  flow_id INTEGER NOT NULL
) WITHOUT ROWID;
