#ifndef HOSTRANGE_H
#define HOSTRANGE_H

#include <QByteArray>
#include <QByteArrayView>
#include <QObject>
#include <QSet>

#include "valuerange.h"

class HostRange : public ValueRange
{
    Q_OBJECT

public:
    // Each of them is checked on a name's matching
    static constexpr int patternNamesMax = 1000;

    explicit HostRange(QObject *parent = nullptr);

    int size() const { return m_names.size() + m_wildcardNames.size() + m_patternNames.size(); }

    // A name and a "*." wildcard are parsed as the "." name: the name with its subdomains
    bool withSubdomains() const { return m_withSubdomains; }
    void setWithSubdomains(bool v) { m_withSubdomains = v; }

    // The host() filter's id
    quint32 hostId() const { return m_hostId; }
    void setHostId(quint32 v) { m_hostId = v; }

    const QSet<QString> &names() const { return m_names; }
    const QSet<QString> &wildcardNames() const { return m_wildcardNames; }
    const QSet<QString> &patternNames() const { return m_patternNames; }

    bool isEmpty() const override;

    int sizeToWrite() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    bool fromList(const StringViewList &list, bool sort = true) override;

    void write(ConfData &confData) const override;

    // Lower case, without the root's trailing dot
    static QString normalizedName(QStringView name);

    // The normalized name in Latin-1: the names are ASCII
    static QByteArray normalizedKey(QStringView name);

    // Walks the keys listing the normalized name: the name itself, then the parent domains of
    // its "*." wildcards
    template<typename NameFunc, typename ParentFunc>
    static void walkName(const QByteArray &key, NameFunc &&onName, ParentFunc &&onParent)
    {
        onName(QByteArrayView(key));

        for (int pos = key.indexOf('.'); pos >= 0; pos = key.indexOf('.', pos + 1)) {
            const int parentPos = pos + 1;

            onParent(QByteArrayView(key.constData() + parentPos, key.size() - parentPos));
        }
    }

private:
    enum ParseError {
        ErrorOk = 0,
        ErrorBadName,
        ErrorTooManyPatterns,
    };

    HostRange::ParseError parseName(QStringView line);
    HostRange::ParseError parsePlainName(QString name);
    void addPlainName(const QString &name, bool isWildcard);
    HostRange::ParseError parsePattern(const QString &pattern);

private:
    bool m_withSubdomains = false;

    quint32 m_hostId = 0;

    QSet<QString> m_names;
    QSet<QString> m_wildcardNames; // the parent domains of the "*." wildcards and "." names
    QSet<QString> m_patternNames; // the names with other masks: HostPattern
};

#endif // HOSTRANGE_H
