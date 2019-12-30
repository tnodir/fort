#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <QColor>
#include <QHash>
#include <QObject>
#include <QRect>
#include <QSettings>

#include "../common/version.h"

QT_FORWARD_DECLARE_CLASS(FirewallConf)

using TasksMap = QHash<QString, QByteArray>;

class FortSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool debug READ debug WRITE setDebug NOTIFY iniChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY iniChanged)
    Q_PROPERTY(bool hasPassword READ hasPassword NOTIFY iniChanged)
    Q_PROPERTY(QString passwordHash READ passwordHash WRITE setPasswordHash NOTIFY iniChanged)
    Q_PROPERTY(int appVersion READ appVersion CONSTANT)
    Q_PROPERTY(int iniVersion READ iniVersion WRITE setIniVersion NOTIFY iniChanged)
    Q_PROPERTY(QByteArray optWindowAddrSplit READ optWindowAddrSplit WRITE setOptWindowAddrSplit NOTIFY iniChanged)
    Q_PROPERTY(QByteArray optWindowAppsSplit READ optWindowAppsSplit WRITE setOptWindowAppsSplit NOTIFY iniChanged)
    Q_PROPERTY(bool graphWindowVisible READ graphWindowVisible WRITE setGraphWindowVisible NOTIFY iniChanged)
    Q_PROPERTY(bool graphWindowAlwaysOnTop READ graphWindowAlwaysOnTop WRITE setGraphWindowAlwaysOnTop NOTIFY iniChanged)
    Q_PROPERTY(bool graphWindowFrameless READ graphWindowFrameless WRITE setGraphWindowFrameless NOTIFY iniChanged)
    Q_PROPERTY(bool graphWindowClickThrough READ graphWindowClickThrough WRITE setGraphWindowClickThrough NOTIFY iniChanged)
    Q_PROPERTY(bool graphWindowHideOnHover READ graphWindowHideOnHover WRITE setGraphWindowHideOnHover NOTIFY iniChanged)
    Q_PROPERTY(int graphWindowOpacity READ graphWindowOpacity WRITE setGraphWindowOpacity NOTIFY iniChanged)
    Q_PROPERTY(int graphWindowHoverOpacity READ graphWindowHoverOpacity WRITE setGraphWindowHoverOpacity NOTIFY iniChanged)
    Q_PROPERTY(int graphWindowMaxSeconds READ graphWindowMaxSeconds WRITE setGraphWindowMaxSeconds NOTIFY iniChanged)
    Q_PROPERTY(QColor graphWindowColor READ graphWindowColor WRITE setGraphWindowColor NOTIFY iniChanged)
    Q_PROPERTY(QColor graphWindowColorIn READ graphWindowColorIn WRITE setGraphWindowColorIn NOTIFY iniChanged)
    Q_PROPERTY(QColor graphWindowColorOut READ graphWindowColorOut WRITE setGraphWindowColorOut NOTIFY iniChanged)
    Q_PROPERTY(QColor graphWindowAxisColor READ graphWindowAxisColor WRITE setGraphWindowAxisColor NOTIFY iniChanged)
    Q_PROPERTY(QColor graphWindowTickLabelColor READ graphWindowTickLabelColor WRITE setGraphWindowTickLabelColor NOTIFY iniChanged)
    Q_PROPERTY(QColor graphWindowLabelColor READ graphWindowLabelColor WRITE setGraphWindowLabelColor NOTIFY iniChanged)
    Q_PROPERTY(QColor graphWindowGridColor READ graphWindowGridColor WRITE setGraphWindowGridColor NOTIFY iniChanged)
    Q_PROPERTY(bool startWithWindows READ startWithWindows WRITE setStartWithWindows NOTIFY startWithWindowsChanged)
    Q_PROPERTY(bool hotKeyEnabled READ hotKeyEnabled WRITE setHotKeyEnabled NOTIFY iniChanged)
    Q_PROPERTY(QString logsPath READ logsPath CONSTANT)
    Q_PROPERTY(QString profilePath READ profilePath CONSTANT)
    Q_PROPERTY(QString statPath READ statPath CONSTANT)
    Q_PROPERTY(QString cachePath READ cachePath CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString appUpdatesUrl READ appUpdatesUrl CONSTANT)

public:
    explicit FortSettings(const QStringList &args,
                          QObject *parent = nullptr);

    bool isPortable() const { return m_isPortable; }
    bool noCache() const { return m_noCache; }
    bool hasProvBoot() const { return m_hasProvBoot; }

    bool debug() const { return iniBool("base/debug"); }
    void setDebug(bool on) { setIniValue("base/debug", on); }

    bool console() const { return iniBool("base/console"); }
    void setConsole(bool on) { setIniValue("base/console", on); }

    QString language() const { return iniText("base/language", "en"); }
    void setLanguage(const QString &v) { setIniValue("base/language", v); }

    bool hasPassword() const { return !passwordHash().isEmpty(); }

    QString passwordHash() const { return iniText("base/passwordHash"); }
    void setPasswordHash(const QString &v) { setIniValue("base/passwordHash", v); }

    int appVersion() const { return APP_VERSION; }

    int iniVersion() const { return iniInt("base/version", appVersion()); }
    void setIniVersion(int v) { setIniValue("base/version", v); }

    QRect optWindowGeometry() const { return iniValue("optWindow/geometry").toRect(); }
    void setOptWindowGeometry(const QRect &v) { setIniValue("optWindow/geometry", v); }

    bool optWindowMaximized() const { return iniBool("optWindow/maximized"); }
    void setOptWindowMaximized(bool on) { setIniValue("optWindow/maximized", on); }

    QByteArray optWindowAddrSplit() const { return iniByteArray("optWindow/addrSplit"); }
    void setOptWindowAddrSplit(const QByteArray &v) { setIniValue("optWindow/addrSplit", v); }

    QByteArray optWindowAppsSplit() const { return iniByteArray("optWindow/appsSplit"); }
    void setOptWindowAppsSplit(const QByteArray &v) { setIniValue("optWindow/appsSplit", v); }

    QByteArray optWindowStatSplit() const { return iniByteArray("optWindow/statSplit"); }
    void setOptWindowStatSplit(const QByteArray &v) { setIniValue("optWindow/statSplit", v); }

    bool graphWindowVisible() const { return iniBool("graphWindow/visible"); }
    void setGraphWindowVisible(bool on) { setIniValue("graphWindow/visible", on); }

    QRect graphWindowGeometry() const { return iniValue("graphWindow/geometry").toRect(); }
    void setGraphWindowGeometry(const QRect &v) { setIniValue("graphWindow/geometry", v); }

    bool graphWindowMaximized() const { return iniBool("graphWindow/maximized"); }
    void setGraphWindowMaximized(bool on) { setIniValue("graphWindow/maximized", on); }

    bool graphWindowAlwaysOnTop() const { return iniBool("graphWindow/alwaysOnTop", true); }
    void setGraphWindowAlwaysOnTop(bool on) { setIniValue("graphWindow/alwaysOnTop", on); }

    bool graphWindowFrameless() const { return iniBool("graphWindow/frameless"); }
    void setGraphWindowFrameless(bool on) { setIniValue("graphWindow/frameless", on); }

    bool graphWindowClickThrough() const { return iniBool("graphWindow/clickThrough"); }
    void setGraphWindowClickThrough(bool on) { setIniValue("graphWindow/clickThrough", on); }

    bool graphWindowHideOnHover() const { return iniBool("graphWindow/hideOnHover"); }
    void setGraphWindowHideOnHover(bool on) { setIniValue("graphWindow/hideOnHover", on); }

    int graphWindowOpacity() const { return iniInt("graphWindow/opacity", 90); }
    void setGraphWindowOpacity(int v) { setIniValue("graphWindow/opacity", v); }

    int graphWindowHoverOpacity() const { return iniInt("graphWindow/hoverOpacity", 95); }
    void setGraphWindowHoverOpacity(int v) { setIniValue("graphWindow/hoverOpacity", v); }

    int graphWindowMaxSeconds() const { return iniInt("graphWindow/maxSeconds", 500); }
    void setGraphWindowMaxSeconds(int v) { setIniValue("graphWindow/maxSeconds", v); }

    QColor graphWindowColor() const { return iniColor("graphWindow/color", QColor(255, 255, 255)); }
    void setGraphWindowColor(const QColor &v) { setIniColor("graphWindow/color", v); }

    QColor graphWindowColorIn() const { return iniColor("graphWindow/colorIn", QColor(52, 196, 84)); }
    void setGraphWindowColorIn(const QColor &v) { setIniColor("graphWindow/colorIn", v); }

    QColor graphWindowColorOut() const { return iniColor("graphWindow/colorOut", QColor(235, 71, 63)); }
    void setGraphWindowColorOut(const QColor &v) { setIniColor("graphWindow/colorOut", v); }

    QColor graphWindowAxisColor() const { return iniColor("graphWindow/axisColor", QColor(0, 0, 0)); }
    void setGraphWindowAxisColor(const QColor &v) { setIniColor("graphWindow/axisColor", v); }

    QColor graphWindowTickLabelColor() const { return iniColor("graphWindow/tickLabelColor", QColor(0, 0, 0)); }
    void setGraphWindowTickLabelColor(const QColor &v) { setIniColor("graphWindow/tickLabelColor", v); }

    QColor graphWindowLabelColor() const { return iniColor("graphWindow/labelColor", QColor(0, 0, 0)); }
    void setGraphWindowLabelColor(const QColor &v) { setIniColor("graphWindow/labelColor", v); }

    QColor graphWindowGridColor() const { return iniColor("graphWindow/gridColor", QColor(200, 200, 200)); }
    void setGraphWindowGridColor(const QColor &v) { setIniColor("graphWindow/gridColor", v); }

    qint32 quotaDayAlerted() const { return iniInt("quota/dayAlerted"); }
    void setQuotaDayAlerted(qint32 v) { setIniValue("quota/dayAlerted", v); }

    qint32 quotaMonthAlerted() const { return iniInt("quota/monthAlerted"); }
    void setQuotaMonthAlerted(qint32 v) { setIniValue("quota/monthAlerted", v); }

    bool hotKeyEnabled() const { return iniBool("hotKey/enabled"); }
    void setHotKeyEnabled(bool on) { setIniValue("hotKey/enabled", on); }

    QString hotKeyOptions() const { return iniText("hotKey/options"); }
    QString hotKeyGraph() const { return iniText("hotKey/graph"); }
    QString hotKeyFilter() const { return iniText("hotKey/filter", "Ctrl+Alt+Shift+F"); }
    QString hotKeyStopTraffic() const { return iniText("hotKey/stopTraffic"); }
    QString hotKeyStopInetTraffic() const { return iniText("hotKey/stopInetTraffic"); }
    QString hotKeyAppGroupModifiers() const { return iniText("hotKey/appGroupModifiers", "Ctrl+Alt+Shift"); }
    QString hotKeyQuit() const { return iniText("hotKey/quit"); }

    bool startWithWindows() const;
    void setStartWithWindows(bool start);

    QString tasksKey() const { return "tasks"; }
    TasksMap tasks() const;
    bool setTasks(const TasksMap &map);
    void removeTasks();

    QString logsPath() const;

    QString profilePath() const { return m_profilePath; }

    QString statPath() const { return m_statPath; }
    QString statFilePath() const;

    QString cachePath() const { return m_cachePath; }

    QString confFilePath() const;

    QString confOldFilePath() const;
    QString confBackupFilePath() const;

    QString controlCommand() const { return m_controlCommand; }

    QStringList args() const { return m_args; }

    QString errorMessage() const { return m_errorMessage; }

    QString appUpdatesUrl() const { return APP_UPDATES_URL; }

signals:
    void iniChanged();
    void startWithWindowsChanged();
    void errorMessageChanged();

public slots:
    bool readConf(FirewallConf &conf, bool &isNew);
    bool writeConf(const FirewallConf &conf);

    void readConfIni(FirewallConf &conf) const;
    bool writeConfIni(const FirewallConf &conf);

    bool confMigrated() const;
    bool confCanMigrate(QString &viaVersion) const;

    void bulkUpdateBegin();
    void bulkUpdateEnd();

private:
    void processArguments(const QStringList &args);
    void setupIni();

    void setErrorMessage(const QString &errorMessage);

    bool tryToReadConf(FirewallConf &conf, const QString &filePath);

    QVariant migrateConf(const QVariant &confVar);

    void migrateIniOnStartup();
    void migrateIniOnWrite();

    bool iniBool(const QString &key, bool defaultValue = false) const;
    int iniInt(const QString &key, int defaultValue = 0) const;
    uint iniUInt(const QString &key, int defaultValue = 0) const;
    qreal iniReal(const QString &key, qreal defaultValue = 0) const;
    QString iniText(const QString &key, const QString &defaultValue = QString()) const;
    QStringList iniList(const QString &key) const;
    QVariantMap iniMap(const QString &key) const;
    QByteArray iniByteArray(const QString &key) const;

    QColor iniColor(const QString &key, const QColor &defaultValue = QColor()) const;
    void setIniColor(const QString &key, const QColor &value,
                     const QColor &defaultValue = QColor());

    QVariant iniValue(const QString &key,
                      const QVariant &defaultValue = QVariant()) const;
    void setIniValue(const QString &key, const QVariant &value,
                     const QVariant &defaultValue = QVariant());

    QVariant cacheValue(const QString &key) const;
    void setCacheValue(const QString &key, const QVariant &value) const;

    void removeIniKey(const QString &key);

    QStringList iniChildKeys(const QString &prefix) const;

    bool iniSync();

    static QString startupShortcutPath();

private:
    uint m_iniExists        : 1;
    uint m_isPortable       : 1;
    uint m_noCache          : 1;
    uint m_hasProvBoot      : 1;

    uint m_bulkUpdating     : 1;
    uint m_bulkIniChanged   : 1;

    QString m_profilePath;
    QString m_statPath;
    QString m_cachePath;
    QString m_controlCommand;
    QStringList m_args;

    QString m_errorMessage;

    QSettings *m_ini;

    mutable QHash<QString, QVariant> m_cache;
};

#endif // FORTSETTINGS_H
