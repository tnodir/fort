#ifndef CONTROLCOMMANDUTIL_H
#define CONTROLCOMMANDUTIL_H

#include <QVariant>

#include <control/control_types.h>

class Rule;

class ControlCommandUtil
{
public:
    // The command's error result, e.g. of its object not found
    static void setErrorResult(ProcessCommandResult &r, const QString &errorMessage);

    // The password is checked temporarily: unchecked after the command by its manager
    static bool checkCommandActionPassword(
            ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions = 0);

    static void uncheckCommandActionPassword();

    // Set the fields by the "key=value" arguments from the index:
    // the keys and the values' types are of the fields;
    // a list is of the ids of a mask, 1-32, separated by commas
    static bool parseFieldArgs(
            const QVariantList &args, int index, QVariantMap &fields, QString &errorMessage);

    // The argument is "key=value" of the fields' key
    static bool isFieldArg(const QString &arg, const QVariantMap &fields);

    // The error message of the field's invalid value
    static QString invalidValueText(const QString &key, const QString &text);

    // The field's value index in the names, -1 when invalid
    static int nameIndex(const QVariantMap &fields, const QString &key, const QStringList &names,
            QString &errorMessage);

    // The ids of a mask, 1-32, separated by commas
    static bool parseIds(const QString &text, QVariant &ids);
    static QString idsText(const QVariant &ids);

    // The mask's ids as a list
    static QVariant maskIds(quint32 mask);
    static quint32 idsMask(const QVariant &ids);

    // The "key=value" pairs separated by spaces, the booleans as 0 or 1,
    // a text with white spaces or quotes in quotes, its quotes doubled
    static QString fieldsToText(const QVariantMap &fields);

    // The text of the fields with the read-only "id", not a key to set
    static QString fieldsToTextWithId(QVariantMap fields, const QVariant &id);

    // Client: Replace the <file-path> argument with its file's text
    static bool readClientFileArg(QVariantList &args, ProcessCommandResult &r);

    // Client: Write the result's text to the <file-path> argument's file
    static bool writeClientFileArg(const QVariantList &args, ProcessCommandResult &r);

    // The rule of the unique name
    static bool findRuleByName(const QString &ruleName, Rule &rule, QString &errorMessage);

    // The empty name is of no rule, 0
    static bool findRuleIdByName(const QString &ruleName, quint16 &ruleId, QString &errorMessage);

    // The rule's text is valid, else its error message
    static bool validateRuleText(ProcessCommandResult &r, const QString &ruleText);
};

#endif // CONTROLCOMMANDUTIL_H
