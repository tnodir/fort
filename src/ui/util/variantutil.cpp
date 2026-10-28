#include "variantutil.h"

namespace {

const char *const UserDataKey = "userData";

}

QStringList VariantUtil::listToStringList(const QVariantList &list, Qt::SplitBehavior behavior)
{
    QStringList res;

    for (const QVariant &v : list) {
        const auto s = v.toString();
        if (s.isEmpty() && behavior == Qt::SkipEmptyParts)
            continue;

        res.append(s);
    }

    return res;
}

bool VariantUtil::parseValue(const QString &text, QVariant &value)
{
    switch (value.metaType().id()) {
    case QMetaType::Bool: {
        return parseBool(text, value);
    }
    case QMetaType::QString: {
        value = text;
        return true;
    }
    default:
        return parseNumber(text, value);
    }
}

bool VariantUtil::parseBool(const QString &text, QVariant &value)
{
    static const QStringList boolTexts = { "0", "1", "false", "true" };

    const int index = boolTexts.indexOf(text);
    if (index < 0)
        return false;

    value = bool(index % 2);

    return true;
}

bool VariantUtil::parseNumber(const QString &text, QVariant &value)
{
    bool ok;
    const qint64 number = text.toLongLong(&ok);
    if (!ok)
        return false;

    // Check the number's range by the value's type
    QVariant v(number);
    if (!v.convert(value.metaType()) || v.toLongLong() != number)
        return false;

    value = v;

    return true;
}

QString VariantUtil::toText(const QVariant &value)
{
    switch (value.metaType().id()) {
    case QMetaType::Bool:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar: {
        return QString::number(value.toInt());
    }
    default:
        return value.toString();
    }
}

QVariant VariantUtil::userData(QObject *o)
{
    return o->property(UserDataKey);
}

void VariantUtil::setUserData(QObject *o, const QVariant &v)
{
    o->setProperty(UserDataKey, v);
}
