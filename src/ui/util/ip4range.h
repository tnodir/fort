#ifndef IP4RANGE_H
#define IP4RANGE_H

#include <QObject>
#include <QMap>
#include <QVector>

typedef struct {
    quint32 from, to;
} Ip4Pair;

typedef QMap<quint32, quint32> ip4range_map_t;

class Ip4Range : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int errorLineNo READ errorLineNo NOTIFY errorLineNoChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit Ip4Range(QObject *parent = nullptr);

    int errorLineNo() const { return m_errorLineNo; }
    QString errorMessage() const { return m_errorMessage; }

    const QVector<Ip4Pair> &range() const { return m_range; }

signals:
    void errorLineNoChanged();
    void errorMessageChanged();

public slots:
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

    QVector<Ip4Pair> m_range;
};

#endif // IP4RANGE_H
