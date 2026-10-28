#ifndef CONTROLCOMMANDUTIL_H
#define CONTROLCOMMANDUTIL_H

#include <QVariant>

#include <control/control_types.h>

class ControlCommandUtil
{
public:
    // The password is checked temporarily: unchecked after the command by its manager
    static bool checkCommandActionPassword(
            ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions = 0);

    static void uncheckCommandActionPassword();

    // Set the fields by the "key=value" arguments from the index:
    // the keys and the values' types are of the fields
    static bool parseFieldArgs(
            const QVariantList &args, int index, QVariantMap &fields, QString &errorMessage);

    // The error message of the field's invalid value
    static QString invalidValueText(const QString &key, const QString &text);

    // The "key=value" pairs separated by spaces, the booleans as 0 or 1
    static QString fieldsToText(const QVariantMap &fields);
};

#endif // CONTROLCOMMANDUTIL_H
