#ifndef POLICY_H
#define POLICY_H

#include <QObject>

class Policy
{
public:
    bool isPreset = false;
    bool enabled = true;

    int policyId = 0;

    QString name;
};

#endif // POLICY_H
