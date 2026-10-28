#ifndef CONTROLCOMMANDUTIL_H
#define CONTROLCOMMANDUTIL_H

#include <QVariant>

#include <control/control_types.h>

class Rule;

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

    // Client: Replace the <file-path> argument with its file's text
    static bool readClientFileArg(QVariantList &args, ProcessCommandResult &r);

    // Client: Write the result's text to the <file-path> argument's file
    static bool writeClientFileArg(const QVariantList &args, ProcessCommandResult &r);

    // The rule of the unique name
    static bool findRuleByName(const QString &ruleName, Rule &rule, QString &errorMessage);

    // The rule's text is valid, else its error message
    static bool validateRuleText(ProcessCommandResult &r, const QString &ruleText);
};

#endif // CONTROLCOMMANDUTIL_H
