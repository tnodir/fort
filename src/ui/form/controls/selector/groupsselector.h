#ifndef GROUPSSELECTOR_H
#define GROUPSSELECTOR_H

#include "maskselector.h"

class GroupsSelector : public MaskSelector
{
    Q_OBJECT

public:
    explicit GroupsSelector(QWidget *parent = nullptr);

    void retranslateUi() override;

protected:
    QString titleText() const override;
    MaskSelectorItem itemAt(int row) const override;
};

#endif // GROUPSSELECTOR_H
