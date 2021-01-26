#ifndef TEXTAREA2SPLITTER_H
#define TEXTAREA2SPLITTER_H

#include <QSplitter>

class TextArea2SplitterHandle;

class TextArea2Splitter : public QSplitter
{
    Q_OBJECT

public:
    explicit TextArea2Splitter(QWidget *parent = nullptr);

    TextArea2SplitterHandle *handle() const;

protected:
    QSplitterHandle *createHandle() override;

private:
    void setupUi();
};

#endif // TEXTAREA2SPLITTER_H
