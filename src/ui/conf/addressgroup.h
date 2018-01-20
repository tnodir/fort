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

    bool includeAll() const { return m_includeAll; }
    void setIncludeAll(bool includeAll);

    bool excludeAll() const { return m_excludeAll; }
    void setExcludeAll(bool excludeAll);

    QString includeText() const { return m_includeText; }
    void setIncludeText(const QString &includeText);

    QString excludeText() const { return m_excludeText; }
    void setExcludeText(const QString &excludeText);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

signals:
    void includeAllChanged();
    void excludeAllChanged();
    void includeTextChanged();
    void excludeTextChanged();

public slots:

private:
    uint m_includeAll   : 1;
    uint m_excludeAll   : 1;

    QString m_includeText;
    QString m_excludeText;
};

#endif // ADDRESSGROUP_H
