#ifndef UPDATESWINDOW_H
#define UPDATESWINDOW_H

#include <QDateTime>

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QProgressBar)

class UpdatesWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit UpdatesWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowUpdates; }
    QString windowIconPath() const override { return ":/icons/download.png"; }
    QString helpSection() const override { return "updates-window"; }
    QString iniGroup() const override;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void retranslateUi();
    void retranslateNewVersionBox();

    void setupUi();
    void setupNewVersionBox();
    QLayout *setupButtonsLayout();
    void setupNewVersionUpdate();
    void setupAutoUpdate();

    void refreshAutoUpdate();
    void refreshDownloadProgress(bool isDownloading, bool isDownloaded);

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

#endif // UPDATESWINDOW_H
