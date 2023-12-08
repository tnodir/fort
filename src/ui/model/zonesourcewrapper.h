#ifndef ZONESOURCEWRAPPER_H
#define ZONESOURCEWRAPPER_H

#include <util/json/mapwrapper.h>

class ZoneSourceWrapper : public MapWrapper
{
public:
    explicit ZoneSourceWrapper(const QVariant &var = QVariant());

    int index() const;
    void setIndex(int index);

    QString code() const;
    QString title() const;
    QString zoneType() const;
    QString url() const;
    QString formData() const;

    bool isTextInline() const;

    static QString textSourceCode();
};

#endif // ZONESOURCEWRAPPER_H
