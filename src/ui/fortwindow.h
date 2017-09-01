#ifndef FORTWINDOW_H
#define FORTWINDOW_H

#include <QObject>

class QQmlApplicationEngine;

class FortSettings;
class FirewallConf;

class FortWindow : public QObject
{
    Q_OBJECT

public:
    explicit FortWindow(FirewallConf *firewallConf,
                        FortSettings *fortSettings,
                        QObject *parent = nullptr);

    static void registerQmlTypes();

signals:

public slots:
    void show();

    bool save();

private:
    void setupContext();

private:
    QQmlApplicationEngine *m_engine;

    FirewallConf *m_firewallConf;
    FortSettings *m_fortSettings;
};

#endif // FORTWINDOW_H
