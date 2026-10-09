#ifndef CONFTIMEPERIODMANAGER_H
#define CONFTIMEPERIODMANAGER_H

#include <QHash>
#include <QObject>
#include <QTimer>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

#include "confmanagerbase.h"
#include "timeperiod.h"

class ConfTimePeriodManager : public ConfManagerBase, public IocService
{
    Q_OBJECT

public:
    explicit ConfTimePeriodManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfTimePeriodManager)

    void setUp() override;

    QString timePeriodNameById(quint8 periodId);

    virtual bool addOrUpdateTimePeriod(TimePeriod &period);
    virtual bool deleteTimePeriod(quint8 periodId);
    virtual bool updateTimePeriodName(quint8 periodId, const QString &name);
    virtual bool updateTimePeriodEnabled(quint8 periodId, bool enabled);

    bool loadTimePeriodIntervals(TimePeriodIntervals &intervals, quint8 periodId) const;

    // The Time Periods' activity is tracked since the first call
    bool isTimePeriodActive(bool periodEnabled, quint8 periodId);
    quint64 activePeriodsMask();

signals:
    void timePeriodAdded();
    void timePeriodRemoved(quint8 periodId);
    void timePeriodUpdated();

    // The Groups and Speed Limits of the Time Periods must update their activity
    void activePeriodsChanged();

private:
    void checkActivePeriods(bool forceChanged = false);
    quint64 calcActivePeriodsMask() const;

    void updateDriverPeriods();

    void setupPeriodsTimer();
    void startPeriodsTimer();
    void stopPeriodsTimer();

    void setupTimePeriodNamesCache();
    void clearTimePeriodNamesCache();

    void resetActivePeriods();

    bool saveTimePeriodIntervals(const TimePeriod &period);

    void deleteStatActivePeriod(quint8 periodId);

private:
    bool m_activeMaskValid = false;

    quint64 m_activeMask = 0;

    mutable QHash<quint8, QString> m_timePeriodNamesCache;

    QTimer m_periodsTimer;
};

#endif // CONFTIMEPERIODMANAGER_H
