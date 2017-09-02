#ifndef APPGROUP_H
#define APPGROUP_H

#include <QObject>
#include <QVariant>

class AppGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString blockText READ blockText WRITE setBlockText NOTIFY blockTextChanged)
    Q_PROPERTY(QString allowText READ allowText WRITE setAllowText NOTIFY allowTextChanged)

public:
    explicit AppGroup(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    QString name() const { return m_name; }
    void setName(const QString &name);

    QString blockText() const { return m_blockText; }
    void setBlockText(const QString &blockText);

    QString allowText() const { return m_allowText; }
    void setAllowText(const QString &allowText);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

signals:
    void enabledChanged();
    void nameChanged();
    void blockTextChanged();
    void allowTextChanged();

public slots:

private:
    uint m_enabled  : 1;

    QString m_name;

    QString m_blockText;
    QString m_allowText;
};

#endif // APPGROUP_H
