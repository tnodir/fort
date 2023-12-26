#ifndef APPINFOROW_H
#define APPINFOROW_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class AppInfoCache;

class AppInfoRow : public QWidget
{
    Q_OBJECT

public:
    explicit AppInfoRow(QWidget *parent = nullptr);

    void retranslateUi();

    void refreshAppInfoVersion(const QString &appPath, AppInfoCache *appInfoCache);

private:
    void setupUi();

private:
    QString m_filePath;

    QToolButton *m_btAppCopyPath = nullptr;
    QToolButton *m_btAppOpenFolder = nullptr;
    QLineEdit *m_lineAppPath = nullptr;
    QLabel *m_labelAppProductName = nullptr;
    QLabel *m_labelAppCompanyName = nullptr;
};

#endif // APPINFOROW_H
