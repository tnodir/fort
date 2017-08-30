#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <QObject>
#include <QSettings>

class FirewallConf;

class FortSettings : public QObject
{
    Q_OBJECT

public:
    explicit FortSettings(const QStringList &args,
                          QObject *parent = nullptr);

signals:

public slots:
    QString confFilePath() const;
    QString confBackupFilePath() const;

    bool readConf(FirewallConf &conf) const;
    bool writeConf(const FirewallConf &conf);

    bool readConfFlags(FirewallConf &conf) const;
    bool writeConfFlags(const FirewallConf &conf);

private:
    void processArguments(const QStringList &args);
    void setupIni();

    bool tryToReadConf(FirewallConf &conf, const QString &filePath) const;
    bool tryToWriteConf(const FirewallConf &conf, const QString &filePath);

    bool iniBool(const QString &key, bool defaultValue = false) const;
    int iniInt(const QString &key, int defaultValue = 0) const;
    int iniUInt(const QString &key, int defaultValue = 0) const;
    int iniReal(const QString &key, qreal defaultValue = 0) const;
    QString iniText(const QString &key, const QString &defaultValue = QString()) const;
    QStringList iniList(const QString &key) const;

    QVariant iniValue(const QString &key,
                      const QVariant &defaultValue = QVariant()) const;
    void setIniValue(const QString &key, const QVariant &value);

private:
    QString m_profilePath;

    QSettings *m_ini;
};

#endif // FORTSETTINGS_H
