CREATE TABLE app(
  app_id INTEGER PRIMARY KEY,
  path TEXT UNIQUE NOT NULL,
  creat_time INTEGER NOT NULL,
  traf_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
);

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

CREATE TABLE conn_app(
  conn_id INTEGER PRIMARY KEY,
  app_id INTEGER NOT NULL,
  process_id INTEGER NOT NULL,
  conn_time INTEGER NOT NULL,
  inbound BOOLEAN NOT NULL,
  blocked BOOLEAN NOT NULL,
  ip_proto INTEGER NOT NULL,
  local_port INTEGER NOT NULL,
  remote_port INTEGER NOT NULL,
  local_ip INTEGER NOT NULL,
  remote_ip INTEGER NOT NULL
);

CREATE UNIQUE INDEX uk_conn_app_app_id ON conn_app(app_id);

CREATE TABLE conn_block_app(
  conn_id INTEGER PRIMARY KEY,
  block_reason INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE conn_traffic_app(
  conn_id INTEGER PRIMARY KEY,
  end_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL,
  out_bytes INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE conn_flow_app(
  conn_id INTEGER PRIMARY KEY,
  flow_id INTEGER NOT NULL
) WITHOUT ROWID;
