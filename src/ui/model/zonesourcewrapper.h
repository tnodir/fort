#ifndef ZONESOURCEWRAPPER_H
#define ZONESOURCEWRAPPER_H

#include <util/json/mapwrapper.h>

class ZoneSourceWrapper : public MapWrapper
{
public:
    enum SourceId {
        SourceText = 0,
        SourceFile,
        SourceWinSpyBlock,
        SourceFireHol_Level1,
        SourceTasix,
    };

    explicit ZoneSourceWrapper(const QVariant &var = {});

    int id() const;
    void setId(int id);

    QString code() const;
    QString title() const;
    QString zoneType() const;
    QString url() const;
    QString formData() const;

    bool isTextInline() const;

    static int idByCode(const QString &code);
};

#endif // ZONESOURCEWRAPPER_H
