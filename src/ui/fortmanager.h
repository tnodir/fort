#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

class QQmlApplicationEngine;

class FortSettings;
class FirewallConf;

class FortManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FortSettings *fortSettings READ fortSettings CONSTANT)
    Q_PROPERTY(FirewallConf *firewallConf READ firewallConf CONSTANT)
    Q_PROPERTY(FirewallConf *firewallConfToEdit READ firewallConfToEdit CONSTANT)

public:
    explicit FortManager(QObject *parent = nullptr);

    FortSettings *fortSettings() const { return m_fortSettings; }
    FirewallConf *firewallConf() const { return m_firewallConf; }
    FirewallConf *firewallConfToEdit() const { return m_firewallConfToEdit; }

signals:

public slots:
    void showWindow();

    bool saveConf();
    bool applyConf();

private slots:
    void handleClosedWindow();

private:
    static void registerQmlTypes();

    void setupContext();

    bool saveSettings(FirewallConf *newConf);

    FirewallConf *cloneConf(const FirewallConf &conf);

private:
    QQmlApplicationEngine *m_engine;

    FortSettings *m_fortSettings;
    FirewallConf *m_firewallConf;
    FirewallConf *m_firewallConfToEdit;
};

#endif // FORTMANAGER_H
