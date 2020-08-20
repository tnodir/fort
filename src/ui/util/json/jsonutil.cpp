#include "jsonutil.h"

#include <QJsonDocument>

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
