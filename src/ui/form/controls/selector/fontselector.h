#ifndef FONTSELECTOR_H
#define FONTSELECTOR_H

#include <QFont>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QToolButton)

class LineEdit;

class FontSelector : public QWidget
{
    Q_OBJECT

public:
    explicit FontSelector(QWidget *parent = nullptr);

    const QFont &defaultFont() const { return m_defaultFont; }
    void setDefaultFont(const QFont &font);

    const QFont &selectedFont() const { return m_selectedFont; }
    void setSelectedFont(const QFont &font);

    void retranslateUi();

signals:
    void fontSelected(const QFont &font);

private:
    void updateFontText();

    void setupUi();

    void selectFontDialog();

private:
    LineEdit *m_editFont = nullptr;
    QToolButton *m_btSelectFont = nullptr;

    QFont m_defaultFont;
    QFont m_selectedFont;
};

#endif // FONTSELECTOR_H
