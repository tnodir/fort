#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <QObject>
#include <QHash>
#include <QRect>
#include <QSettings>

#include "../common/version.h"

QT_FORWARD_DECLARE_CLASS(FirewallConf)

typedef QHash<QString, QByteArray> TasksMap;

class FortSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool debug READ debug WRITE setDebug NOTIFY iniChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY iniChanged)
    Q_PROPERTY(QString updatesUrl READ updatesUrl CONSTANT)
    Q_PROPERTY(bool startWithWindows READ startWithWindows WRITE setStartWithWindows NOTIFY startWithWindowsChanged)
    Q_PROPERTY(QString logsPath READ logsPath CONSTANT)
    Q_PROPERTY(QString profilePath READ profilePath CONSTANT)
    Q_PROPERTY(QString statPath READ statPath CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString appUpdatesUrl READ appUpdatesUrl CONSTANT)

public:
    explicit FortSettings(const QStringList &args,
                          QObject *parent = nullptr);

    bool hasProvBoot() const { return m_hasProvBoot; }

    bool debug() const { return iniBool("base/debug"); }
    void setDebug(bool on) { setIniValue("base/debug", on); }

    bool console() const { return iniBool("base/console"); }
    void setConsole(bool on) { setIniValue("base/console", on); }

    QString language() const { return iniText("base/language", "en"); }
    void setLanguage(const QString &v) { setIniValue("base/language", v); }

    int iniVersion() const { return iniInt("base/version"); }
    void setIniVersion(int v) { setIniValue("base/version", v); }

    QRect windowGeometry() const { return iniValue("window/geometry").toRect(); }
    void setWindowGeometry(const QRect &v) { setIniValue("window/geometry", v); }

    bool windowMaximized() const { return iniBool("window/maximized"); }
    void setWindowMaximized(bool on) { setIniValue("window/maximized", on); }

    qint32 quotaDayAlerted() const { return iniInt("quota/dayAlerted"); }
    void setQuotaDayAlerted(qint32 v) { setIniValue("quota/dayAlerted", v); }

    qint32 quotaMonthAlerted() const { return iniInt("quota/monthAlerted"); }
    void setQuotaMonthAlerted(qint32 v) { setIniValue("quota/monthAlerted", v); }

    QString updatesUrl() const { return APP_UPDATES_URL; }

    bool startWithWindows() const;
    void setStartWithWindows(bool start);

    TasksMap tasks() const;
    bool setTasks(const TasksMap &map);

    QString logsPath() const;

    QString profilePath() const { return m_profilePath; }

    QString statPath() const { return m_statPath; }
    QString statFilePath() const;

    QString errorMessage() const { return m_errorMessage; }

    QString appUpdatesUrl() const { return APP_UPDATES_URL; }

signals:
    void iniChanged();
    void startWithWindowsChanged();
    void errorMessageChanged();

public slots:
    bool readConf(FirewallConf &conf, bool &isNew);
    bool writeConf(const FirewallConf &conf);

    bool readConfIni(FirewallConf &conf) const;
    bool writeConfIni(const FirewallConf &conf);

private:
    void processArguments(const QStringList &args);
    void setupIni();

    void setErrorMessage(const QString &errorMessage);

    QString confFilePath() const;
    QString confBackupFilePath() const;

    bool tryToReadConf(FirewallConf &conf, const QString &filePath);
    bool tryToWriteConf(const FirewallConf &conf, const QString &filePath);

    QVariant migrateConf(const QVariant &confVar);
    void removeMigratedKeys();

    bool iniBool(const QString &key, bool defaultValue = false) const;
    int iniInt(const QString &key, int defaultValue = 0) const;
    uint iniUInt(const QString &key, int defaultValue = 0) const;
    qreal iniReal(const QString &key, qreal defaultValue = 0) const;
    QString iniText(const QString &key, const QString &defaultValue = QString()) const;
    QStringList iniList(const QString &key) const;

    QVariant iniValue(const QString &key,
                      const QVariant &defaultValue = QVariant()) const;
    void setIniValue(const QString &key, const QVariant &value,
                     const QVariant &defaultValue = QVariant());

    void removeIniKey(const QString &key);

    QStringList iniChildKeys(const QString &prefix) const;

    bool iniSync();

    static QString startupShortcutPath();

private:
    uint m_hasProvBoot  : 1;

    QString m_profilePath;
    QString m_statPath;

    QString m_errorMessage;

    QSettings *m_ini;
};

#endif // FORTSETTINGS_H
