#include "conftimeperiodmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>

#include "confmanager.h"
#include "confrulemanager.h"

namespace {

const QLoggingCategory LC("confTimePeriod");

inline constexpr int TIME_PERIODS_UPDATE_INTERVAL = 60 * 1000; // 1 minute

const char *const sqlInsertTimePeriod = "INSERT INTO time_period(period_id, name, notes,"
                                        "    enabled, mod_time)"
                                        "  VALUES(?1, ?2, ?3, ?4, ?5);";

const char *const sqlUpdateTimePeriod = "UPDATE time_period"
                                        "  SET name = ?2, notes = ?3, enabled = ?4, mod_time = ?5"
                                        "  WHERE period_id = ?1;";

const char *const sqlSelectTimePeriodNameById =
        "SELECT name FROM time_period WHERE period_id = ?1;";

const char *const sqlSelectTimePeriodIds = "SELECT period_id FROM time_period"
                                           "  WHERE period_id <= ?1 ORDER BY period_id;";

const char *const sqlDeleteTimePeriod = "DELETE FROM time_period WHERE period_id = ?1;";

const char *const sqlDeleteGroupTimePeriod = "UPDATE app_group SET period_id = NULL"
                                             "  WHERE period_id = ?1;";

const char *const sqlDeleteSpeedLimitTimePeriod = "UPDATE speed_limit SET period_id = NULL"
                                                  "  WHERE period_id = ?1;";

const char *const sqlDeleteRuleTimePeriod = "UPDATE rule SET period_id = NULL"
                                            "  WHERE period_id = ?1;";

const char *const sqlUpdateTimePeriodEnabled =
        "UPDATE time_period SET enabled = ?2 WHERE period_id = ?1;";

const char *const sqlUpdateTimePeriodName =
        "UPDATE time_period SET name = ?2 WHERE period_id = ?1;";

const char *const sqlSelectAnyTimePeriod = "SELECT 1 FROM time_period LIMIT 1;";

const char *const sqlSelectTimePeriodIntervals = "SELECT week_days, time_from, time_to"
                                                 "  FROM time_period_interval"
                                                 "  WHERE period_id = ?1"
                                                 "  ORDER BY order_index;";

const char *const sqlSelectAllTimePeriodIntervals =
        "SELECT p.period_id, p.enabled, i.week_days, i.time_from, i.time_to"
        "  FROM time_period p"
        "  LEFT JOIN time_period_interval i ON i.period_id = p.period_id;";

const char *const sqlInsertTimePeriodInterval =
        "INSERT INTO time_period_interval(period_id, order_index, week_days,"
        "    time_from, time_to)"
        "  VALUES(?1, ?2, ?3, ?4, ?5);";

const char *const sqlDeleteTimePeriodIntervals =
        "DELETE FROM time_period_interval WHERE period_id = ?1;";

void fillTimePeriodInterval(TimePeriodInterval &interval, const SqliteStmt &stmt, int column)
{
    interval.weekDays = stmt.columnInt(column);
    interval.timeFrom = stmt.columnText(column + 1);
    interval.timeTo = stmt.columnText(column + 2);
}

// The disabled Time Period is always active, as if it isn't set
bool isTimePeriodRowActive(const SqliteStmt &stmt, int dayOfWeek, QTime time)
{
    const bool enabled = stmt.columnBool(1);
    if (!enabled)
        return true;

    TimePeriodInterval interval;
    fillTimePeriodInterval(interval, stmt, /*column=*/2);

    return interval.isActive(dayOfWeek, time);
}

}

ConfTimePeriodManager::ConfTimePeriodManager(QObject *parent) : ConfManagerBase(parent)
{
    setupPeriodsTimer();
    setupTimePeriodNamesCache();
}

void ConfTimePeriodManager::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();

    // The imported DB may reuse the ids
    connect(confManager, &ConfManager::imported, this, [&] {
        clearTimePeriodNamesCache();
        resetActivePeriods();
    });
}

QString ConfTimePeriodManager::timePeriodNameById(quint8 periodId)
{
    if (periodId == 0)
        return {};

    QString name = m_timePeriodNamesCache.value(periodId);

    if (name.isEmpty()) {
        name = DbQuery(sqliteDb())
                       .sql(sqlSelectTimePeriodNameById)
                       .vars({ periodId })
                       .execute()
                       .toString();

        m_timePeriodNamesCache.insert(periodId, name);
    }

    return name;
}

bool ConfTimePeriodManager::addOrUpdateTimePeriod(TimePeriod &period)
{
    bool ok = true;

    beginWriteTransaction();

    const bool isNew = (period.periodId == 0);
    if (isNew) {
        period.periodId = DbQuery(sqliteDb(), &ok)
                                  .sql(sqlSelectTimePeriodIds)
                                  .vars({ ConfUtil::timePeriodMaxCount() })
                                  .getFreeId(/*maxId=*/ConfUtil::timePeriodMaxCount());
    }

    if (ok) {
        const QVariantList vars = {
            period.periodId,
            period.name,
            period.notes,
            period.enabled,
            DateUtil::now(),
        };

        DbQuery(sqliteDb(), &ok)
                .sql(isNew ? sqlInsertTimePeriod : sqlUpdateTimePeriod)
                .vars(vars)
                .executeOk();
    }

    ok = ok && saveTimePeriodIntervals(period);

    endTransaction(ok);

    if (!ok)
        return false;

    if (isNew) {
        emit timePeriodAdded();
    } else {
        emit timePeriodUpdated();
    }

    checkActivePeriods();

    return true;
}

bool ConfTimePeriodManager::deleteTimePeriod(quint8 periodId)
{
    bool ok = false;
    int rulePeriodsCount = 0;

    beginWriteTransaction();

    const QVariantList vars = { periodId };

    if (DbQuery(sqliteDb(), &ok).sql(sqlDeleteTimePeriod).vars(vars).executeOk()) {
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteTimePeriodIntervals).vars(vars).executeOk();

        // Delete the Time Period from Groups, Speed Limits and Rules
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteGroupTimePeriod).vars(vars).executeOk();
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteSpeedLimitTimePeriod).vars(vars).executeOk();
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteRuleTimePeriod).vars(vars).executeOk();

        rulePeriodsCount = sqliteDb()->changes();
    }

    endTransaction(ok);

    if (!ok)
        return false;

    if (rulePeriodsCount > 0) {
        Fort::confRuleManager()->updateDriverRules(); // the Time Period's id can be reused
    }

    emit timePeriodRemoved(periodId);

    checkActivePeriods(/*forceChanged=*/true);

    return true;
}

bool ConfTimePeriodManager::updateTimePeriodName(quint8 periodId, const QString &name)
{
    if (!executeWrite(sqlUpdateTimePeriodName, { periodId, name }))
        return false;

    emit timePeriodUpdated();

    return true;
}

bool ConfTimePeriodManager::updateTimePeriodEnabled(quint8 periodId, bool enabled)
{
    if (!executeWrite(sqlUpdateTimePeriodEnabled, { periodId, enabled }))
        return false;

    emit timePeriodUpdated();

    checkActivePeriods();

    return true;
}

bool ConfTimePeriodManager::loadTimePeriodIntervals(
        TimePeriodIntervals &intervals, quint8 periodId) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectTimePeriodIntervals).vars({ periodId }).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        TimePeriodInterval interval;
        fillTimePeriodInterval(interval, stmt, /*column=*/0);

        intervals.append(interval);
    }

    return true;
}

bool ConfTimePeriodManager::isTimePeriodActive(bool periodEnabled, quint8 periodId)
{
    if (!periodEnabled)
        return true; // the Time Period isn't applied

    if (periodId == 0 || periodId > ConfUtil::timePeriodMaxCount())
        return true; // no Time Period

    return (activePeriodsMask() & (quint64(1) << (periodId - 1))) != 0;
}

quint64 ConfTimePeriodManager::activePeriodsMask()
{
    if (!m_activeMaskValid) {
        m_activeMask = calcActivePeriodsMask();
        m_activeMaskValid = true;

        startPeriodsTimer();
    }

    return m_activeMask;
}

void ConfTimePeriodManager::checkActivePeriods(bool forceChanged)
{
    if (!m_activeMaskValid)
        return; // not tracked yet

    stopPeriodsTimer();

    const quint64 activeMask = calcActivePeriodsMask();
    const bool changed = forceChanged || (activeMask != m_activeMask);

    m_activeMask = activeMask;

    startPeriodsTimer();

    if (changed) {
        updateDriverPeriods();

        emit activePeriodsChanged();
    }
}

quint64 ConfTimePeriodManager::calcActivePeriodsMask() const
{
    const QDateTime now = DateUtil::now();
    const int dayOfWeek = now.date().dayOfWeek();
    const QTime time = now.time();

    quint64 activeMask = 0;

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectAllTimePeriodIntervals).prepare(stmt))
        return 0;

    while (stmt.step() == SqliteStmt::StepRow) {
        const int periodId = stmt.columnInt(0);
        if (Q_UNLIKELY(periodId <= 0 || periodId > ConfUtil::timePeriodMaxCount()))
            continue; // skip an out of range Time Period

        if (isTimePeriodRowActive(stmt, dayOfWeek, time)) {
            activeMask |= (quint64(1) << (periodId - 1));
        }
    }

    return activeMask;
}

void ConfTimePeriodManager::updateDriverPeriods()
{
    ConfBuffer confBuf;

    confBuf.writePeriods(m_activeMask);

    auto driverManager = Fort::driverManager();

    if (!driverManager->writePeriods(confBuf.buffer())) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
    }
}

void ConfTimePeriodManager::setupPeriodsTimer()
{
    m_periodsTimer.setSingleShot(true);
    m_periodsTimer.setTimerType(Qt::PreciseTimer); // not earlier than the next minute

    connect(&m_periodsTimer, &QTimer::timeout, this, [&] { checkActivePeriods(); });
}

void ConfTimePeriodManager::startPeriodsTimer()
{
    const bool anyTimePeriod = DbQuery(sqliteDb()).sql(sqlSelectAnyTimePeriod).execute().toBool();

    if (anyTimePeriod) {
        // Wake up at the start of the next minute: the intervals are in "hh:mm"
        const int msecs = DateUtil::currentTime().msecsSinceStartOfDay();
        m_periodsTimer.start(TIME_PERIODS_UPDATE_INTERVAL - msecs % TIME_PERIODS_UPDATE_INTERVAL);
    }
}

void ConfTimePeriodManager::stopPeriodsTimer()
{
    m_periodsTimer.stop();
}

void ConfTimePeriodManager::setupTimePeriodNamesCache()
{
    connect(this, &ConfTimePeriodManager::timePeriodRemoved, this,
            &ConfTimePeriodManager::clearTimePeriodNamesCache);
    connect(this, &ConfTimePeriodManager::timePeriodUpdated, this,
            &ConfTimePeriodManager::clearTimePeriodNamesCache);
}

void ConfTimePeriodManager::clearTimePeriodNamesCache()
{
    m_timePeriodNamesCache.clear();
}

void ConfTimePeriodManager::resetActivePeriods()
{
    stopPeriodsTimer();

    m_activeMaskValid = false; // re-tracked by the next call
}

bool ConfTimePeriodManager::saveTimePeriodIntervals(const TimePeriod &period)
{
    if (!DbQuery(sqliteDb())
                    .sql(sqlDeleteTimePeriodIntervals)
                    .vars({ period.periodId })
                    .executeOk())
        return false;

    int orderIndex = 0;
    for (const TimePeriodInterval &interval : period.intervals) {
        const QVariantList vars = {
            period.periodId,
            orderIndex++,
            interval.weekDays,
            interval.timeFrom,
            interval.timeTo,
        };

        if (!DbQuery(sqliteDb()).sql(sqlInsertTimePeriodInterval).vars(vars).executeOk())
            return false;
    }

    return true;
}
