#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

class QQmlApplicationEngine;

class FortSettings;
class FirewallConf;

class FortManager : public QObject
{
    Q_OBJECT

public:
    explicit FortManager(QObject *parent = nullptr);

    static void registerQmlTypes();

signals:
    void fillConf(QObject *newConf);

public slots:
    void showWindow();

    bool saveConf();

private:
    void setupContext();

private:
    QQmlApplicationEngine *m_engine;

    FortSettings *m_fortSettings;
    FirewallConf *m_firewallConf;
};

#endif // FORTMANAGER_H
