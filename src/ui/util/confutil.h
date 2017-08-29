#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QMap>

class AppGroup;
class FirewallConf;
class Ip4Range;

typedef QMap<QString, quint32> appperms_map_t;
typedef QVector<quint32> appperms_arr_t;

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
    bool write(const FirewallConf &conf, QByteArray &buf);

private:
    void setErrorMessage(const QString &errorMessage);

    // Convert app. groups to plain lists
    bool parseAppGroups(const QList<AppGroup *> &appGroups,
                        quint32 &groupBits,
                        QStringList &groupNames,
                        int &groupNamesLen,
                        QStringList &appPaths,
                        int &appPathsLen,
                        appperms_arr_t &appPerms);

    bool parseApps(const QString &text, bool blocked,
                   appperms_map_t &appPermsMap, int groupOffset);

    static QString parseAppPath(const QStringRef &line);

    static void writeData(char *output, const FirewallConf &conf,
                          const Ip4Range &incRange, const Ip4Range &excRange,
                          const QStringList &groupNames, const QStringList &appPaths,
                          const appperms_arr_t &appPerms, quint32 groupBits);

    static void writeNumbers(char **data, const QVector<quint32> &array);
    static void writeStrings(char **data, const QStringList &list);

private:
    QString m_errorMessage;
};

#endif // CONFUTIL_H
