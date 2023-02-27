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
