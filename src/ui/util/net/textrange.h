#ifndef TEXTRANGE_H
#define TEXTRANGE_H

#include <QObject>

#include "valuerange.h"

class TextRange : public ValueRange
{
    Q_OBJECT

public:
    explicit TextRange(QObject *parent = nullptr);

    int sizeToWrite() const override;

    bool fromList(const StringViewList &list, bool sort = true) override;

protected:
    enum ParseError {
        ErrorOk = 0,
        ErrorBadText,
    };

    virtual TextRange::ParseError parseText(const QString &text) = 0;
};

#endif // TEXTRANGE_H
