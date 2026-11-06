#ifndef ZONESSELECTOR_H
#define ZONESSELECTOR_H

#include "maskselector.h"

class ZonesSelector : public MaskSelector
{
    Q_OBJECT

public:
    explicit ZonesSelector(QWidget *parent = nullptr);

    void retranslateUi() override;

protected:
    QString titleText() const override;
    MaskSelectorItem itemAt(int row) const override;
};

#endif // ZONESSELECTOR_H
