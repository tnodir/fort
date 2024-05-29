#ifndef DBERRORMANAGER_H
#define DBERRORMANAGER_H

#include <QObject>

#include <util/dirinfo.h>
#include <util/ioc/iocservice.h>

class DbErrorManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DbErrorManager(QObject *parent = nullptr);

    void setUp() override;

private slots:
    void checkConfDir();

protected:
    virtual void setupTimer();

    void setupDirInfo(const QString &path);

private:
    DirInfo m_confDir;
};

#endif // DBERRORMANAGER_H
