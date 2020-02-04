#ifndef JSONWRAPPER_H
#define JSONWRAPPER_H

#include <QObject>
#include <QVariantMap>

class MapWrapper
{
public:
    explicit MapWrapper(const QVariant &var = QVariant());

    const QVariantMap &map() const { return m_map; }

protected:
    int valueInt(const QString &key) const;
    bool valueBool(const QString &key) const;
    QString valueText(const QString &key) const;
    QVariant value(const QString &key) const;

    void setValueInt(const QString &key, int v);
    void setValue(const QString &key, const QVariant &v);

private:
    QVariantMap m_map;
};

#endif // JSONWRAPPER_H
