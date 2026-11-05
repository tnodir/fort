#include "maskselector.h"

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QMenu>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <util/bitutil.h>
#include <util/iconcache.h>
#include <util/stringutil.h>

namespace {

const char *const idPropertyName = "maskId";

constexpr quint32 getIdMask(int id)
{
    return quint32(1) << (id - 1);
}

constexpr void setIdMask(quint32 &mask, int id, bool on)
{
    mask = on ? (mask | getIdMask(id)) : (mask & ~getIdMask(id));
}

}

MaskSelector::MaskSelector(
        QAbstractItemModel *listModel, const QString &iconPath, QWidget *parent) :
    PushButton(parent), m_listModel(listModel)
{
    setupUi(iconPath);
    setupListModel();
}

void MaskSelector::setMask(quint32 mask)
{
    if (m_mask == mask)
        return;

    m_mask = mask;

    resetMenu();
}

void MaskSelector::setUncheckedMask(quint32 uncheckedMask)
{
    if (m_uncheckedMask == uncheckedMask)
        return;

    m_uncheckedMask = uncheckedMask;

    resetMenu();
}

void MaskSelector::retranslateUi()
{
    retranslateMaskText();
}

void MaskSelector::retranslateMaskText()
{
    QString countText;

    const int count = BitUtil::bitCount(m_mask);
    if (count != 0) {
        countText += QString::number(count);
    }

    const int uncheckedCount = BitUtil::bitCount(m_uncheckedMask);
    if (uncheckedCount != 0) {
        countText += '^' + QString::number(uncheckedCount);
    }

    this->setText(StringUtil::textWithDetails(titleText(), countText));
}

bool MaskSelector::removeCheckedId(int id)
{
    if ((m_mask & getIdMask(id)) == 0)
        return false;

    setIdMask(m_mask, id, false);

    return true;
}

void MaskSelector::setupUi(const QString &iconPath)
{
    setIcon(IconCache::icon(iconPath));

    m_listLayout = new QVBoxLayout();

    m_menu = ControlUtil::createMenuByLayout(m_listLayout, this);
    this->setMenu(m_menu);

    connect(m_menu, &QMenu::aboutToShow, this, &MaskSelector::updateMenu);
}

void MaskSelector::setupListModel()
{
    const auto refreshMenu = [&] {
        clearMenu();
        updateMenuEnabled();
    };

    refreshMenu();

    connect(m_listModel, &QAbstractItemModel::modelReset, this, refreshMenu);
    connect(m_listModel, &QAbstractItemModel::dataChanged, this, refreshMenu);
}

void MaskSelector::resetMenu()
{
    clearMenu();
    retranslateMaskText();
}

void MaskSelector::clearMenu()
{
    m_menu->close();

    ControlUtil::clearLayout(m_listLayout);
}

void MaskSelector::createMenu()
{
    const int count = qMin(m_listModel->rowCount(), m_maxCount);
    for (int row = 0; row < count; ++row) {
        const auto item = itemAt(row);

        auto cb = new QCheckBox(item.text, m_menu);
        cb->setTristate(isTristate());
        cb->setProperty(idPropertyName, item.id);

        connect(cb, &QCheckBox::clicked, this, &MaskSelector::onCheckBoxClicked);

        m_listLayout->addWidget(cb);
    }
}

void MaskSelector::updateMenu()
{
    if (m_listLayout->isEmpty()) {
        createMenu();
    }

    int i = m_listLayout->count();
    while (--i >= 0) {
        auto item = m_listLayout->itemAt(i);
        auto cb = static_cast<QCheckBox *>(item->widget());

        const int id = cb->property(idPropertyName).toInt();
        const quint32 idMask = getIdMask(id);
        const bool checked = (m_mask & idMask) != 0;
        const bool unchecked = (m_uncheckedMask & idMask) != 0;

        cb->setCheckState(checked
                        ? Qt::Checked
                        : (!unchecked && isTristate() ? Qt::PartiallyChecked : Qt::Unchecked));
    }
}

void MaskSelector::updateMenuEnabled()
{
    const bool isItemExist = (m_listModel->rowCount() != 0);

    this->setMenu(isItemExist ? m_menu : nullptr);
}

void MaskSelector::onCheckBoxClicked()
{
    auto cb = qobject_cast<QCheckBox *>(sender());
    const int id = cb->property(idPropertyName).toInt();
    const Qt::CheckState state = cb->checkState();

    setIdMask(m_mask, id, state == Qt::Checked);
    setIdMask(m_uncheckedMask, id, isTristate() && state == Qt::Unchecked);

    emit maskChanged();

    retranslateMaskText();
}
