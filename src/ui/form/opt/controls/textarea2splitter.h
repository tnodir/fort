#ifndef TEXTAREA2SPLITTER_H
#define TEXTAREA2SPLITTER_H

#include <QSplitter>

QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(TextArea2SplitterHandle)
QT_FORWARD_DECLARE_CLASS(OptionsController)

class TextArea2Splitter : public QSplitter
{
    Q_OBJECT

public:
    explicit TextArea2Splitter(OptionsController *ctrl,
                               QWidget *parent = nullptr);

    bool selectFileEnabled() const { return m_selectFileEnabled; }
    void setSelectFileEnabled(bool v) { m_selectFileEnabled = v; }

    const char *settingsPropName() const { return m_settingsPropName; }
    void setSettingsPropName(const char *v) { m_settingsPropName = v; }

    OptionsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;

    TextArea2SplitterHandle *handle() const;

protected:
    QSplitterHandle *createHandle() override;

private:
    void setupUi();
    void setupState();

private:
    bool m_selectFileEnabled = false;

    const char *m_settingsPropName;

    OptionsController *m_ctrl = nullptr;
};

#endif // TEXTAREA2SPLITTER_H
