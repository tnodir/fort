#include "sqlitedb.h"

#include <QDataStream>
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QImage>

#include <sqlite3.h>

SqliteDb::SqliteDb() :
    m_db(nullptr)
{
    sqlite3_initialize();
}

SqliteDb::~SqliteDb()
{
    close();

    sqlite3_shutdown();
}

bool SqliteDb::open(const QString &filePath)
{
    return sqlite3_open16(filePath.utf16(), &m_db) == SQLITE_OK;
}

void SqliteDb::close()
{
    if (m_db != nullptr) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SqliteDb::execute(const char *sql)
{
    return sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool SqliteDb::execute16(const ushort *sql)
{
    sqlite3_stmt *stmt = nullptr;
    int res = sqlite3_prepare16_v2(db(), sql, -1, &stmt, nullptr);
    if (stmt != nullptr) {
        res = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return res;
}

bool SqliteDb::executeStr(const QString &sql)
{
    return execute16(sql.utf16());
}

QVariant SqliteDb::executeEx(const char *sql,
                             const QVariantList &vars,
                             int resultCount,
                             bool *ok)
{
    QVariantList list;

    QStringList bindTexts;
    QList<QByteArray> bindDatas;

    sqlite3_stmt *stmt = nullptr;
    int res;

    res = sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr);
    if (res != SQLITE_OK)
        goto end;

    // Bind variables
    if (!vars.isEmpty()) {
        int index = 0;
        for (const QVariant &v : vars) {
            ++index;

            const int vType = v.type();

            switch (vType) {
            case QVariant::Invalid:
                res = sqlite3_bind_null(stmt, index);
                break;
            case QVariant::Bool:
            case QVariant::Int:
            case QVariant::UInt:
                res = sqlite3_bind_int(stmt, index, v.toInt());
                break;
            case QVariant::LongLong:
            case QVariant::ULongLong:
                res = sqlite3_bind_int64(stmt, index, v.toLongLong());
                break;
            case QVariant::Double:
                res = sqlite3_bind_double(stmt, index, v.toDouble());
                break;
            case QVariant::String: {
                const QString text = v.toString();
                const int bytesCount = text.size() * int(sizeof(wchar_t));

                bindTexts.append(text);

                res = sqlite3_bind_text16(stmt, index, text.utf16(),
                                          bytesCount, SQLITE_STATIC);
                break;
            }
            default: {
                QByteArray data;

                // Write type
                QDataStream stream(&data, QIODevice::WriteOnly);
                stream << vType;

                // Write content
                {
                    QByteArray bufData;

                    QBuffer buf(&bufData);
                    buf.open(QIODevice::WriteOnly);

                    switch (vType) {
                    case QVariant::Image: {
                        const QImage image = v.value<QImage>();
                        image.save(&buf, "PNG");
                        break;
                    }
                    default:
                        Q_UNREACHABLE();
                    }

                    buf.close();
                    stream << bufData;
                }

                const char *bits = data.constData();
                const int bytesCount = data.size();

                bindDatas.append(data);

                res = sqlite3_bind_blob(stmt, index, bits,
                                        bytesCount, SQLITE_STATIC);
            }
            }

            if (res != SQLITE_OK)
                goto end;
        }
    }

    res = sqlite3_step(stmt);

    if (res == SQLITE_ROW) {
        for (int i = 0; i < resultCount; ++i) {
            QVariant v;
            switch (sqlite3_column_type(stmt, i)) {
            case SQLITE_INTEGER:
                v = sqlite3_column_int64(stmt, i);
                break;
            case SQLITE_FLOAT:
                v = sqlite3_column_double(stmt, i);
                break;
            case SQLITE_TEXT:
                v = QString::fromUtf8(reinterpret_cast<const char *>(
                                          sqlite3_column_text(stmt, i)));
                break;
            case SQLITE_BLOB: {
                const char *bits = reinterpret_cast<const char *>(
                            sqlite3_column_blob(stmt, i));
                const int bytesCount = sqlite3_column_bytes(stmt, i);

                QByteArray data(bits, bytesCount);
                QDataStream stream(data);

                // Load type
                int vType;
                stream >> vType;

                // Load content
                {
                    QByteArray bufData;
                    stream >> bufData;

                    switch (vType) {
                    case QVariant::Image: {
                        QImage image;
                        image.loadFromData(bufData, "PNG");
                        v = image;
                        break;
                    }
                    default:
                        Q_UNREACHABLE();
                    }
                }
                break;
            }
            case SQLITE_NULL:
                break;
            default:
                Q_UNREACHABLE();
            }
            list.append(v);
        }
    }

 end:
    if (stmt != nullptr) {
        sqlite3_finalize(stmt);
    }

    if (ok != nullptr) {
        *ok = (res == SQLITE_OK || res == SQLITE_ROW || res == SQLITE_DONE);
    }

    const int listSize = list.size();
    return (listSize == 0) ? QVariant()
                           : (list.size() == 1) ? list.at(0) : list;
}

qint64 SqliteDb::lastInsertRowid() const
{
    return sqlite3_last_insert_rowid(m_db);
}

int SqliteDb::changes() const
{
    return sqlite3_changes(m_db);
}

bool SqliteDb::beginTransaction()
{
    return execute("BEGIN;");
}

bool SqliteDb::endTransaction(bool ok)
{
    return ok ? commitTransaction()
              : rollbackTransaction();
}

bool SqliteDb::commitTransaction()
{
    return execute("COMMIT;");
}

bool SqliteDb::rollbackTransaction()
{
    return execute("ROLLBACK;");
}

bool SqliteDb::beginSavepoint(const char *name)
{
    return (name == nullptr)
            ? execute("SAVEPOINT _;")
            : executeStr(QString("SAVEPOINT %1;").arg(name));
}

bool SqliteDb::releaseSavepoint(const char *name)
{
    return (name == nullptr)
            ? execute("RELEASE _;")
            : executeStr(QString("RELEASE %1;").arg(name));
}

bool SqliteDb::rollbackSavepoint(const char *name)
{
    return (name == nullptr)
            ? execute("ROLLBACK TO _;")
            : executeStr(QString("ROLLBACK TO %1;").arg(name));
}

QString SqliteDb::errorMessage() const
{
    const char *text = sqlite3_errmsg(m_db);

    return QString::fromUtf8(text);
}

int SqliteDb::userVersion()
{
    return executeEx("PRAGMA user_version;").toInt();
}

bool SqliteDb::migrate(const QString &sqlDir, int version,
                       SQLITEDB_MIGRATE_FUNC migrateFunc,
                       void *migrateContext)
{
    // Check version
    const int userVersion = this->userVersion();
    if (userVersion == version)
        return true;

    if (userVersion > version) {
        qWarning() << "SQLite: Cannot open new DB" << userVersion
                   << "from old code" << version;
        return false;
    }

    // Run migration SQL scripts
    QDir dir(sqlDir);
    bool res = true;

    beginTransaction();
    for (int i = userVersion + 1; i <= version; ++i) {
        const QString filePath = dir.filePath(QString::number(i) + ".sql");

        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qWarning() << "SQLite: Cannot open migration file" << filePath
                       << file.errorString();
            res = false;
            break;
        }

        const QByteArray data = file.readAll();
        if (data.isEmpty()) {
            qWarning() << "SQLite: Migration file is empty" << filePath;
            res = false;
            break;
        }

        beginSavepoint();
        if (!execute(data.constData())
                || !(migrateFunc == nullptr
                     || migrateFunc(this, i, migrateContext))) {
            qWarning() << "SQLite: Migration error:" << filePath << errorMessage();
            res = false;
            rollbackSavepoint();
            break;
        }
        releaseSavepoint();
    }
    commitTransaction();

    return res;
}
