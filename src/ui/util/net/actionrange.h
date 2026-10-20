#ifndef ACTIONRANGE_H
#define ACTIONRANGE_H

#include <QObject>

#include <common/fortconf.h>

#include "textrange.h"

class ActionRange : public TextRange
{
    Q_OBJECT

public:
    explicit ActionRange(QObject *parent = nullptr);

    quint8 actionTypeId() const { return m_actionTypeId; }

    bool isEmpty() const override;

    void clear() override;

    void toList(QStringList &list) const override;

    /* The action's name in the rule text */
    static QString actionTypeName(quint8 actionTypeId);

    /* The action's id by its name, 0 - unknown */
    static quint8 actionTypeIdByName(const QString &name);

    /* The action's translated text and icon */
    static QString actionTypeText(quint8 actionTypeId);
    static QString actionTypeIconPath(quint8 actionTypeId);

    /* The actions in their order: Allow, Block, Ask, Drop */
    static quint8 actionTypeIdAt(int index); // 0 - out of the range
    static int actionTypeIndex(quint8 actionTypeId); // -1 - not an action
    static QStringList actionTypeTexts();
    static QStringList actionTypeIconPaths();

    void write(ConfData &confData) const override;

protected:
    TextRange::ParseError parseText(const QString &text);

private:
    quint8 m_actionTypeId = 0;
};

#endif // ACTIONRANGE_H
