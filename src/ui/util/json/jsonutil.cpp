#include "jsonutil.h"

#include <QJsonDocument>

#include <util/fileutil.h>

QVariant JsonUtil::jsonToVariant(const QByteArray &data, QString &errorString)
{
    QJsonParseError jsonParseError {};
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        errorString = jsonParseError.errorString();
        return QVariant();
    }

    return jsonDoc.toVariant();
}

QVariant JsonUtil::fileToVariant(const QString &filePath, QString &errorString)
{
    // A missing file's empty data is a parse error too
    return jsonToVariant(FileUtil::readFileData(filePath), errorString);
}
