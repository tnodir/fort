#include "textrange.h"

TextRange::TextRange(QObject *parent) : ValueRange(parent) { }

bool TextRange::fromList(const StringViewList &list, bool /*sort*/)
{
    clear();

    int lineNo = 0;
    for (const auto &line : list) {
        ++lineNo;

        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        const auto text = line.toString().toUpper();

        if (parseText(text) != ErrorOk) {
            appendErrorDetails(QString("line='%1'").arg(line));
            setErrorLineNo(lineNo);
            return false;
        }
    }

    return true;
}
