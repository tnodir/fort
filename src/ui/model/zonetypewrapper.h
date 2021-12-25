#ifndef ZONEWRAPPER_H
#define ZONEWRAPPER_H

#include <util/json/mapwrapper.h>

class ZoneTypeWrapper : public MapWrapper
{
public:
    explicit ZoneTypeWrapper(const QVariant &var = QVariant());

    int index() const;
    void setIndex(int index);

    QString code() const;
    QString title() const;
    QString description() const;
    bool sort() const;
    QString pattern() const;
    int emptyNetMask() const;
};

#endif // ZONEWRAPPER_H
