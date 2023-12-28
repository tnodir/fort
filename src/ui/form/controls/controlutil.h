#ifndef CONTROLUTIL_H
#define CONTROLUTIL_H

#include <QObject>
#include <QVector>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLayout)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class LabelColor;
class LabelDoubleSpin;
class LabelSpin;
class LabelSpinCombo;

class ControlUtil
{
public:
    static QCheckBox *createCheckBox(
            bool checked, const std::function<void(bool checked)> &onToggled);

    static QSpinBox *createSpinBox();

    static QComboBox *createComboBox(const QStringList &texts = {});
    static QComboBox *createComboBox(
            const QStringList &texts, const std::function<void(int index)> &onActivated);

    static void setComboBoxTexts(
            QComboBox *c, const QStringList &texts = {}, int currentIndex = -2);

    static QPushButton *createButton(const QString &iconPath, const QString &text = QString());
    static QPushButton *createButton(
            const QString &iconPath, const std::function<void()> &onClicked);

    static QToolButton *createSideButton(
            const QString &iconPath, const std::function<void()> &onClicked);

    static QToolButton *createToolButton(const QString &iconPath);
    static QToolButton *createToolButton(
            const QString &iconPath, const std::function<void()> &onClicked);

    static QToolButton *createFlatToolButton(const QString &iconPath);
    static QToolButton *createFlatToolButton(
            const QString &iconPath, const std::function<void()> &onClicked);

    static QToolButton *createIconToolButton(const QString &iconPath);

    static QToolButton *createSplitterButton(
            const QString &iconPath, const std::function<void()> &onClicked);

    static QLabel *createLabel(const QString &text = QString());
    static QLineEdit *createLineLabel();
    static QLineEdit *createLineEdit(
            const QString &text, const std::function<void(const QString &text)> &onChanged);

    static QMenu *createMenu(QWidget *parent = nullptr);
    static QMenu *createMenuByLayout(QBoxLayout *layout, QWidget *parent);

    static QBoxLayout *createLayoutByWidgets(
            const QList<QWidget *> &widgets, Qt::Orientation o = Qt::Vertical);

    static QFrame *createSeparator(Qt::Orientation o = Qt::Horizontal);

    static QLayout *createRowLayout(QWidget *w1, QWidget *w2, int stretch1 = 1);

    static QLayout *createScrollLayout(QLayout *content, bool isBgTransparent = true);
    static QWidget *wrapToScrollArea(QWidget *content, bool isBgTransparent = true);

    static QFont fontDemiBold();

    static LabelSpinCombo *createSpinCombo(int v, int min, int max, const QVector<int> &values,
            const QString &suffix, const std::function<void(int value)> &onValueChanged);
    static LabelSpin *createSpin(int v, int min, int max, const QString &suffix,
            const std::function<void(int value)> &onValueChanged);
    static LabelDoubleSpin *createDoubleSpin(double v, double min, double max,
            const QString &suffix, const std::function<void(double value)> &onValueChanged);
    static LabelColor *createLabelColor(
            const QColor &v, const std::function<void(const QColor &color)> &onColorChanged);
};

#endif // CONTROLUTIL_H
