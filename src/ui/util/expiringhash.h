#ifndef EXPIRINGHASH_H
#define EXPIRINGHASH_H

#include <QDeadlineTimer>
#include <QHash>

#include <optional>

// The values with their deadlines: an expired one is as absent, and is removed by a later insert,
// checking them all once per the purge period
template<typename Key, typename T>
class ExpiringHash
{
public:
    explicit ExpiringHash(qint64 purgePeriodMsecs) :
        m_purgePeriodMsecs(purgePeriodMsecs), m_purgeDeadline(purgePeriodMsecs)
    {
    }

    T value(const Key &key, const T &defaultValue = T()) const
    {
        const auto it = m_hash.constFind(key);
        if (it == m_hash.constEnd() || it->deadline.hasExpired())
            return defaultValue;

        return it->value;
    }

    void insert(const Key &key, const T &value, const QDeadlineTimer &deadline)
    {
        purgeExpired();

        m_hash.insert(key, { .deadline = deadline, .value = value });
    }

    std::optional<T> take(const Key &key)
    {
        const auto it = m_hash.find(key);
        if (it == m_hash.end())
            return {};

        const Entry entry = *it;
        m_hash.erase(it);

        if (entry.deadline.hasExpired())
            return {};

        return entry.value;
    }

    // The expired ones too
    template<typename Func>
    void forEachValue(Func func)
    {
        for (Entry &entry : m_hash) {
            func(entry.value);
        }
    }

private:
    struct Entry
    {
        QDeadlineTimer deadline;
        T value;
    };

    void purgeExpired()
    {
        if (!m_purgeDeadline.hasExpired())
            return;

        m_purgeDeadline.setRemainingTime(m_purgePeriodMsecs);

        m_hash.removeIf([](const typename QHash<Key, Entry>::iterator &it) {
            return it->deadline.hasExpired();
        });
    }

private:
    qint64 m_purgePeriodMsecs = 0;

    QDeadlineTimer m_purgeDeadline;

    QHash<Key, Entry> m_hash;
};

#endif // EXPIRINGHASH_H
