#ifndef MASKSELECTOR_H
#define MASKSELECTOR_H

#include <QWidget>

#include <form/form_types.h>

QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class PushButton;

struct MaskSelectorItem
{
    int id = 0;
    QString text;
};

class MaskSelector : public QWidget
{
    Q_OBJECT

public:
    PushButton *menuButton() const { return m_btMenu; }

    void hideListWindowButton();

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
    explicit MaskSelector(QAbstractItemModel *listModel, WindowCode listWindowCode,
            const QString &iconPath, QWidget *parent = nullptr);

    void setMaxCount(int maxCount) { m_maxCount = maxCount; }

    virtual QString titleText() const = 0;
    virtual MaskSelectorItem itemAt(int row) const = 0;

    void retranslateMaskText();

    bool removeCheckedId(int id);

private:
    void setupUi(WindowCode listWindowCode, const QString &iconPath);
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
    PushButton *m_btMenu = nullptr;
    QToolButton *m_btListWindow = nullptr;
};

#endif // MASKSELECTOR_H
