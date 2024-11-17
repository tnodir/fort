#ifndef VALUERANGE_H
#define VALUERANGE_H

#include <QMap>
#include <QObject>
#include <QVector>

#include <util/util_types.h>

template<typename T>
struct ValuePair
{
    T from, to;
};

class ValueRange : public QObject
{
    Q_OBJECT

public:
    explicit ValueRange(QObject *parent = nullptr);

    int errorLineNo() const { return m_errorLineNo; }

    QString errorMessage() const { return m_errorMessage; }
    QString errorDetails() const { return m_errorDetails; }
    QString errorLineAndMessageDetails() const;

    virtual bool isEmpty() const = 0;

    virtual void clear();

    QString toText() const;
    virtual void toList(QStringList &list) const = 0;

    bool fromText(const QString &text);
    virtual bool fromList(const StringViewList &list, bool sort = true) = 0;

protected:
    void setErrorLineNo(int lineNo) { m_errorLineNo = lineNo; }
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }
    void setErrorDetails(const QString &errorDetails) { m_errorDetails = errorDetails; }

    void appendErrorDetails(const QString &errorDetails);

    template<typename T>
    struct FillRangeArraysArgs
    {
        const QMap<T, T> &rangeMap;
        QVector<T> &valuesArray;
        QVector<T> &pairFromArray;
        QVector<T> &pairToArray;
        int pairSize;
    };

    template<typename T>
    static void fillRangeArrays(const FillRangeArraysArgs<T> &fra);

private:
    int m_errorLineNo = 0;
    QString m_errorMessage;
    QString m_errorDetails;
};

template<typename T>
static void ValueRange::fillRangeArrays(const FillRangeArraysArgs<T> &fra)
{
    if (fra.rangeMap.isEmpty())
        return;

    const int mapSize = fra.rangeMap.size();
    fra.valuesArray.reserve(mapSize - fra.pairSize);
    fra.pairFromArray.reserve(fra.pairSize);
    fra.pairToArray.reserve(fra.pairSize);

    ValuePair<T> prevPair;
    int prevIndex = -1;

    auto it = fra.rangeMap.constBegin();
    auto end = fra.rangeMap.constEnd();

    for (; it != end; ++it) {
        const ValuePair<T> v { it.key(), it.value() };

        // try to merge colliding addresses
        if (prevIndex >= 0 && v.from <= prevPair.to + 1) {
            if (v.to > prevPair.to) {
                fra.pairToArray.replace(prevIndex, v.to);

                prevPair.to = v.to;
            }
            // else skip it
        } else if (v.from == v.to) {
            fra.valuesArray.append(v.from);
        } else {
            fra.pairFromArray.append(v.from);
            fra.pairToArray.append(v.to);

            prevPair = v;
            ++prevIndex;
        }
    }
}

#endif // VALUERANGE_H
