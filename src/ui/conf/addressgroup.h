#ifndef ADDRESSGROUP_H
#define ADDRESSGROUP_H

#include <QObject>
#include <QVariant>

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

    void copy(const AddressGroup &o);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

signals:
    void includeAllChanged();
    void excludeAllChanged();
    void includeTextChanged();
    void excludeTextChanged();

public slots:

private:
    uint m_edited       : 1;

    uint m_includeAll   : 1;
    uint m_excludeAll   : 1;

    qint64 m_id;

    QString m_includeText;
    QString m_excludeText;
};

#endif // ADDRESSGROUP_H
