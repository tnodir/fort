#ifndef MASKSELECTOR_H
#define MASKSELECTOR_H

#include <form/controls/pushbutton.h>

QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
QT_FORWARD_DECLARE_CLASS(QBoxLayout)

struct MaskSelectorItem
{
    int id = 0;
    QString text;
};

class MaskSelector : public PushButton
{
    Q_OBJECT

public:
    bool isTristate() const { return m_isTristate; }
    void setIsTristate(bool isTristate) { m_isTristate = isTristate; }

    quint32 mask() const { return m_mask; }
    void setMask(quint32 mask);

    quint32 uncheckedMask() const { return m_uncheckedMask; }
    void setUncheckedMask(quint32 uncheckedMask);

    virtual void retranslateUi();

signals:
    void maskChanged();

protected:
    explicit MaskSelector(
            QAbstractItemModel *listModel, const QString &iconPath, QWidget *parent = nullptr);

    void setMaxCount(int maxCount) { m_maxCount = maxCount; }

    virtual QString titleText() const = 0;
    virtual MaskSelectorItem itemAt(int row) const = 0;

    void retranslateMaskText();

    bool removeCheckedId(int id);

private:
    void setupUi(const QString &iconPath);
    void setupListModel();

    void resetMenu();
    void clearMenu();
    void createMenu();
    void updateMenu();
    void updateMenuEnabled();

    void onCheckBoxClicked();

private:
    bool m_isTristate = false;

    int m_maxCount = 0;

    quint32 m_mask = 0;
    quint32 m_uncheckedMask = 0;

    QAbstractItemModel *m_listModel = nullptr;

    QBoxLayout *m_listLayout = nullptr;
    QMenu *m_menu = nullptr;
};

#endif // MASKSELECTOR_H
