#ifndef ZONEWRAPPER_H
#define ZONEWRAPPER_H

#include <util/json/mapwrapper.h>

class ZoneTypeWrapper : public MapWrapper
{
public:
    enum TypeId {
        TypeGeneric = 0,
        TypeBgp,
    };

    explicit ZoneTypeWrapper(const QVariant &var = {});

    int id() const;
    void setId(int index);

    QString code() const;
    QString title() const;
    QString description() const;
    bool sort() const;
    QString pattern() const;
    int emptyNetMask() const;

    static int idByCode(const QString &code);
};

#endif // ZONEWRAPPER_H
