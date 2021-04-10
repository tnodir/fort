#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <QColor>
#include <QHash>
#include <QObject>
#include <QRect>
#include <QSettings>

class EnvManager;
class FirewallConf;

class FortSettings : public QObject
{
    Q_OBJECT

public:
    explicit FortSettings(QObject *parent = nullptr);

    bool debug() const { return iniBool("base/debug"); }
    void setDebug(bool on) { setIniValue("base/debug", on); }

    bool console() const { return iniBool("base/console"); }
    void setConsole(bool on) { setIniValue("base/console", on); }

    QString defaultLanguage() const { return m_defaultLanguage; }

    QString language() const { return iniText("base/language", defaultLanguage()); }
    void setLanguage(const QString &v) { setIniValue("base/language", v); }

    bool hasPassword() const { return !passwordHash().isEmpty(); }

    QString passwordHash() const { return iniText("base/passwordHash"); }
    void setPasswordHash(const QString &v) { setIniValue("base/passwordHash", v); }

    QString appUpdatesUrl() const;
    int appVersion() const;

    int iniVersion() const { return iniInt("base/version", appVersion()); }
    void setIniVersion(int v) { setIniValue("base/version", v); }

    QRect progWindowGeometry() const { return iniValue("progWindow/geometry").toRect(); }
    void setProgWindowGeometry(const QRect &v) { setIniValue("progWindow/geometry", v); }

    bool progWindowMaximized() const { return iniBool("progWindow/maximized"); }
    void setProgWindowMaximized(bool on) { setIniValue("progWindow/maximized", on); }

    bool progAppsSortDesc() const { return iniBool("progWindow/appsSortDesc"); }
    void setProgSortDesc(bool v) { setIniValue("progWindow/appsSortDesc", v); }

    int progAppsSortColumn() const { return iniInt("progWindow/appsSortColumn"); }
    void setProgSortColumn(int v) { setIniValue("progWindow/appsSortColumn", v); }

    int progAppsHeaderVersion() const { return iniInt("progWindow/appsHeaderVersion"); }
    void setProgAppsHeaderVersion(int v) { setIniValue("progWindow/appsHeaderVersion", v); }

    QByteArray progAppsHeader() const { return iniByteArray("progWindow/appsHeader"); }
    void setProgAppsHeader(const QByteArray &v) { setIniValue("progWindow/appsHeader", v); }

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

    QRect zoneWindowGeometry() const { return iniValue("zoneWindow/geometry").toRect(); }
    void setZoneWindowGeometry(const QRect &v) { setIniValue("zoneWindow/geometry", v); }

    bool zoneWindowMaximized() const { return iniBool("zoneWindow/maximized"); }
    void setZoneWindowMaximized(bool on) { setIniValue("zoneWindow/maximized", on); }

    int zonesHeaderVersion() const { return iniInt("zoneWindow/zonesHeaderVersion"); }
    void setZonesHeaderVersion(int v) { setIniValue("zoneWindow/zonesHeaderVersion", v); }

    QByteArray zonesHeader() const { return iniByteArray("zoneWindow/zonesHeader"); }
    void setZonesHeader(const QByteArray &v) { setIniValue("zoneWindow/zonesHeader", v); }

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

    QColor graphWindowColorIn() const
    {
        return iniColor("graphWindow/colorIn", QColor(52, 196, 84));
    }
    void setGraphWindowColorIn(const QColor &v) { setIniColor("graphWindow/colorIn", v); }

    QColor graphWindowColorOut() const
    {
        return iniColor("graphWindow/colorOut", QColor(235, 71, 63));
    }
    void setGraphWindowColorOut(const QColor &v) { setIniColor("graphWindow/colorOut", v); }

    QColor graphWindowAxisColor() const
    {
        return iniColor("graphWindow/axisColor", QColor(0, 0, 0));
    }
    void setGraphWindowAxisColor(const QColor &v) { setIniColor("graphWindow/axisColor", v); }

    QColor graphWindowTickLabelColor() const
    {
        return iniColor("graphWindow/tickLabelColor", QColor(0, 0, 0));
    }
    void setGraphWindowTickLabelColor(const QColor &v)
    {
        setIniColor("graphWindow/tickLabelColor", v);
    }

    QColor graphWindowLabelColor() const
    {
        return iniColor("graphWindow/labelColor", QColor(0, 0, 0));
    }
    void setGraphWindowLabelColor(const QColor &v) { setIniColor("graphWindow/labelColor", v); }

    QColor graphWindowGridColor() const
    {
        return iniColor("graphWindow/gridColor", QColor(200, 200, 200));
    }
    void setGraphWindowGridColor(const QColor &v) { setIniColor("graphWindow/gridColor", v); }

    QRect connWindowGeometry() const { return iniValue("connWindow/geometry").toRect(); }
    void setConnWindowGeometry(const QRect &v) { setIniValue("connWindow/geometry", v); }

    bool connWindowMaximized() const { return iniBool("connWindow/maximized"); }
    void setConnWindowMaximized(bool on) { setIniValue("connWindow/maximized", on); }

    int connListHeaderVersion() const { return iniInt("connWindow/connListHeaderVersion"); }
    void setConnListHeaderVersion(int v) { setIniValue("connWindow/connListHeaderVersion", v); }

    QByteArray connListHeader() const { return iniByteArray("connWindow/connListHeader"); }
    void setConnListHeader(const QByteArray &v) { setIniValue("connWindow/connListHeader", v); }

    bool connAutoScroll() const { return iniBool("connWindow/autoScroll"); }
    void setConnAutoScroll(bool on) { setIniValue("connWindow/autoScroll", on); }

    bool connShowHostNames() const { return iniBool("connWindow/showHostNames"); }
    void setConnShowHostNames(bool on) { setIniValue("connWindow/showHostNames", on); }

    qint32 quotaDayAlerted() const { return iniInt("quota/dayAlerted"); }
    void setQuotaDayAlerted(qint32 v) { setIniValue("quota/dayAlerted", v); }

    qint32 quotaMonthAlerted() const { return iniInt("quota/monthAlerted"); }
    void setQuotaMonthAlerted(qint32 v) { setIniValue("quota/monthAlerted", v); }

    bool hotKeyEnabled() const { return iniBool("hotKey/enabled"); }
    void setHotKeyEnabled(bool on) { setIniValue("hotKey/enabled", on); }

    QString hotKeyPrograms() const { return iniText("hotKey/programs"); }
    QString hotKeyOptions() const { return iniText("hotKey/options"); }
    QString hotKeyZones() const { return iniText("hotKey/zones"); }
    QString hotKeyGraph() const { return iniText("hotKey/graph"); }
    QString hotKeyConnections() const { return iniText("hotKey/connections"); }
    QString hotKeyFilter() const { return iniText("hotKey/filter", "Ctrl+Alt+Shift+F"); }
    QString hotKeyStopTraffic() const { return iniText("hotKey/stopTraffic"); }
    QString hotKeyStopInetTraffic() const { return iniText("hotKey/stopInetTraffic"); }
    QString hotKeyAllowAllNew() const { return iniText("hotKey/allowAllNew"); }
    QString hotKeyAppGroupModifiers() const
    {
        return iniText("hotKey/appGroupModifiers", "Ctrl+Alt+Shift");
    }
    QString hotKeyQuit() const { return iniText("hotKey/quit"); }

    bool noCache() const { return m_noCache; }
    bool isService() const { return m_isService; }
    bool hasService() const { return m_hasService; }
    void setHasService(bool v) { m_hasService = v; }

    QString profilePath() const { return m_profilePath; }

    QString statPath() const { return m_statPath; }
    QString statFilePath() const;

    QString logsPath() const { return m_logsPath; }
    QString cachePath() const { return m_cachePath; }

    QString confFilePath() const;

    bool isWindowControl() const { return m_isWindowControl; }
    QString controlCommand() const { return m_controlCommand; }

    const QStringList &args() const { return m_args; }

    const QStringList &appArguments() const { return m_appArguments; }

    QString errorMessage() const { return m_errorMessage; }

    bool isPasswordRequired();
    void setPasswordChecked(bool checked, int unlockType);
    void resetCheckedPassword(int unlockType = 0);

    bool confMigrated() const;
    bool confCanMigrate(QString &viaVersion) const;

signals:
    void iniChanged();
    void errorMessageChanged();
    void passwordUnlocked();

public slots:
    void setupGlobal();
    void initialize(const QStringList &args, EnvManager *envManager = nullptr);

    void readConfIni(FirewallConf &conf) const;
    bool writeConfIni(const FirewallConf &conf);

    void bulkUpdateBegin();
    void bulkUpdateEnd();

private:
    void processArguments(const QStringList &args, EnvManager *envManager);
    void setupIni();

    void setErrorMessage(const QString &errorMessage);

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
    void setIniColor(
            const QString &key, const QColor &value, const QColor &defaultValue = QColor());

    QVariant iniValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setIniValue(
            const QString &key, const QVariant &value, const QVariant &defaultValue = QVariant());

    QVariant cacheValue(const QString &key) const;
    void setCacheValue(const QString &key, const QVariant &value) const;

    void removeIniKey(const QString &key);

    QStringList iniChildKeys(const QString &prefix) const;

    bool iniSync();

    static QString startupShortcutPath();

private:
    uint m_iniExists : 1;
    uint m_noCache : 1;
    uint m_isService : 1;
    uint m_hasService : 1;
    uint m_isWindowControl : 1;

    uint m_passwordChecked : 1;
    uint m_passwordUnlockType : 3;

    uint m_bulkUpdating : 1;
    uint m_bulkIniChanged : 1;

    QString m_defaultLanguage;
    QString m_profilePath;
    QString m_statPath;
    QString m_logsPath;
    QString m_cachePath;
    QString m_controlCommand;
    QStringList m_args;

    QStringList m_appArguments;

    QString m_errorMessage;

    QSettings *m_ini = nullptr;

    mutable QHash<QString, QVariant> m_cache;
};

#endif // FORTSETTINGS_H
