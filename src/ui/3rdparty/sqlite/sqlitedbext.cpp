#include "sqlitedbext.h"

#include <QLoggingCategory>

#include <sqlite.h>

#include "sqlitedb.h"

namespace {

const QLoggingCategory LC("dbExt");

void extLower(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    Q_ASSERT(argc == 1);

    const unsigned char *textUtf8 = sqlite3_value_text(argv[0]);
    const auto text = QString::fromUtf8(textUtf8);
    const auto textLower = text.toLower();
    const auto result = textLower.toUtf8();

    sqlite3_result_text(ctx, result.data(), result.size(), SQLITE_TRANSIENT);
}

bool createExtLower(struct sqlite3 *db)
{
    return sqlite3_create_function(db, "EXT_LOWER",
                   /*nArg=*/1,
                   /*eTextRep=*/(SQLITE_DETERMINISTIC | SQLITE_UTF8),
                   /*pApp=*/nullptr,
                   /*xFunc=*/&extLower,
                   /*xStep=*/nullptr,
                   /*xFinal=*/nullptr)
            == SQLITE_OK;
}

}

void SqliteDbExt::registerExtensions(SqliteDb *sqliteDb)
{
    struct sqlite3 *db = sqliteDb->db();

    createExtLower(db);
}
