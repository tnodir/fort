#ifndef MAPSETTINGS_H
#define MAPSETTINGS_H

#include <QRect>

#include <util/json/mapwrapper.h>

class Settings;

class MapSettings : public MapWrapper
{
public:
    explicit MapSettings(Settings *settings = nullptr);
    explicit MapSettings(const MapSettings &o);

    Settings *settings() const { return m_settings; }

    QVariant value(const QString &key, const QVariant &defaultValue = {}) const override;
    void setValue(
            const QString &key, const QVariant &v, const QVariant &defaultValue = {}) override;

    void setCacheValue(const QString &key, const QVariant &v) const;

    static bool isTransientKey(const QString &key) { return key.endsWith('_'); }

public slots:
    void save();
    void saveAndClear();

private:
    Settings *m_settings = nullptr;
};

#endif // MAPSETTINGS_H
