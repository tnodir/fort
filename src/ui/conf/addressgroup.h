#ifndef ADDRESSGROUP_H
#define ADDRESSGROUP_H

#include <QObject>
#include <QVariant>
#include <QVector>

class AddressGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool includeAll READ includeAll WRITE setIncludeAll NOTIFY includeAllChanged)
    Q_PROPERTY(bool excludeAll READ excludeAll WRITE setExcludeAll NOTIFY excludeAllChanged)
    Q_PROPERTY(QString includeText READ includeText WRITE setIncludeText NOTIFY includeTextChanged)
    Q_PROPERTY(QString excludeText READ excludeText WRITE setExcludeText NOTIFY excludeTextChanged)

public:
    explicit AddressGroup(QObject *parent = nullptr);

    bool edited() const { return m_edited; }
    void setEdited(bool edited) { m_edited = edited; }

    bool includeAll() const { return m_includeAll; }
    void setIncludeAll(bool includeAll);

    bool excludeAll() const { return m_excludeAll; }
    void setExcludeAll(bool excludeAll);

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString includeText() const { return m_includeText; }
    void setIncludeText(const QString &includeText);

    QString excludeText() const { return m_excludeText; }
    void setExcludeText(const QString &excludeText);

    const QVector<int> &includeZones() const { return m_includeZones; }
    const QVector<int> &excludeZones() const { return m_excludeZones; }

    void addIncludeZone(int zoneId, bool sorting = false);
    void removeIncludeZone(int zoneId);

    void addExcludeZone(int zoneId, bool sorting = false);
    void removeExcludeZone(int zoneId);

    void copy(const AddressGroup &o);

signals:
    void includeAllChanged();
    void excludeAllChanged();
    void includeTextChanged();
    void excludeTextChanged();

private:
    void addZone(QVector<int> &zones, int zoneId, bool sorting);
    void removeZone(QVector<int> &zones, int zoneId);

private:
    bool m_edited       : 1;

    bool m_includeAll   : 1;
    bool m_excludeAll   : 1;

    qint64 m_id = 0;

    QString m_includeText;
    QString m_excludeText;

    QVector<int> m_includeZones;
    QVector<int> m_excludeZones;
};

#endif // ADDRESSGROUP_H
