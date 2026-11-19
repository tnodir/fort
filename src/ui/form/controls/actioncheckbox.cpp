#include "actioncheckbox.h"

#include <QAction>

ActionCheckBox::ActionCheckBox(QAction *action, QWidget *parent) :
    QCheckBox(parent), m_action(action)
{
    updateState();

    connect(m_action, &QAction::changed, this, &ActionCheckBox::updateState);
    connect(this, &QCheckBox::clicked, m_action, &QAction::trigger);
}

void ActionCheckBox::updateState()
{
    QString text = m_action->iconText();
    text.replace('&', "&&");

    setText(text);
    setToolTip(m_action->toolTip());
    setEnabled(m_action->isEnabled());
    setChecked(m_action->isChecked());
}
