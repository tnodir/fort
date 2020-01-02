#ifndef PROGRAMSCONTROLLER_H
#define PROGRAMSCONTROLLER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(AppListModel)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(TranslationManager)

class ProgramsController : public QObject
{
    Q_OBJECT

public:
    explicit ProgramsController(FortManager *fortManager,
                                QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    FirewallConf *conf() const;
    AppListModel *appListModel() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();

public slots:
    void closeWindow();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // PROGRAMSCONTROLLER_H
