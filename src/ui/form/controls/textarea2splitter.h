#ifndef TEXTAREA2SPLITTER_H
#define TEXTAREA2SPLITTER_H

#include <QSplitter>

QT_FORWARD_DECLARE_CLASS(TextArea2SplitterHandle)

class TextArea2Splitter : public QSplitter
{
    Q_OBJECT

public:
    explicit TextArea2Splitter(QWidget *parent = nullptr);

    bool selectFileEnabled() const { return m_selectFileEnabled; }
    void setSelectFileEnabled(bool v) { m_selectFileEnabled = v; }

    TextArea2SplitterHandle *handle() const;

protected:
    QSplitterHandle *createHandle() override;

private:
    void setupUi();

private:
    bool m_selectFileEnabled = false;
};

#endif // TEXTAREA2SPLITTER_H
