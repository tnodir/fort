#ifndef SETTINGS_H
#define SETTINGS_H

#include <QHash>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT

    friend class MapSettings;

public:
    explicit Settings(QObject *parent = nullptr);

    bool hasError() const;
    QString errorMessage() const;

    QString filePath() const { return ini()->fileName(); }

    void reload();
    void clearCache();

    bool canMigrate(QString &viaVersion) const;

protected:
    int qtVersion() const { return iniInt("base/qtVersion"); }
    void setQtVersion(int v) { setIniValue("base/qtVersion", v); }

    bool checkQtVersionIsEqual() const;
    bool checkQtVersionIsValid() const;

    bool iniVersionSet() const { return ini()->contains("base/version"); }

    int iniVersion() const { return iniInt("base/version", appVersion()); }
    void setIniVersion(int v) { setIniValue("base/version", v); }

    bool checkIniVersion(int &oldVersion) const;
    bool checkIniVersionOnWrite(int &oldVersion) const;

    bool iniExists() const { return m_iniExists; }

    QSettings *ini() const { return m_ini; }

    void setupIni(const QString &filePath);

    virtual void migrateQtVerOnLoad() { }
    virtual void migrateQtVerOnWrite();

    virtual void migrateIniOnLoad() { }
    virtual void migrateIniOnWrite();

    bool iniBool(const QString &key, bool defaultValue = false) const;
    int iniInt(const QString &key, int defaultValue = 0) const;
    uint iniUInt(const QString &key, uint defaultValue = 0) const;
    quint64 iniUInt64(const QString &key, quint64 defaultValue = 0) const;
    qreal iniReal(const QString &key, qreal defaultValue = 0) const;
    QString iniText(const QString &key, const QString &defaultValue = QString()) const;
    QStringList iniList(const QString &key) const;
    QVariantMap iniMap(const QString &key) const;
    QByteArray iniByteArray(const QString &key) const;

    QVariant iniValue(const QString &key, const QVariant &defaultValue = {}) const;
    void setIniValue(const QString &key, const QVariant &value, const QVariant &defaultValue = {});

    QString cacheKey(const QString &key) const;
    QVariant cacheValue(const QString &key) const;
    void setCacheValue(const QString &key, const QVariant &value) const;

    void saveIniValue(const QString &key, const QVariant &value);
    void removeIniKey(const QString &key);

    QStringList iniChildKeys(const QString &prefix) const;

    void iniFlush();
    void iniSync();

    void checkStatus() const;

    static int appVersion();

private:
    uint m_iniExists : 1 = false;

    QSettings *m_ini = nullptr;

    mutable QHash<QString, QVariant> m_cache;
};

#endif // SETTINGS_H
