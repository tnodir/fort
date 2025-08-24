#ifndef OPTIONRANGE_H
#define OPTIONRANGE_H

#include <QObject>

#include "textrange.h"

class OptionRange : public TextRange
{
    Q_OBJECT

public:
    explicit OptionRange(QObject *parent = nullptr);

    quint8 optionTypeIds() const { return m_optionTypeIds; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    void write(ConfData &confData) const override;

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    quint8 m_optionTypeIds = 0;
};

#endif // OPTIONRANGE_H
