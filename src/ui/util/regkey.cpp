#include "regkey.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "stringutil.h"

RegKey::RegKey(RegHandle parentHandle, const QString &subKey, quint32 flags)
{
    LPCWSTR subKeyStr = (LPCWSTR) subKey.utf16();
    const REGSAM samDesired = ((flags & ReadOnly) != 0 ? KEY_READ : KEY_ALL_ACCESS)
            | ((flags & Notify) != 0 ? KEY_NOTIFY : 0)
            | ((flags & Native64Key) != 0 ? KEY_WOW64_64KEY : 0)
            | ((flags & Native32Key) != 0 ? KEY_WOW64_32KEY : 0);

    if ((flags & Create) != 0) {
        DWORD disposition;
        m_openError = RegCreateKeyEx((HKEY) parentHandle, subKeyStr, 0, nullptr, 0, samDesired,
                nullptr, (PHKEY) &m_handle, &disposition);
    } else {
        m_openError =
                RegOpenKeyEx((HKEY) parentHandle, subKeyStr, 0, samDesired, (PHKEY) &m_handle);
    }
}

RegKey::RegKey(Root root, const QString &subKey, quint32 flags) :
    RegKey(predefinedRootHandle(root), subKey, flags)
{
}

RegKey::RegKey(const RegKey &parent, const QString &subKey, quint32 flags) :
    RegKey(parent.handle(), subKey, flags)
{
}

RegKey::~RegKey()
{
    if (!isNull()) {
        RegCloseKey((HKEY) handle());
    }
}

bool RegKey::isAccessDenied() const
{
    return m_openError == ERROR_ACCESS_DENIED;
}

bool RegKey::removeKey(const QString &subKey)
{
    return !RegDeleteKey((HKEY) handle(), (LPCWSTR) subKey.utf16());
}

bool RegKey::clearTree(const QString &subKey)
{
    return !RegDeleteTree((HKEY) handle(), subKey.isEmpty() ? nullptr : (LPCWSTR) subKey.utf16());
}

bool RegKey::removeRecursively(const QString &subKey)
{
    return clearTree(subKey) && removeKey(subKey);
}

bool RegKey::removeValue(const QString &name)
{
    return !RegDeleteValue((HKEY) handle(), (LPCWSTR) name.utf16());
}

namespace {

struct RegData
{
    DWORD type = REG_SZ;
    QByteArray buffer;
};

template<typename T>
void setNumberData(RegData &data, T number, DWORD type)
{
    data.type = type;
    data.buffer = QByteArray((const char *) &number, sizeof(T));
}

bool fillNumberData(RegData &data, const QVariant &value)
{
    switch (value.userType()) {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
        setNumberData<qint32>(data, value.toInt(), REG_DWORD);
        return true;
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        setNumberData<qint64>(data, value.toLongLong(), REG_QWORD);
        return true;
    default:
        return false;
    }
}

void fillTextData(RegData &data, const QVariant &value, bool expand)
{
    data.type = expand ? REG_EXPAND_SZ : REG_SZ;
    StringUtil::addStringToBuffer(data.buffer, value.toString());
}

bool fillRegData(RegData &data, const QVariant &value, bool expand)
{
    switch (value.userType()) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
    case QMetaType::Nullptr:
        return true;
    case QMetaType::QStringList:
        data.type = REG_MULTI_SZ;
        return StringUtil::buildMultiString(data.buffer, value.toStringList());
    default:
        if (!fillNumberData(data, value)) {
            fillTextData(data, value, expand);
        }
        return true;
    }
}

}

bool RegKey::setValue(const QString &name, const QVariant &value, bool expand)
{
    RegData data;
    if (!fillRegData(data, value, expand))
        return false;

    const BYTE *dataPtr = data.buffer.isEmpty() ? nullptr : (const BYTE *) data.buffer.constData();

    return !RegSetValueEx((HKEY) handle(), (LPCWSTR) name.utf16(), 0, data.type, dataPtr,
            DWORD(data.buffer.size()));
}

bool RegKey::setOrRemoveValue(const QString &name, const QVariant &value, bool expand)
{
    return !value.isNull() ? setValue(name, value, expand) : removeValue(name);
}

QVariant RegKey::value(const QString &name, bool *expand) const
{
    char data[16 * 1024];
    DWORD size = sizeof(data);
    DWORD type;

    if (!RegQueryValueEx((HKEY) handle(), (LPCWSTR) name.utf16(), 0, &type, (LPBYTE) data, &size)) {
        switch (type) {
        case REG_EXPAND_SZ:
            if (expand) {
                *expand = true;
            }
            Q_FALLTHROUGH();
        case REG_SZ:
            return QString::fromWCharArray((LPCWSTR) data);
        case REG_MULTI_SZ:
            return StringUtil::parseMultiString(data);
        case REG_DWORD:
            return *((qint32 *) data);
        case REG_QWORD:
            return *((qint64 *) data);
        }
    }

    return QVariant();
}

bool RegKey::contains(const QString &name) const
{
    return !RegQueryValueEx((HKEY) handle(), (LPCWSTR) name.utf16(), 0, nullptr, nullptr, nullptr);
}

bool RegKey::notify(
        bool watchSubtree, quint32 filterFlags, qintptr eventHandle, bool isAsynchronous)
{
    return !RegNotifyChangeKeyValue(
            (HKEY) handle(), watchSubtree, filterFlags, (HANDLE) eventHandle, isAsynchronous);
}

RegKey::RegHandle RegKey::predefinedRootHandle(Root root)
{
    switch (root) {
    case HKCR:
        return HKEY_CLASSES_ROOT;
    case HKCU:
        return HKEY_CURRENT_USER;
    case HKLM:
        return HKEY_LOCAL_MACHINE;
    case HKU:
        return HKEY_USERS;
    }

    Q_UNREACHABLE();
    return nullptr;
}
