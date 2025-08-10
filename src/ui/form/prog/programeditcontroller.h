#ifndef PROGRAMEDITCONTROLLER_H
#define PROGRAMEDITCONTROLLER_H

#include <conf/app.h>
#include <form/basecontroller.h>

class ProgramEditController : public BaseController
{
    Q_OBJECT

public:
    explicit ProgramEditController(QObject *parent = nullptr);

    const App &app() const { return m_app; }

    bool isWildcard() const { return m_isWildcard; }
    bool isNew() const { return !m_app.isValid(); }
    bool isSingleSelection() const { return m_appIdList.size() <= 1; }

    void initialize(const App &app, const QVector<qint64> &appIdList = {});

signals:
    void initializePage(const App &app);

    void validateFields(bool &ok);
    void fillApp(App &app);

    void closeOnSave();
    void closeDialog();

    void allowToggled(bool checked);

    void isWildcardSwitched();

public slots:
    void closeWindow();

    void switchWildcard();

    void saveChanges();

    bool addOrUpdateApp(App &app, bool onlyUpdate = false);
    bool updateApp(App &app);
    bool updateAppName(qint64 appId, const QString &appName);

    void warnDangerousOption() const;
    void warnRestartNeededOption() const;

private:
    bool save();

    bool saveApp(App &app);
    bool saveMulti(App &app);

    bool checkValidateFields();

private:
    bool m_isWildcard = false;

    App m_app;
    QVector<qint64> m_appIdList;
};

#endif // PROGRAMEDITCONTROLLER_H
