#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>

#include "../util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_CLASS(SqliteStmt)
QT_FORWARD_DECLARE_CLASS(TaskInfo)

class ConfManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ConfManager(const QString &filePath,
                         FortSettings *fortSettings,
                         QObject *parent = nullptr);
    ~ConfManager() override;
    CLASS_DELETE_COPY_MOVE(ConfManager)

    bool initialize();

    FirewallConf *cloneConf(const FirewallConf &conf,
                            QObject *parent = nullptr);

    bool load(FirewallConf &conf);
    bool save(const FirewallConf &conf, bool onlyFlags = false);

    bool loadTasks(const QList<TaskInfo *> &taskInfos);
    bool saveTasks(const QList<TaskInfo *> &taskInfos);

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();

private:
    void setErrorMessage(const QString &errorMessage);

    void setupDefault(FirewallConf &conf) const;

    bool loadFromDb(FirewallConf &conf, bool &isNew);
    bool saveToDb(const FirewallConf &conf);

    bool loadTask(TaskInfo *taskInfo);
    bool saveTask(TaskInfo *taskInfo);

    static QString migrateAppsText(const QString &text);

private:
    QString m_errorMessage;

    FortSettings *m_fortSettings;
    SqliteDb *m_sqliteDb;
};

#endif // CONFMANAGER_H
