#include "sqlitestmt.h"

#include <QBuffer>
#include <QDataStream>
#include <QImage>

#include <sqlite3.h>

SqliteStmt::SqliteStmt() :
    m_stmt(nullptr)
{
}

SqliteStmt::~SqliteStmt()
{
    finalize();
}

bool SqliteStmt::prepare(sqlite3 *db, const char *sql,
                         SqliteStmt::PrepareFlags flags)
{
    return sqlite3_prepare_v3(db, sql, -1, flags,
                              &m_stmt, nullptr) == SQLITE_OK;
}

void SqliteStmt::finalize()
{
    if (m_stmt != nullptr) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

bool SqliteStmt::bindInt(int index, qint32 number)
{
    return sqlite3_bind_int(m_stmt, index, number) == SQLITE_OK;
}

bool SqliteStmt::bindInt64(int index, qint64 number)
{
    return sqlite3_bind_int64(m_stmt, index, number) == SQLITE_OK;
}

bool SqliteStmt::bindDouble(int index, double number)
{
    return sqlite3_bind_double(m_stmt, index, number) == SQLITE_OK;
}

bool SqliteStmt::bindNull(int index)
{
    return sqlite3_bind_null(m_stmt, index) == SQLITE_OK;
}

bool SqliteStmt::bindText(int index, const QString &text)
{
    const int bytesCount = text.size() * int(sizeof(wchar_t));

    m_bindObjects.insert(index, text);

    return sqlite3_bind_text16(m_stmt, index, text.utf16(),
                               bytesCount, SQLITE_STATIC) == SQLITE_OK;
}

bool SqliteStmt::bindDateTime(int index, const QDateTime &dateTime)
{
    const auto msecs = dateTime.isNull() ? 0
                                         : dateTime.toMSecsSinceEpoch();
    return bindInt64(index, msecs);
}

bool SqliteStmt::bindBlob(int index, const QByteArray &data)
{
    const int bytesCount = data.size();

    m_bindObjects.insert(index, data);

    return sqlite3_bind_blob(m_stmt, index, data.constData(),
                             bytesCount, SQLITE_STATIC) == SQLITE_OK;
}

bool SqliteStmt::bindVar(int index, const QVariant &v)
{
    const qint16 vType = v.type();

    switch (vType) {
    case QVariant::Invalid:
        return bindNull(index);
    case QVariant::Bool:
    case QVariant::Int:
    case QVariant::UInt:
        return bindInt(index, v.toInt());
    case QVariant::LongLong:
    case QVariant::ULongLong:
        return bindInt64(index, v.toLongLong());
    case QVariant::Double:
        return bindDouble(index, v.toDouble());
    case QVariant::String: {
        return bindText(index, v.toString());
    }
    case QVariant::DateTime: {
        return bindDateTime(index, v.toDateTime());
    }
    case QVariant::ByteArray: {
        return bindBlob(index, v.toByteArray());
    }
    default: {
        QByteArray data;

        // Write type
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << vType;

        // Write content
        {
            switch (vType) {
            case QVariant::Image: {
                QByteArray bufData;

                QBuffer buf(&bufData);
                buf.open(QIODevice::WriteOnly);

                const QImage image = v.value<QImage>();
                image.save(&buf, "PNG");

                buf.close();
                stream << bufData;
                break;
            }
            default:
                Q_UNREACHABLE();
            }
        }

        return bindBlob(index, data);
    }
    }
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

bool SqliteStmt::isBusy()
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

int SqliteStmt::dataCount()
{
    return sqlite3_data_count(m_stmt);
}

int SqliteStmt::columnCount()
{
    return sqlite3_column_count(m_stmt);
}

QString SqliteStmt::columnName(int column)
{
    const auto name = sqlite3_column_name16(m_stmt, column);
    return QString::fromWCharArray((const wchar_t *) name);
}

qint32 SqliteStmt::columnInt(int column)
{
    return sqlite3_column_int(m_stmt, column);
}

qint64 SqliteStmt::columnInt64(int column)
{
    return sqlite3_column_int64(m_stmt, column);
}

double SqliteStmt::columnDouble(int column)
{
    return sqlite3_column_double(m_stmt, column);
}

QString SqliteStmt::columnText(int column)
{
    const ushort *p = static_cast<const ushort *>(
                sqlite3_column_text16(m_stmt, column));
    if (p == nullptr || *p == '\0')
        return QString();

    const int bytesCount = sqlite3_column_bytes16(m_stmt, column);
    if (bytesCount == 0)
        return QString();

    return QString::fromUtf16(p, bytesCount / int(sizeof(wchar_t)));
}

QDateTime SqliteStmt::columnDateTime(int column)
{
    const auto msecs = columnInt64(column);
    return (msecs == 0) ? QDateTime()
                        : QDateTime::fromMSecsSinceEpoch(msecs);
}

QByteArray SqliteStmt::columnBlob(int column)
{
    const char *p = static_cast<const char *>(
                sqlite3_column_blob(m_stmt, column));
    if (p == nullptr)
        return QByteArray();

    const int bytesCount = sqlite3_column_bytes(m_stmt, column);
    if (bytesCount == 0)
        return QByteArray();

    return QByteArray(p, bytesCount);
}

QVariant SqliteStmt::columnVar(int column)
{
    switch (sqlite3_column_type(m_stmt, column)) {
    case SQLITE_INTEGER:
        return columnInt64(column);
    case SQLITE_FLOAT:
        return columnDouble(column);
    case SQLITE_TEXT:
        return columnText(column);
    case SQLITE_BLOB: {
        const QByteArray data = columnBlob(column);
        QDataStream stream(data);

        // Load type
        qint16 vType;
        stream >> vType;

        // Load content
        {
            switch (vType) {
            case QVariant::Image: {
                QByteArray bufData;
                stream >> bufData;

                QImage image;
                image.loadFromData(bufData, "PNG");
                return image;
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

    return QVariant();
}
