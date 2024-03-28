#include "sqlitestmt.h"

#include <QBuffer>
#include <QDataStream>
#include <QHash>
#include <QImage>

#include <sqlite.h>

namespace {

bool stmtBindVarNull(SqliteStmt &stmt, int index, const QVariant & /*v*/)
{
    return stmt.bindNull(index);
}

bool stmtBindVarInt(SqliteStmt &stmt, int index, const QVariant &v)
{
    return stmt.bindInt(index, v.toInt());
}

bool stmtBindVarInt64(SqliteStmt &stmt, int index, const QVariant &v)
{
    return stmt.bindInt64(index, v.toLongLong());
}

bool stmtBindVarDouble(SqliteStmt &stmt, int index, const QVariant &v)
{
    return stmt.bindDouble(index, v.toDouble());
}

bool stmtBindVarText(SqliteStmt &stmt, int index, const QVariant &v)
{
    return stmt.bindText(index, v.toString());
}

bool stmtBindVarDateTime(SqliteStmt &stmt, int index, const QVariant &v)
{
    return stmt.bindDateTime(index, v.toDateTime());
}

bool stmtBindVarBlob(SqliteStmt &stmt, int index, const QVariant &v)
{
    return stmt.bindBlob(index, v.toByteArray());
}

bool stmtBindVarDataStream(SqliteStmt &stmt, int index, const QVariant &v)
{
    return stmt.bindDataStream(index, v);
}

using stmtBindVar_func = bool (*)(SqliteStmt &stmt, int index, const QVariant &v);

static QHash<qint16, stmtBindVar_func> stmtBindVar_funcMap = {
    { QMetaType::UnknownType, &stmtBindVarNull },
    { QMetaType::Void, &stmtBindVarNull },
    { QMetaType::Nullptr, &stmtBindVarNull },
    { QMetaType::Bool, &stmtBindVarInt },
    { QMetaType::Int, &stmtBindVarInt },
    { QMetaType::UInt, &stmtBindVarInt },
    { QMetaType::LongLong, &stmtBindVarInt64 },
    { QMetaType::ULongLong, &stmtBindVarInt64 },
    { QMetaType::Double, &stmtBindVarDouble },
    { QMetaType::QString, &stmtBindVarText },
    { QMetaType::QDateTime, &stmtBindVarDateTime },
    { QMetaType::QByteArray, &stmtBindVarBlob },
};

bool stmtBindVar(SqliteStmt &stmt, int index, const QVariant &v)
{
    const qint16 vType = v.userType();
    const stmtBindVar_func func = stmtBindVar_funcMap.value(vType, &stmtBindVarDataStream);

    return func(stmt, index, v);
}

bool checkBindResult(int res)
{
    return res == SQLITE_OK || res == SQLITE_RANGE;
}

}

SqliteStmt::SqliteStmt() = default;

SqliteStmt::~SqliteStmt()
{
    finalize();
}

bool SqliteStmt::prepare(sqlite3 *db, const char *sql, SqliteStmt::PrepareFlags flags)
{
    return sqlite3_prepare_v3(db, sql, /*nByte=*/-1, flags, &m_stmt, nullptr) == SQLITE_OK;
}

void SqliteStmt::finalize()
{
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

QString SqliteStmt::expandedSql() const
{
    char *sql = sqlite3_expanded_sql(m_stmt);
    const auto sqlStr = QString::fromUtf8(sql);
    sqlite3_free(sql);
    return sqlStr;
}

int SqliteStmt::bindParameterIndex(const QString &name) const
{
    return sqlite3_bind_parameter_index(m_stmt, name.toUtf8());
}

bool SqliteStmt::bindInt(int index, qint32 number)
{
    return checkBindResult(sqlite3_bind_int(m_stmt, index, number));
}

bool SqliteStmt::bindInt64(int index, qint64 number)
{
    return checkBindResult(sqlite3_bind_int64(m_stmt, index, number));
}

bool SqliteStmt::bindDouble(int index, double number)
{
    return checkBindResult(sqlite3_bind_double(m_stmt, index, number));
}

bool SqliteStmt::bindNull(int index)
{
    return checkBindResult(sqlite3_bind_null(m_stmt, index));
}

bool SqliteStmt::bindText(int index, const QString &text)
{
    const auto textUtf8 = text.toUtf8();
    const int bytesCount = textUtf8.size();

    m_bindObjects.insert(index, textUtf8);

    return checkBindResult(
            sqlite3_bind_text(m_stmt, index, textUtf8.data(), bytesCount, SQLITE_STATIC));
}

bool SqliteStmt::bindDateTime(int index, const QDateTime &dateTime)
{
    const auto msecs = dateTime.isNull() ? 0 : dateTime.toMSecsSinceEpoch();
    return bindInt64(index, msecs);
}

bool SqliteStmt::bindBlob(int index, const QByteArray &data)
{
    const int bytesCount = data.size();

    m_bindObjects.insert(index, data);

    return checkBindResult(
            sqlite3_bind_blob(m_stmt, index, data.constData(), bytesCount, SQLITE_STATIC));
}

bool SqliteStmt::bindDataStream(int index, const QVariant &v)
{
    const qint16 vType = v.userType();

    QByteArray data;

    // Write type
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << vType;

    // Write content
    switch (vType) {
    case QMetaType::QImage: {
        QByteArray bufData;

        QBuffer buf(&bufData);
        buf.open(QIODevice::WriteOnly);

        const QImage image = v.value<QImage>();
        image.save(&buf, "PNG");

        buf.close();
        stream << bufData;
    } break;
    default:
        Q_UNREACHABLE();
    }

    return bindBlob(index, data);
}

bool SqliteStmt::bindVar(int index, const QVariant &v)
{
    return stmtBindVar(*this, index, v);
}

bool SqliteStmt::bindVars(const QVariantList &vars, int index)
{
    for (const QVariant &v : vars) {
        if (!bindVar(index++, v))
            return false;
    }
    return true;
}

bool SqliteStmt::bindVarsMap(const QVariantHash &varsMap)
{
    if (varsMap.isEmpty())
        return true;

    auto it = varsMap.constBegin();
    for (; it != varsMap.constEnd(); ++it) {
        const QString k = it.key();
        const QVariant v = it.value();

        const int index = bindParameterIndex(k);
        if (index == 0)
            return false;

        if (!bindVar(index, v))
            return false;
    }
    return true;
}

bool SqliteStmt::clearBindings()
{
    m_bindObjects.clear();

    return sqlite3_clear_bindings(m_stmt) == SQLITE_OK;
}

bool SqliteStmt::reset()
{
    return sqlite3_reset(m_stmt) == SQLITE_OK;
}

bool SqliteStmt::isBusy() const
{
    return sqlite3_stmt_busy(m_stmt) != 0;
}

SqliteStmt::StepResult SqliteStmt::step()
{
    const int res = sqlite3_step(m_stmt);

    switch (res) {
    case SQLITE_ROW:
        return StepRow;
    case SQLITE_DONE:
        return StepDone;
    default:
        return StepError;
    }
}

int SqliteStmt::dataCount() const
{
    return sqlite3_data_count(m_stmt);
}

int SqliteStmt::columnCount() const
{
    return sqlite3_column_count(m_stmt);
}

QString SqliteStmt::columnName(int column) const
{
    const char *name = sqlite3_column_name(m_stmt, column);
    return QString::fromUtf8(name);
}

qint32 SqliteStmt::columnInt(int column) const
{
    return sqlite3_column_int(m_stmt, column);
}

quint32 SqliteStmt::columnUInt(int column) const
{
    return quint32(columnInt(column));
}

qint64 SqliteStmt::columnInt64(int column) const
{
    return sqlite3_column_int64(m_stmt, column);
}

quint64 SqliteStmt::columnUInt64(int column) const
{
    return quint64(columnInt64(column));
}

double SqliteStmt::columnDouble(int column) const
{
    return sqlite3_column_double(m_stmt, column);
}

bool SqliteStmt::columnBool(int column) const
{
    return columnInt(column) != 0;
}

QString SqliteStmt::columnText(int column) const
{
    const char *p = reinterpret_cast<const char *>(sqlite3_column_text(m_stmt, column));
    if (!p || *p == '\0')
        return QString();

    const int bytesCount = sqlite3_column_bytes(m_stmt, column);
    if (bytesCount == 0)
        return QString();

    return QString::fromUtf8(p, bytesCount);
}

QDateTime SqliteStmt::columnDateTime(int column) const
{
    const auto msecs = columnInt64(column);
    return (msecs == 0) ? QDateTime() : QDateTime::fromMSecsSinceEpoch(msecs);
}

QDateTime SqliteStmt::columnUnixTime(int column) const
{
    const auto secs = columnInt64(column);
    return (secs == 0) ? QDateTime() : QDateTime::fromSecsSinceEpoch(secs);
}

QByteArray SqliteStmt::columnBlob(int column, bool isRaw) const
{
    const char *p = static_cast<const char *>(sqlite3_column_blob(m_stmt, column));
    if (!p)
        return QByteArray();

    const int bytesCount = sqlite3_column_bytes(m_stmt, column);
    if (bytesCount == 0)
        return QByteArray();

    return isRaw ? QByteArray::fromRawData(p, bytesCount) : QByteArray(p, bytesCount);
}

QVariant SqliteStmt::columnDataStream(int column) const
{
    const QByteArray data = columnBlob(column);
    QDataStream stream(data);

    // Load type
    qint16 vType;
    stream >> vType;

    // Load content
    switch (vType) {
    case QMetaType::QImage: {
        QByteArray bufData;
        stream >> bufData;

        QImage image;
        image.loadFromData(bufData, "PNG");
        return image;
    }
    default:
        Q_UNREACHABLE();
        return {};
    }
}

QVariant SqliteStmt::columnVar(int column) const
{
    switch (sqlite3_column_type(m_stmt, column)) {
    case SQLITE_INTEGER:
        return columnInt64(column);
    case SQLITE_FLOAT:
        return columnDouble(column);
    case SQLITE_TEXT:
        return columnText(column);
    case SQLITE_BLOB:
        return columnDataStream(column);
    case SQLITE_NULL:
        break;
    default:
        Q_UNREACHABLE();
    }

    return QVariant();
}

bool SqliteStmt::columnIsNull(int column) const
{
    return sqlite3_column_type(m_stmt, column) == SQLITE_NULL;
}
