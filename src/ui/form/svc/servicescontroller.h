#ifndef SERVICESCONTROLLER_H
#define SERVICESCONTROLLER_H

#include <QObject>

class ConfManager;
class IniUser;
class ServiceListModel;
class TranslationManager;
class WindowManager;

class ServicesController : public QObject
{
    Q_OBJECT

public:
    explicit ServicesController(QObject *parent = nullptr);

    ConfManager *confManager() const;
    IniUser *iniUser() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;
    ServiceListModel *serviceListModel() const { return m_serviceListModel; }

    void initialize();

signals:
    void retranslateUi();

private:
    ServiceListModel *m_serviceListModel = nullptr;
};

#endif // SERVICESCONTROLLER_H
