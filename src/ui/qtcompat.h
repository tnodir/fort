#ifndef QTCOMPAT_H
#define QTCOMPAT_H

#include <QtGlobal>

namespace QtCompat {

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
template<typename Map>
class KeyValueRange
{
public:
    explicit KeyValueRange(const Map &map) : m_map(map) { }

    auto begin() const { return m_map.constKeyValueBegin(); }
    auto end() const { return m_map.constKeyValueEnd(); }

private:
    const Map &m_map;
};
#endif

// A QMap's or QHash's pairs for "for (const auto &[key, value] : ...)"
template<typename Map>
auto keyValueRange(const Map &map)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    return map.asKeyValueRange();
#else
    return KeyValueRange<Map>(map);
#endif
}

template<typename Map>
void keyValueRange(const Map &&) = delete;

}

#endif // QTCOMPAT_H
