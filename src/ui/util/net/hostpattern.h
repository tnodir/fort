#ifndef HOSTPATTERN_H
#define HOSTPATTERN_H

#include <QByteArrayView>

// A host name's mask: "*" is any characters of a label, the leading and trailing ones are also
// any labels before and after it
class HostPattern
{
public:
    static bool match(QByteArrayView pattern, QByteArrayView name);
};

#endif // HOSTPATTERN_H
