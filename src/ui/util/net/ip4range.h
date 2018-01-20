#ifndef IP4RANGE_H
#define IP4RANGE_H

#include <QObject>
#include <QMap>
#include <QVector>

typedef struct {
    quint32 from, to;
} Ip4Pair;

typedef QMap<quint32, quint32> ip4range_map_t;
typedef QVector<quint32> ip4range_arr_t;

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

    const ip4range_arr_t &fromArray() const { return m_fromArray; }
    const ip4range_arr_t &toArray() const { return m_toArray; }

    const int size() const { return m_toArray.size(); }
    const Ip4Pair at(int i) const {
        return Ip4Pair{m_fromArray.at(i), m_toArray.at(i)};
    }

signals:
    void errorLineNoChanged();
    void errorMessageChanged();

public slots:
    void clear();

    QString toText();

    // Parse IPv4 ranges from text
    bool fromText(const QString &text);

private:
    void setErrorLineNo(int lineNo);
    void setErrorMessage(const QString &errorMessage);

    bool parseAddressMask(const QStringRef &line,
                          quint32 &from, quint32 &to);

    void fillRange(const ip4range_map_t &ipRangeMap);

private:
    int m_errorLineNo;
    QString m_errorMessage;

    ip4range_arr_t m_fromArray;
    ip4range_arr_t m_toArray;
};

#endif // IP4RANGE_H
