#ifndef APPGROUP_H
#define APPGROUP_H

#include <QObject>
#include <QVariant>

class AppGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool periodEnabled READ periodEnabled WRITE setPeriodEnabled NOTIFY periodEnabledChanged)
    Q_PROPERTY(int periodFrom READ periodFrom WRITE setPeriodFrom NOTIFY periodFromChanged)
    Q_PROPERTY(int periodTo READ periodTo WRITE setPeriodTo NOTIFY periodToChanged)
    Q_PROPERTY(bool limitInEnabled READ limitInEnabled WRITE setLimitInEnabled NOTIFY limitInEnabledChanged)
    Q_PROPERTY(bool limitOutEnabled READ limitOutEnabled WRITE setLimitOutEnabled NOTIFY limitOutEnabledChanged)
    Q_PROPERTY(quint32 speedLimitIn READ speedLimitIn WRITE setSpeedLimitIn NOTIFY speedLimitInChanged)
    Q_PROPERTY(quint32 speedLimitOut READ speedLimitOut WRITE setSpeedLimitOut NOTIFY speedLimitOutChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString blockText READ blockText WRITE setBlockText NOTIFY blockTextChanged)
    Q_PROPERTY(QString allowText READ allowText WRITE setAllowText NOTIFY allowTextChanged)

public:
    explicit AppGroup(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool periodEnabled() const { return m_periodEnabled; }
    void setPeriodEnabled(bool periodEnabled);

    int periodFrom() const { return m_periodFrom; }
    void setPeriodFrom(int periodFrom);

    int periodTo() const { return m_periodTo; }
    void setPeriodTo(int periodTo);

    bool limitInEnabled() const { return m_limitInEnabled; }
    void setLimitInEnabled(bool enabled);

    bool limitOutEnabled() const { return m_limitOutEnabled; }
    void setLimitOutEnabled(bool enabled);

    quint32 speedLimitIn() const { return m_speedLimitIn; }
    void setSpeedLimitIn(quint32 limit);

    quint32 speedLimitOut() const { return m_speedLimitOut; }
    void setSpeedLimitOut(quint32 limit);

    QString name() const { return m_name; }
    void setName(const QString &name);

    QString blockText() const { return m_blockText; }
    void setBlockText(const QString &blockText);

    QString allowText() const { return m_allowText; }
    void setAllowText(const QString &allowText);

    QString label() const;

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

signals:
    void enabledChanged();
    void periodEnabledChanged();
    void periodFromChanged();
    void periodToChanged();
    void limitInEnabledChanged();
    void limitOutEnabledChanged();
    void speedLimitInChanged();
    void speedLimitOutChanged();
    void nameChanged();
    void blockTextChanged();
    void allowTextChanged();

public slots:

private:
    uint m_enabled          : 1;

    uint m_periodEnabled    : 1;
    uint m_periodFrom       : 5;
    uint m_periodTo         : 5;

    uint m_limitInEnabled   : 1;
    uint m_limitOutEnabled  : 1;

    // KiBytes per sec.
    quint32 m_speedLimitIn;
    quint32 m_speedLimitOut;

    QString m_name;

    QString m_blockText;
    QString m_allowText;
};

#endif // APPGROUP_H
