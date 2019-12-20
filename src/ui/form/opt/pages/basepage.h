#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QFrame>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(OptionsController)

class BasePage : public QFrame
{
    Q_OBJECT

public:
    explicit BasePage(OptionsController *ctrl,
                      QWidget *parent = nullptr);

protected:
    OptionsController *ctrl() { return m_ctrl; }
    FortSettings *settings();
    FirewallConf *conf();

protected slots:
    virtual void onEditResetted() {}
    virtual void onAboutToSave() {}
    virtual void onSaved() {}

    virtual void onRetranslateUi() {}

protected:
    static QCheckBox *createCheckBox(bool checked,
                                     const std::function<void (bool checked)> &onToggled);
    static QComboBox *createComboBox(const QStringList &texts,
                                     const std::function<void (int index)> &onActivated);

private:
    void setupController();

private:
    OptionsController *m_ctrl = nullptr;
};

#endif // BASEPAGE_H
