#ifndef POLICY_H
#define POLICY_H

#include <QObject>

class Policy
{
public:
    enum PolicyType {
        TypeInvalid = -1,
        TypeNone = 0,
        TypePresetLibrary,
        TypePresetApp,
        TypeGlobalBeforeApp,
        TypeGlobalAfterApp,
    };

    PolicyType policyType = TypeNone;
    bool enabled = true;

    int policyId = 0;

    QString name;
    QString description;
};

#endif // POLICY_H
