#include "statsql.h"

const char *const StatSql::sqlSelectAppId = "SELECT app_id FROM app WHERE path = ?1;";

const char *const StatSql::sqlInsertAppId =
        "INSERT INTO app(path, creat_time, traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, ?2, ?3, 0, 0);";

const char *const StatSql::sqlDeleteStatAppId =
        "DELETE FROM app WHERE app_id = ?1"
        "  and (SELECT 1 FROM conn WHERE app_id = ?1 LIMIT 1) is null;";

const char *const StatSql::sqlSelectStatAppPaths = "SELECT app_id, path"
                                                   "  FROM app JOIN traffic_app USING(app_id)"
                                                   "  ORDER BY app_id;";

const char *const StatSql::sqlInsertTrafAppHour =
        "INSERT INTO traffic_app_hour(app_id, traf_time, in_bytes, out_bytes)"
        "  VALUES(?4, ?1, ?2, ?3);";

const char *const StatSql::sqlInsertTrafAppDay =
        "INSERT INTO traffic_app_day(app_id, traf_time, in_bytes, out_bytes)"
        "  VALUES(?4, ?1, ?2, ?3);";

const char *const StatSql::sqlInsertTrafAppMonth =
        "INSERT INTO traffic_app_month(app_id, traf_time, in_bytes, out_bytes)"
        "  VALUES(?4, ?1, ?2, ?3);";

const char *const StatSql::sqlInsertTrafAppTotal =
        "INSERT INTO traffic_app(app_id, traf_time, in_bytes, out_bytes)"
        "  VALUES(?4, ?1, ?2, ?3);";

const char *const StatSql::sqlInsertTrafHour =
        "INSERT INTO traffic_hour(traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, ?2, ?3);";

const char *const StatSql::sqlInsertTrafDay =
        "INSERT INTO traffic_day(traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, ?2, ?3);";

const char *const StatSql::sqlInsertTrafMonth =
        "INSERT INTO traffic_month(traf_time, in_bytes, out_bytes)"
        "  VALUES(?1, ?2, ?3);";

const char *const StatSql::sqlUpdateTrafAppHour = "UPDATE traffic_app_hour"
                                                  "  SET in_bytes = in_bytes + ?2,"
                                                  "    out_bytes = out_bytes + ?3"
                                                  "  WHERE app_id = ?4 and traf_time = ?1;";

const char *const StatSql::sqlUpdateTrafAppDay = "UPDATE traffic_app_day"
                                                 "  SET in_bytes = in_bytes + ?2,"
                                                 "    out_bytes = out_bytes + ?3"
                                                 "  WHERE app_id = ?4 and traf_time = ?1;";

const char *const StatSql::sqlUpdateTrafAppMonth = "UPDATE traffic_app_month"
                                                   "  SET in_bytes = in_bytes + ?2,"
                                                   "    out_bytes = out_bytes + ?3"
                                                   "  WHERE app_id = ?4 and traf_time = ?1;";

const char *const StatSql::sqlUpdateTrafAppTotal = "UPDATE traffic_app"
                                                   "  SET in_bytes = in_bytes + ?2,"
                                                   "    out_bytes = out_bytes + ?3"
                                                   "  WHERE app_id = ?4 and 0 != ?1;";

const char *const StatSql::sqlUpdateTrafHour = "UPDATE traffic_hour"
                                               "  SET in_bytes = in_bytes + ?2,"
                                               "    out_bytes = out_bytes + ?3"
                                               "  WHERE traf_time = ?1;";

const char *const StatSql::sqlUpdateTrafDay = "UPDATE traffic_day"
                                              "  SET in_bytes = in_bytes + ?2,"
                                              "    out_bytes = out_bytes + ?3"
                                              "  WHERE traf_time = ?1;";

const char *const StatSql::sqlUpdateTrafMonth = "UPDATE traffic_month"
                                                "  SET in_bytes = in_bytes + ?2,"
                                                "    out_bytes = out_bytes + ?3"
                                                "  WHERE traf_time = ?1;";

const char *const StatSql::sqlSelectMinTrafAppHour = "SELECT min(traf_time) FROM traffic_app_hour"
                                                     "  WHERE app_id = ?1;";

const char *const StatSql::sqlSelectMinTrafAppDay = "SELECT min(traf_time) FROM traffic_app_day"
                                                    "  WHERE app_id = ?1;";

const char *const StatSql::sqlSelectMinTrafAppMonth = "SELECT min(traf_time) FROM traffic_app_month"
                                                      "  WHERE app_id = ?1;";

const char *const StatSql::sqlSelectMinTrafAppTotal =
        "SELECT traf_time FROM traffic_app WHERE app_id = ?1;";

const char *const StatSql::sqlSelectMinTrafHour = "SELECT min(traf_time) FROM traffic_hour;";

const char *const StatSql::sqlSelectMinTrafDay = "SELECT min(traf_time) FROM traffic_app_day;";

const char *const StatSql::sqlSelectMinTrafMonth = "SELECT min(traf_time) FROM traffic_app_month;";

const char *const StatSql::sqlSelectMinTrafTotal = "SELECT min(traf_time) FROM traffic_app;";

const char *const StatSql::sqlSelectTrafAppHour = "SELECT in_bytes, out_bytes"
                                                  "  FROM traffic_app_hour"
                                                  "  WHERE app_id = ?2 and traf_time = ?1;";

const char *const StatSql::sqlSelectTrafAppDay = "SELECT in_bytes, out_bytes"
                                                 "  FROM traffic_app_day"
                                                 "  WHERE app_id = ?2 and traf_time = ?1;";

const char *const StatSql::sqlSelectTrafAppMonth = "SELECT in_bytes, out_bytes"
                                                   "  FROM traffic_app_month"
                                                   "  WHERE app_id = ?2 and traf_time = ?1;";

const char *const StatSql::sqlSelectTrafAppTotal = "SELECT in_bytes, out_bytes"
                                                   "  FROM traffic_app"
                                                   "  WHERE app_id = ?2 and 0 != ?1;";

const char *const StatSql::sqlSelectTrafHour = "SELECT in_bytes, out_bytes"
                                               "  FROM traffic_hour WHERE traf_time = ?1;";

const char *const StatSql::sqlSelectTrafDay = "SELECT in_bytes, out_bytes"
                                              "  FROM traffic_day WHERE traf_time = ?1;";

const char *const StatSql::sqlSelectTrafMonth = "SELECT in_bytes, out_bytes"
                                                "  FROM traffic_month WHERE traf_time = ?1;";

const char *const StatSql::sqlSelectTrafTotal = "SELECT sum(in_bytes), sum(out_bytes)"
                                                "  FROM traffic_app WHERE 0 != ?1;";

const char *const StatSql::sqlDeleteTrafAppHour = "DELETE FROM traffic_app_hour"
                                                  "  WHERE traf_time < ?1;";

const char *const StatSql::sqlDeleteTrafAppDay = "DELETE FROM traffic_app_day"
                                                 "  WHERE traf_time < ?1;";

const char *const StatSql::sqlDeleteTrafAppMonth = "DELETE FROM traffic_app_month"
                                                   "  WHERE traf_time < ?1;";

const char *const StatSql::sqlDeleteTrafHour = "DELETE FROM traffic_hour WHERE traf_time < ?1;";

const char *const StatSql::sqlDeleteTrafDay = "DELETE FROM traffic_day WHERE traf_time < ?1;";

const char *const StatSql::sqlDeleteTrafMonth = "DELETE FROM traffic_month WHERE traf_time < ?1;";

const char *const StatSql::sqlDeleteAppTrafHour = "DELETE FROM traffic_app_hour"
                                                  "  WHERE app_id = ?1;";

const char *const StatSql::sqlDeleteAppTrafDay = "DELETE FROM traffic_app_day"
                                                 "  WHERE app_id = ?1;";

const char *const StatSql::sqlDeleteAppTrafMonth = "DELETE FROM traffic_app_month"
                                                   "  WHERE app_id = ?1;";

const char *const StatSql::sqlDeleteAppTrafTotal = "DELETE FROM traffic_app"
                                                   "  WHERE app_id = ?1;";

const char *const StatSql::sqlResetAppTrafTotals =
        "UPDATE traffic_app"
        "  SET traf_time = ?1, in_bytes = 0, out_bytes = 0;";
