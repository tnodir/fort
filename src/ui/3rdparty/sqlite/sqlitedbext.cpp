#include "sqlitedbext.h"

#include <QLoggingCategory>
#include <QRegularExpression>

#include <sqlite.h>

#include "sqlitedb.h"

namespace {

const QLoggingCategory LC("dbExt");

QString valueTextUtf8(sqlite3_value *value)
{
    const unsigned char *textUtf8 = sqlite3_value_text(value);
    return QString::fromUtf8(textUtf8);
}

void extLower(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    Q_ASSERT(argc == 1);

    const auto textLower = valueTextUtf8(argv[0]).toLower();
    const auto result = textLower.toUtf8();

    sqlite3_result_text(ctx, result.data(), result.size(), SQLITE_TRANSIENT);
}

bool createExtLower(struct sqlite3 *db)
{
    return sqlite3_create_function(db, "LOWER",
                   /*nArg=*/1,
                   /*eTextRep=*/(SQLITE_DETERMINISTIC | SQLITE_UTF8),
                   /*pApp=*/nullptr,
                   /*xFunc=*/&extLower,
                   /*xStep=*/nullptr,
                   /*xFinal=*/nullptr)
            == SQLITE_OK;
}

void extRegexp(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    Q_ASSERT(argc == 2);

    const auto pattern = valueTextUtf8(argv[0]);
    const auto text = valueTextUtf8(argv[1]);

    const QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(text);

    sqlite3_result_int(ctx, match.hasMatch());
}

bool createExtRegexp(struct sqlite3 *db)
{
    return sqlite3_create_function(db, "REGEXP",
                   /*nArg=*/2,
                   /*eTextRep=*/(SQLITE_DETERMINISTIC | SQLITE_UTF8),
                   /*pApp=*/nullptr,
                   /*xFunc=*/&extRegexp,
                   /*xStep=*/nullptr,
                   /*xFinal=*/nullptr)
            == SQLITE_OK;
}

}

void SqliteDbExt::registerExtensions(SqliteDb *sqliteDb)
{
    struct sqlite3 *db = sqliteDb->db();

    createExtLower(db);
    createExtRegexp(db);
}
