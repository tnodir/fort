CREATE TABLE host(
  name TEXT PRIMARY KEY,
  name_mask INTEGER NOT NULL,
  sub_mask INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE host_pattern(
  name TEXT PRIMARY KEY,
  zones_mask INTEGER NOT NULL
) WITHOUT ROWID;

CREATE TABLE host_zone(
  zone_id INTEGER PRIMARY KEY,
  bin_checksum TEXT NOT NULL,
  subdomains BOOLEAN NOT NULL
);
