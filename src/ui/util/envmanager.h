#ifndef ENVMANAGER_H
#define ENVMANAGER_H

#include <QHash>
#include <QObject>

class EnvManager : public QObject
{
    Q_OBJECT

public:
    explicit EnvManager(QObject *parent = nullptr);

    QString expandString(const QString &text);

    QString envVar(const QString &key);
    void setCachedEnvVar(const QString &key, const QVariant &value);

signals:
    void environmentUpdated();

public slots:
    void clearCache();

    void onEnvironmentChanged();

private:
    QString expandStringRecursive(const QString &text,
                                  quint16 callLevel = 0);

    static QVariant readEnvVar(const QString &key);
    static QVariant readRegVar(const QString &key, const char *envPath);

private:
    QHash<QString, QVariant> m_cache;
};

#endif // ENVMANAGER_H
