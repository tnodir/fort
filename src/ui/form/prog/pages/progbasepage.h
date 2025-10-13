#ifndef PROGBASEPAGE_H
#define PROGBASEPAGE_H

#include <QFrame>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QActionGroup)
QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDateTimeEdit)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTabBar)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class App;
class ProgramEditController;

class ProgBasePage : public QFrame
{
    Q_OBJECT

public:
    explicit ProgBasePage(ProgramEditController *ctrl, QWidget *parent = nullptr);

    ProgramEditController *ctrl() const { return m_ctrl; }

    const App &app() const;

    bool isWildcard() const;
    bool isNew() const;
    bool isSingleSelection() const;

public slots:
    virtual void onPageActivated() { }

    virtual bool validateFields() const { return true; }
    virtual void fillApp(App & /*app*/) const { }

protected slots:
    virtual void onPageInitialize(const App & /*app*/) { }

    virtual void onRetranslateUi() { }

private:
    void setupController();

private:
    ProgramEditController *m_ctrl = nullptr;
};

#endif // PROGBASEPAGE_H
