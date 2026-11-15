#include "hostpattern.h"

namespace {

char charAt(QByteArrayView s, qsizetype pos)
{
    return (pos < s.size()) ? s[pos] : '\0';
}

// The label at the position: up to the dot or the end
QByteArrayView labelAt(QByteArrayView s, qsizetype pos)
{
    const qsizetype end = s.indexOf('.', pos);

    return s.sliced(pos, (end < 0 ? s.size() : end) - pos);
}

bool matchLabel(QByteArrayView pattern, QByteArrayView label)
{
    qsizetype p = 0;
    qsizetype s = 0;
    qsizetype starP = -1; // the last "*" to retry from
    qsizetype starS = 0;

    while (s < label.size()) {
        const char c = charAt(pattern, p);

        if (c == '*') {
            starP = p++;
            starS = s;
        } else if (c == label[s]) {
            ++p;
            ++s;
        } else if (starP >= 0) {
            p = starP + 1;
            s = ++starS;
        } else {
            return false;
        }
    }

    if (charAt(pattern, p) == '*') {
        ++p;
    }

    return p == pattern.size();
}

// The name has at least as many labels as the pattern
bool matchLabels(QByteArrayView pattern, QByteArrayView name)
{
    qsizetype p = 0;
    qsizetype s = 0;

    while (p <= pattern.size()) {
        const QByteArrayView patternLabel = labelAt(pattern, p);
        const QByteArrayView nameLabel = labelAt(name, s);

        if (!matchLabel(patternLabel, nameLabel))
            return false;

        p += patternLabel.size() + 1;
        s += nameLabel.size() + 1;
    }

    return true;
}

}

bool HostPattern::match(QByteArrayView pattern, QByteArrayView name)
{
    const bool anyPrefix = pattern.startsWith('*');
    const bool anySuffix = pattern.endsWith('*');

    const qsizetype extraLabels = name.count('.') - pattern.count('.');
    if (extraLabels < 0)
        return false;

    // The name's labels to skip: none without the leading "*", all the extra ones without the
    // trailing one
    const qsizetype skipMin = anySuffix ? 0 : extraLabels;
    const qsizetype skipMax = anyPrefix ? extraLabels : 0;

    qsizetype pos = 0;

    for (qsizetype i = 0; i <= skipMax; ++i) {
        if (i >= skipMin && matchLabels(pattern, name.sliced(pos)))
            return true;

        pos = name.indexOf('.', pos) + 1;
    }

    return false;
}
