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
        "  unix_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ");"

        "CREATE TABLE traffic_app_hour("
        "  app_id INTEGER NOT NULL,"
        "  unix_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL,"
        "  PRIMARY KEY (app_id, unix_time)"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_app_day("
        "  app_id INTEGER NOT NULL,"
        "  unix_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL,"
        "  PRIMARY KEY (app_id, unix_time)"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_app_month("
        "  app_id INTEGER NOT NULL,"
        "  unix_time INTEGER NOT NULL,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL,"
        "  PRIMARY KEY (app_id, unix_time)"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_hour("
        "  unix_time INTEGER PRIMARY KEY,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_day("
        "  unix_time INTEGER PRIMARY KEY,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ") WITHOUT ROWID;"

        "CREATE TABLE traffic_month("
        "  unix_time INTEGER PRIMARY KEY,"
        "  in_bytes INTEGER NOT NULL,"
        "  out_bytes INTEGER NOT NULL"
        ") WITHOUT ROWID;"
        ;

const char * const DatabaseSql::sqlSelectAppId =
        "SELECT id FROM app WHERE path = ?1;"
        ;

const char * const DatabaseSql::sqlInsertAppId =
        "INSERT INTO app(path, unix_time, in_bytes, out_bytes)"
        "  VALUES(?1, ?2, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficAppHour =
        "INSERT INTO traffic_app_hour(app_id, unix_time, in_bytes, out_bytes)"
        "  VALUES(?2, ?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficAppDay =
        "INSERT INTO traffic_app_day(app_id, unix_time, in_bytes, out_bytes)"
        "  VALUES(?2, ?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficAppMonth =
        "INSERT INTO traffic_app_month(app_id, unix_time, in_bytes, out_bytes)"
        "  VALUES(?2, ?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficHour =
        "INSERT INTO traffic_hour(unix_time, in_bytes, out_bytes)"
        "  VALUES(?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficDay =
        "INSERT INTO traffic_day(unix_time, in_bytes, out_bytes)"
        "  VALUES(?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlInsertTrafficMonth =
        "INSERT INTO traffic_month(unix_time, in_bytes, out_bytes)"
        "  VALUES(?1, 0, 0);"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppHour =
        "UPDATE traffic_app_hour"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE app_id = ?4 and unix_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppDay =
        "UPDATE traffic_app_day"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE app_id = ?4 and unix_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppMonth =
        "UPDATE traffic_app_month"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE app_id = ?4 and unix_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficHour =
        "UPDATE traffic_hour"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE unix_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficDay =
        "UPDATE traffic_day"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE unix_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficMonth =
        "UPDATE traffic_month"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE unix_time = ?1;"
        ;

const char * const DatabaseSql::sqlUpdateTrafficAppTotal =
        "UPDATE app"
        "  SET in_bytes = in_bytes + ?2,"
        "    out_bytes = out_bytes + ?3"
        "  WHERE id = ?1;"
        ;
