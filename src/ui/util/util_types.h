#pragma once

#include <QString>

enum OutputType : qint8 {
    OUT_CONSOLE = 0,
    OUT_STDOUT,
    OUT_STDERR,
    OUT_FILE,
};

using StringViewList = QList<QStringView>;
using TokenizeViewResult = QStringTokenizer<QStringView, QChar>;
