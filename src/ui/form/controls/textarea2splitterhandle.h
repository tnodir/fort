#ifndef TEXTAREA2SPLITTERHANDLE_H
#define TEXTAREA2SPLITTERHANDLE_H

#include <QSplitterHandle>

QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class TextArea2Splitter;

class TextArea2SplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    explicit TextArea2SplitterHandle(Qt::Orientation o, QSplitter *parent);

    QPlainTextEdit *textArea1() const { return m_textArea1; }
    void setTextArea1(QPlainTextEdit *v) { m_textArea1 = v; }

    QPlainTextEdit *textArea2() const { return m_textArea2; }
    void setTextArea2(QPlainTextEdit *v) { m_textArea2 = v; }

    QPlainTextEdit *currentTextArea() const;

    QToolButton *btMoveAllFrom1To2() const { return m_btMoveAllFrom1To2; }
    QToolButton *btMoveSelectedFrom1To2() const { return m_btMoveSelectedFrom1To2; }
    QToolButton *btInterchangeAll() const { return m_btInterchangeAll; }
    QToolButton *btMoveSelectedFrom2To1() const { return m_btMoveSelectedFrom2To1; }
    QToolButton *btMoveAllFrom2To1() const { return m_btMoveAllFrom2To1; }

    TextArea2Splitter *splitter() const;

    QBoxLayout *buttonsLayout() const { return m_buttonsLayout; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void setupUi();

private:
    QPlainTextEdit *m_textArea1 = nullptr;
    QPlainTextEdit *m_textArea2 = nullptr;

    QBoxLayout *m_buttonsLayout = nullptr;
    QToolButton *m_btMoveAllFrom1To2 = nullptr;
    QToolButton *m_btMoveSelectedFrom1To2 = nullptr;
    QToolButton *m_btInterchangeAll = nullptr;
    QToolButton *m_btMoveSelectedFrom2To1 = nullptr;
    QToolButton *m_btMoveAllFrom2To1 = nullptr;
};

#endif // TEXTAREA2SPLITTERHANDLE_H
