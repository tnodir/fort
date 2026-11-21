CREATE TABLE proc(
  pid INTEGER PRIMARY KEY,
  app_id INTEGER, -- the Program, NULL when unknown
  parent_pid INTEGER, -- NULL when unknown
  path TEXT NOT NULL,
  parent_path TEXT -- NULL when unknown
);

CREATE TABLE flow(
  flow_id INTEGER PRIMARY KEY,
  pid INTEGER NOT NULL,
  inbound BOOLEAN NOT NULL,
  ip_proto INTEGER NOT NULL,
  local_port INTEGER NOT NULL,
  remote_port INTEGER NOT NULL,
  local_ip INTEGER,
  remote_ip INTEGER,
  local_ip6 BLOB,
  remote_ip6 BLOB,
  remote_host TEXT,
  conn_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL DEFAULT 0,
  out_bytes INTEGER NOT NULL DEFAULT 0,
  in_speed INTEGER NOT NULL DEFAULT 0, -- bytes per second
  out_speed INTEGER NOT NULL DEFAULT 0, -- bytes per second
  updated_tick INTEGER NOT NULL DEFAULT 0 -- the driver's log timer tick of the last bytes
);

CREATE INDEX flow_pid_idx ON flow(pid);
