#ifndef IP4RANGE_H
#define IP4RANGE_H

#include <QMap>
#include <QObject>
#include <QVector>

#include "../../fortcompat.h"

using Ip4Pair = struct {
    quint32 from, to;
};

using ip4range_map_t = QMap<quint32, quint32>;
using ip4_arr_t = QVector<quint32>;

class Ip4Range : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int errorLineNo READ errorLineNo NOTIFY errorLineNoChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString errorLineAndMessage READ errorLineAndMessage NOTIFY errorMessageChanged)

public:
    explicit Ip4Range(QObject *parent = nullptr);

    int errorLineNo() const { return m_errorLineNo; }

    QString errorMessage() const { return m_errorMessage; }
    QString errorLineAndMessage() const;

    const ip4_arr_t &ipArray() const { return m_ipArray; }
    ip4_arr_t &ipArray() { return m_ipArray; }

    const ip4_arr_t &pairFromArray() const { return m_pairFromArray; }
    ip4_arr_t &pairFromArray() { return m_pairFromArray; }

    const ip4_arr_t &pairToArray() const { return m_pairToArray; }
    ip4_arr_t &pairToArray() { return m_pairToArray; }

    int ipSize() const { return m_ipArray.size(); }
    int pairSize() const { return m_pairToArray.size(); }

    bool isEmpty() const { return ipSize() == 0 && pairSize() == 0; }

    quint32 ipAt(int i) const { return m_ipArray.at(i); }
    Ip4Pair pairAt(int i) const {
        return Ip4Pair{m_pairFromArray.at(i), m_pairToArray.at(i)};
    }

    QString toText() const;

    // Parse IPv4 ranges
    bool fromText(const QString &text);
    bool fromList(const StringViewList &list, int emptyNetMask = 32,
                  bool sort = true);

signals:
    void errorLineNoChanged();
    void errorMessageChanged();

public slots:
    void clear();

private:
    void setErrorLineNo(int lineNo);
    void setErrorMessage(const QString &errorMessage);

    bool parseAddressMask(const StringView line,
                          quint32 &from, quint32 &to,
                          int emptyNetMask = 32);

    void fillRange(const ip4range_map_t &ipRangeMap, int pairSize);

private:
    int m_errorLineNo = 0;
    QString m_errorMessage;

    ip4_arr_t m_ipArray;
    ip4_arr_t m_pairFromArray;
    ip4_arr_t m_pairToArray;
};

#endif // IP4RANGE_H
