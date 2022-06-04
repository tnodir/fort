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
        TypeSubPolicy,
    };

    PolicyType policyType = TypeNone;
    bool enabled = true;

    int policyId = 0;

    QString name;
};

#endif // POLICY_H
