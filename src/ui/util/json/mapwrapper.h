#ifndef JSONWRAPPER_H
#define JSONWRAPPER_H

#include <QColor>
#include <QVariantMap>

class MapWrapper
{
public:
    explicit MapWrapper() = default;
    explicit MapWrapper(const QVariant &var);
    explicit MapWrapper(const MapWrapper &o);
    virtual ~MapWrapper() = default;

    const QVariantMap &map() const { return m_map; }
    void setMap(const QVariantMap &map) { m_map = map; }

    void clear();

protected:
    int valueInt(const QString &key, int defaultValue = 0) const;
    bool valueBool(const QString &key, bool defaultValue = false) const;
    uint valueUInt(const QString &key, int defaultValue = 0) const;
    qreal valueReal(const QString &key, qreal defaultValue = 0) const;
    QString valueText(const QString &key, const QString &defaultValue = QString()) const;
    QStringList valueList(const QString &key) const;
    QVariantMap valueMap(const QString &key) const;
    QByteArray valueByteArray(const QString &key) const;

    QColor valueColor(const QString &key, const QColor &defaultValue = QColor()) const;
    void setColor(const QString &key, const QColor &v);

    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    virtual void setValue(
            const QString &key, const QVariant &v, const QVariant &defaultValue = QVariant());

    bool contains(const QString &key) const;

private:
    QVariantMap m_map;
};

#endif // JSONWRAPPER_H
