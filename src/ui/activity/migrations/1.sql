CREATE TABLE proc(
  pid INTEGER PRIMARY KEY,
  app_id INTEGER, -- the Program by the flows' CONN, NULL when unknown
  parent_pid INTEGER, -- NULL when unknown
  create_time INTEGER, -- FILETIME, NULL when unknown
  path TEXT NOT NULL,
  parent_path TEXT -- by the parent's row at the process's start, NULL when unknown
);

CREATE TABLE flow(
  flow_id INTEGER PRIMARY KEY,
  pid INTEGER NOT NULL,
  terminated BOOLEAN NOT NULL, -- by the Terminating Rule
  inherited BOOLEAN NOT NULL,
  inbound BOOLEAN NOT NULL,
  loopback BOOLEAN NOT NULL,
  local_net BOOLEAN NOT NULL, -- the remote address is of the LAN addresses
  reason INTEGER NOT NULL, -- FORT_CONN_REASON_*, 0 when unknown
  ip_proto INTEGER NOT NULL,
  zone_id INTEGER, -- NULL when none
  rule_id INTEGER, -- NULL when none
  local_port INTEGER NOT NULL,
  remote_port INTEGER NOT NULL,
  local_ip INTEGER,
  remote_ip INTEGER,
  local_ip6 BLOB,
  remote_ip6 BLOB,
  remote_host TEXT,
  inherit_path TEXT, -- the path of the process's inherited Program, NULL when unknown
  conn_time INTEGER NOT NULL,
  in_bytes INTEGER NOT NULL DEFAULT 0,
  out_bytes INTEGER NOT NULL DEFAULT 0,
  in_speed INTEGER NOT NULL DEFAULT 0, -- bytes per second
  out_speed INTEGER NOT NULL DEFAULT 0, -- bytes per second
  updated_tick INTEGER NOT NULL DEFAULT 0 -- the driver's log timer tick of the last bytes
);

CREATE INDEX flow_pid_idx ON flow(pid);
