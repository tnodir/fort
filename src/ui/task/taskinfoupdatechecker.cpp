#include "taskinfoupdatechecker.h"

#include <QDataStream>
#include <QTextDocument>

#include "../../common/version.h"
#include "../fortmanager.h"
#include "taskupdatechecker.h"

#define TASK_INFO_VERSION   1

TaskInfoUpdateChecker::TaskInfoUpdateChecker(QObject *parent) :
    TaskInfo(UpdateChecker, parent)
{
}

QString TaskInfoUpdateChecker::plainReleaseText() const
{
    QTextDocument doc;
    doc.setMarkdown(releaseText());
    return doc.toPlainText();
}

QByteArray TaskInfoUpdateChecker::data() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    // Store data
    const quint16 infoVersion = TASK_INFO_VERSION;

    stream
            << infoVersion
            << m_version
            << m_downloadUrl
            << m_releaseText;

    return data;
}

void TaskInfoUpdateChecker::setData(const QByteArray &data)
{
    QDataStream stream(data);

    // Check version
    quint16 infoVersion;
    stream >> infoVersion;

    if (infoVersion > TASK_INFO_VERSION)
        return;

    // Load data
    stream
            >> m_version
            >> m_downloadUrl
            >> m_releaseText;

    emit versionChanged();
}

bool TaskInfoUpdateChecker::processResult(FortManager *fortManager, bool success)
{
    if (!success) {
        m_version = QString();
        m_releaseText = QString();
        return false;
    }

    const auto updateChecker = static_cast<TaskUpdateChecker *>(taskWorker());

    if (m_version == updateChecker->version())
        return false;

    m_version = updateChecker->version();
    m_downloadUrl = updateChecker->downloadUrl();
    m_releaseText = updateChecker->releaseText();

    emit versionChanged();

    fortManager->showTrayMessage(tr("New version v%1 available!")
                                 .arg(m_version));
    return true;
}
