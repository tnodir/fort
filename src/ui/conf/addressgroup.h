#ifndef ADDRESSGROUP_H
#define ADDRESSGROUP_H

#include <QObject>
#include <QVariant>

class AddressGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool useAll READ useAll WRITE setUseAll NOTIFY useAllChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    explicit AddressGroup(QObject *parent = nullptr);

    bool useAll() const { return m_useAll; }
    void setUseAll(bool useAll);

    QString text() const { return m_text; }
    void setText(const QString &text);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

signals:
    void useAllChanged();
    void textChanged();

public slots:

private:
    uint m_useAll   : 1;

    QString m_text;
};

#endif // ADDRESSGROUP_H
