#ifndef ABOUTPAGE_H
#define ABOUTPAGE_H

#include <QDateTime>

#include "homebasepage.h"

class AboutPage : public HomeBasePage
{
    Q_OBJECT

public:
    explicit AboutPage(HomeController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void retranslateNewVersionBox();

    void setupUi();
    void setupNewVersionBox();
    QLayout *setupButtonsLayout();
    void setupNewVersionUpdate();
    void setupAutoUpdate();

private:
    bool m_keepCurrentVersion = false;
    bool m_isNewVersion = false;
    QDateTime m_lastCheckTime;

    QGroupBox *m_gbNewVersion = nullptr;
    QLabel *m_labelRelease = nullptr;
    QWidget *m_labelArea = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QToolButton *m_btDownload = nullptr;
    QToolButton *m_btInstall = nullptr;
    QToolButton *m_btCheckUpdate = nullptr;
};

#endif // ABOUTPAGE_H
