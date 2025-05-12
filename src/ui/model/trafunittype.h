#ifndef TRAFUNITTYPE_H
#define TRAFUNITTYPE_H

#include <QObject>

class TrafUnitType : public QObject
{
    Q_GADGET

public:
    enum TrafUnit : qint8 {
        UnitAdaptive = 0,
        UnitBytes,
        UnitKB,
        UnitMB,
        UnitGB,
        UnitTB,
        UnitPB,
        UnitEB,
    };
    Q_ENUM(TrafUnit)

    enum TrafType : qint8 {
        TrafHourly = 0,
        TrafDaily,
        TrafMonthly,
        TrafTotal,
    };
    Q_ENUM(TrafType)

    TrafUnitType::TrafUnit unit() const { return m_unit; }
    void setUnit(TrafUnitType::TrafUnit unit) { m_unit = unit; }

    TrafUnitType::TrafType type() const { return m_type; }
    void setType(TrafUnitType::TrafType type) { m_type = type; }

    QString formatTrafUnit(qint64 bytes) const;
    QString formatTrafTime(qint32 trafTime) const;

private:
    TrafUnit m_unit = UnitAdaptive;
    TrafType m_type = TrafHourly;
};

#endif // TRAFUNITTYPE_H
