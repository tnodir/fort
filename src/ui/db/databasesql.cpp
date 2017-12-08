#include "databasesql.h"

const char * const DatabaseSql::sqlPragmas =
        "PRAGMA locking_mode=EXCLUSIVE;"
        "PRAGMA journal_mode=WAL;"
        "PRAGMA synchronous=NORMAL;"
        ;

const char * const DatabaseSql::sqlCreateTables =
        "CREATE TABLE app("
        "  id INTEGER PRIMARY KEY,"
        "  path TEXT UNIQUE NOT NULL,"
        "  creat_time INTEGER NOT NULL,"
        "  traf_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ");"

        "CREATE TABLE traffic_app_hour("
        "  app_id INTEGER NOT NULL,"
        "  traf_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL,"
        "  PRIMARY KEY (app_id, traf_time)"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_app_day("
        "  app_id INTEGER NOT NULL,"
        "  traf_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL,"
        "  PRIMARY KEY (app_id, traf_time)"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_app_month("
        "  app_id INTEGER NOT NULL,"
        "  traf_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL,"
        "  PRIMARY KEY (app_id, traf_time)"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_hour("
        "  traf_time INTEGER PRIMARY KEY,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_day("
        "  traf_time INTEGER PRIMARY KEY,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_month("
        "  traf_time INTEGER PRIMARY KEY,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ") WITHOUT ROWID;"
        ;

const char * const DatabaseSql::sqlSelectAppId =
        "SELECT id FROM app WHERE path = ?1;"
        ;

const char * const DatabaseSql::sqlInsertAppId =
        "INSERT INTO app(path, creat_time, traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, ?2, ?3, 0, 0);"
        ;

const char * const DatabaseSql::sqlSelectAppPaths =
        "SELECT id, path FROM app ORDER BY creat_time;"
        ;

const char * const DatabaseSql::sqlInsertTrafficAppHour =
        "INSERT INTO traffic_app_hour(app_id, traf_time, in_bytes, out_bytes)"
        "  VALUES(?2, ?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficAppDay =
        "INSERT INTO traffic_app_day(app_id, traf_time, in_bytes, out_bytes)"
        "  VALUES(?2, ?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficAppMonth =
        "INSERT INTO traffic_app_month(app_id, traf_time, in_bytes, out_bytes)"
        "  VALUES(?2, ?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficHour =
        "INSERT INTO traffic_hour(traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficDay =
        "INSERT INTO traffic_day(traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficMonth =
        "INSERT INTO traffic_month(traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppHour =
        "UPDATE traffic_app_hour"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE app_id = ?4 and traf_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppDay =
        "UPDATE traffic_app_day"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE app_id = ?4 and traf_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppMonth =
        "UPDATE traffic_app_month"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE app_id = ?4 and traf_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficHour =
        "UPDATE traffic_hour"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE traf_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficDay =
        "UPDATE traffic_day"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE traf_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficMonth =
        "UPDATE traffic_month"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE traf_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppTotal =
        "UPDATE app"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE id = ?1;"
        ;

const char * const DatabaseSql::sqlSelectMinTrafAppHour =
        "SELECT min(traf_time) FROM traffic_app_hour"
        "  WHERE app_id = ?1;"
        ;

const char * const DatabaseSql::sqlSelectMinTrafAppDay =
        "SELECT min(traf_time) FROM traffic_app_day"
        "  WHERE app_id = ?1;"
        ;

const char * const DatabaseSql::sqlSelectMinTrafAppMonth =
        "SELECT min(traf_time) FROM traffic_app_month"
        "  WHERE app_id = ?1;"
        ;

const char * const DatabaseSql::sqlSelectMinTrafHour =
        "SELECT min(traf_time) FROM traffic_hour;"
        ;

const char * const DatabaseSql::sqlSelectMinTrafDay =
        "SELECT min(traf_time) FROM traffic_app_day;"
        ;

const char * const DatabaseSql::sqlSelectMinTrafMonth =
        "SELECT min(traf_time) FROM traffic_app_month;"
        ;

const char * const DatabaseSql::sqlSelectTrafAppHour =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM traffic_app_hour"
        "  WHERE app_id = ?1 and traf_time between ?2 and ?3;"
        ;

const char * const DatabaseSql::sqlSelectTrafAppDay =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM traffic_app_day"
        "  WHERE app_id = ?1 and traf_time between ?2 and ?3;"
        ;

const char * const DatabaseSql::sqlSelectTrafAppMonth =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM traffic_app_month"
        "  WHERE app_id = ?1 and traf_time between ?2 and ?3;"
        ;

const char * const DatabaseSql::sqlSelectTrafAppTotal =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM app WHERE app_id = ?1;"
        ;

const char * const DatabaseSql::sqlSelectTrafHour =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM traffic_hour"
        "  WHERE traf_time between ?2 and ?3;"
        ;

const char * const DatabaseSql::sqlSelectTrafDay =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM traffic_day"
        "  WHERE traf_time between ?2 and ?3;"
        ;

const char * const DatabaseSql::sqlSelectTrafMonth =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM traffic_month"
        "  WHERE traf_time between ?2 and ?3;"
        ;

const char * const DatabaseSql::sqlSelectTrafTotal =
        "SELECT traf_time, in_bytes, out_bytes"
        "  FROM app;"
        ;
