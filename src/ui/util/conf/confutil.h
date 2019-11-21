#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QVector>

#include "addressrange.h"

QT_FORWARD_DECLARE_CLASS(AddressGroup)
QT_FORWARD_DECLARE_CLASS(AppGroup)
QT_FORWARD_DECLARE_CLASS(EnvManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)

QT_FORWARD_DECLARE_STRUCT(fort_traf)

using longs_arr_t = QVector<quint32>;
using shorts_arr_t = QVector<quint16>;
using chars_arr_t = QVector<qint8>;

using addrranges_arr_t = QVarLengthArray<AddressRange, 2>;

using appperms_map_t = QMap<QString, quint32>;
using appgroups_map_t = QMap<QString, qint8>;

class ConfUtil : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ConfUtil(QObject *parent = nullptr);

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();

public slots:
    int write(const FirewallConf &conf, EnvManager &envManager, QByteArray &buf);
    int writeFlags(const FirewallConf &conf, QByteArray &buf);
    int writeVersion(QByteArray &buf);

private:
    void setErrorMessage(const QString &errorMessage);

    bool parseAddressGroups(const QList<AddressGroup *> &addressGroups,
                            addrranges_arr_t &addressRanges,
                            longs_arr_t &addressGroupOffsets,
                            quint32 &addressGroupsSize);

    // Convert app. groups to plain lists
    bool parseAppGroups(EnvManager &envManager,
                        const QList<AppGroup *> &appGroups,
                        QStringList &appPaths,
                        quint32 &appPathsLen,
                        longs_arr_t &appPerms,
                        chars_arr_t &appPeriods,
                        quint8 &appPeriodsCount,
                        appgroups_map_t &appGroupIndexes);

    bool parseApps(const QString &text, bool blocked,
                   appperms_map_t &appPermsMap,
                   appgroups_map_t &appGroupIndexes,
                   int groupOffset);

    static QString parseAppPath(const QStringRef &line);

    static void writeData(char *output, const FirewallConf &conf,
                          const addrranges_arr_t &addressRanges,
                          const longs_arr_t &addressGroupOffsets,
                          const QStringList &appPaths,
                          const longs_arr_t &appPerms,
                          const chars_arr_t &appPeriods,
                          quint8 appPeriodsCount,
                          const appgroups_map_t &appGroupIndexes);

    static void writeFragmentBits(quint16 *fragmentBits,
                                  const FirewallConf &conf);

    static void writeLimits(struct fort_traf *limits,
                            quint16 *limitBits, quint32 *limit2Bits,
                            const QList<AppGroup *> &appGroups);

    static void writeAddressRanges(char **data,
                                   const addrranges_arr_t &addressRanges);
    static void writeAddressRange(char **data,
                                  const AddressRange &addressRange);

    static void writeShorts(char **data, const shorts_arr_t &array);
    static void writeLongs(char **data, const longs_arr_t &array);
    static void writeNumbers(char **data, void const *src,
                             int elemCount, uint elemSize);
    static void writeChars(char **data, const chars_arr_t &array);
    static void writeStrings(char **data, const QStringList &list);

private:
    QString m_errorMessage;
};

#endif // CONFUTIL_H
