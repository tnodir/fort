/*
**  Do shell-style pattern matching for ?, \, [], and * characters.
**  It is 8bit clean.
**
**  Written by Rich $alz, mirror!rs, Wed Nov 26 19:03:17 EST 1986.
**  Rich $alz is now <rsalz@bbn.com>.
**
**  Modified by Wayne Davison to special-case '/' matching, to make '**'
**  work differently than '*', and to fix the character-class code.
**
**  Modified by Nodir Temirkhodjaev to treat '\' as directory separator
**  and to ignore the '/'.
*/

#include "wildmatch.h"

typedef unsigned char uchar;

#define ISASCII(c) ((c) < 128)

#define WM_STRCHR(str, c) ((const WCHAR *) wcschr((const WCHAR *) (str), (c)))

#define DIR_SEP '\\'

/* What character marks an inverted character class? */
#define NEGATE_CLASS  '!'
#define NEGATE_CLASS2 '^'

#define GIT_SPACE          0x01
#define GIT_DIGIT          0x02
#define GIT_ALPHA          0x04
#define GIT_GLOB_SPECIAL   0x08
#define GIT_REGEX_SPECIAL  0x10
#define GIT_PATHSPEC_MAGIC 0x20
#define GIT_CNTRL          0x40
#define GIT_PUNCT          0x80

enum {
    S = GIT_SPACE,
    A = GIT_ALPHA,
    D = GIT_DIGIT,
    G = GIT_GLOB_SPECIAL, /* *, ?, [ */
    R = GIT_REGEX_SPECIAL, /* $, (, ), +, ., ^, {, | */
    P = GIT_PATHSPEC_MAGIC, /* other non-alnum, except for ] and } */
    X = GIT_CNTRL,
    U = GIT_PUNCT,
    Z = GIT_CNTRL | GIT_SPACE
};

const uchar sane_ctype[256] = {
    X, X, X, X, X, X, X, X, X, Z, Z, X, X, Z, X, X, /*   0.. 15 */
    X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, /*  16.. 31 */
    S, P, P, P, R, P, P, P, R, R, G, R, P, P, R, P, /*  32.. 47 */
    D, D, D, D, D, D, D, D, D, D, P, P, P, P, P, G, /*  48.. 63 */
    P, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, /*  64.. 79 */
    A, A, A, A, A, A, A, A, A, A, A, G, P, U, R, P, /*  80.. 95 */
    P, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, /*  96..111 */
    A, A, A, A, A, A, A, A, A, A, A, R, R, U, P, X, /* 112..127 */
    /* Nothing in the 128.. range */
};

#define sane_istest(x, mask) ((sane_ctype[(uchar)(x)] & (mask)) != 0)
#define is_glob_special(x)   (ISASCII(x) && sane_istest(x, GIT_GLOB_SPECIAL))

/* Match the "pattern" against the "text" string. */
WILDMATCH_API int wildmatch(const wm_char *pattern, const wm_char *text)
{
    const wm_char *p = pattern;
    wm_char p_ch;

    for (; (p_ch = *p) != '\0'; text++, p++) {
        int matched, match_slash, negated;
        wm_char t_ch, prev_ch;

        if ((t_ch = *text) == '\0' && p_ch != '*')
            return WM_ABORT_ALL;

        switch (p_ch) {
        default:
            if (t_ch != p_ch)
                return WM_NOMATCH;
            continue;
        case '?':
            /* Match anything but DIR_SEP. */
            if (t_ch == DIR_SEP)
                return WM_NOMATCH;
            continue;
        case '*':
            if (*++p == '*') {
                const wm_char *prev_p = p - 2;
                while (*++p == '*') { }
                if ((prev_p < pattern || *prev_p == DIR_SEP) && (*p == '\0' || *p == DIR_SEP)) {
                    /*
                     * Assuming we already match 'foo/' and are at
                     * <star star slash>, just assume it matches
                     * nothing and go ahead match the rest of the
                     * pattern with the remaining string. This
                     * helps make foo/<*><*>/bar (<> because
                     * otherwise it breaks C comment syntax) match
                     * both foo/bar and foo/a/bar.
                     */
                    if (p[0] == DIR_SEP && wildmatch(p + 1, text) == WM_MATCH)
                        return WM_MATCH;
                    match_slash = 1;
                } else {
                    match_slash = 0;
                }
            } else {
                match_slash = 0;
            }
            if (*p == '\0') {
                /* Trailing "**" matches everything.  Trailing "*" matches
                 * only if there are no more slash characters. */
                if (!match_slash) {
                    if (WM_STRCHR(text, DIR_SEP) != NULL)
                        return WM_NOMATCH;
                }
                return WM_MATCH;
            } else if (!match_slash && *p == DIR_SEP) {
                /*
                 * _one_ asterisk followed by a slash
                 * matches the next directory
                 */
                const wm_char *slash = WM_STRCHR(text, DIR_SEP);
                if (!slash)
                    return WM_NOMATCH;
                text = slash;
                /* the slash is consumed by the top-level for loop */
                break;
            }
            while (1) {
                if (t_ch == '\0')
                    break;
                /*
                 * Try to advance faster when an asterisk is
                 * followed by a literal. We know in this case
                 * that the string before the literal
                 * must belong to "*".
                 * If match_slash is false, do not look past
                 * the first slash as it cannot belong to '*'.
                 */
                if (!is_glob_special(*p)) {
                    p_ch = *p;
                    while ((t_ch = *text) != '\0' && (match_slash || t_ch != DIR_SEP)) {
                        if (t_ch == p_ch)
                            break;
                        text++;
                    }
                    if (t_ch != p_ch)
                        return WM_NOMATCH;
                }
                if ((matched = wildmatch(p, text)) != WM_NOMATCH) {
                    if (!match_slash || matched != WM_ABORT_TO_STARSTAR)
                        return matched;
                } else if (!match_slash && t_ch == DIR_SEP)
                    return WM_ABORT_TO_STARSTAR;
                t_ch = *++text;
            }
            return WM_ABORT_ALL;
        case '[':
            p_ch = *++p;
            /* Assign literal 1/0 because of "matched" comparison. */
            negated = (p_ch == NEGATE_CLASS || p_ch == NEGATE_CLASS2) ? 1 : 0;
            if (negated) {
                /* Inverted character class. */
                p_ch = *++p;
            }
            prev_ch = 0;
            matched = 0;
            do {
                if (!p_ch)
                    return WM_ABORT_ALL;
                if (p_ch == '-' && prev_ch && p[1] && p[1] != ']') {
                    p_ch = *++p;
                    if (t_ch <= p_ch && t_ch >= prev_ch)
                        matched = 1;
                    p_ch = 0; /* This makes "prev_ch" get set to 0. */
                } else if (t_ch == p_ch) {
                    matched = 1;
                }
            } while (prev_ch = p_ch, (p_ch = *++p) != ']');
            if (matched == negated || t_ch == DIR_SEP)
                return WM_NOMATCH;
            continue;
        }
    }

    return *text ? WM_NOMATCH : WM_MATCH;
}
