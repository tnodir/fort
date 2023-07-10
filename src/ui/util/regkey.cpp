#include "regkey.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

RegKey::RegKey(RegHandle parentHandle, const QString &subKey, quint32 flags)
{
    LPCWSTR subKeyStr = (LPCWSTR) subKey.utf16();
    const REGSAM samDesired = ((flags & ReadOnly) != 0 ? KEY_READ : KEY_ALL_ACCESS)
            | ((flags & Native64Key) != 0 ? KEY_WOW64_64KEY : 0)
            | ((flags & Native32Key) != 0 ? KEY_WOW64_32KEY : 0);

    if ((flags & Create) != 0) {
        DWORD disposition;
        RegCreateKeyEx((HKEY) parentHandle, subKeyStr, 0, nullptr, 0, samDesired, nullptr,
                (PHKEY) &m_handle, &disposition);
    } else {
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

bool RegKey::setValue(const QString &name, const QVariant &value, bool expand)
{
    const unsigned char *data = nullptr;
    DWORD size;
    DWORD type;

    switch (value.userType()) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
    case QMetaType::Nullptr: {
        size = 0;
        type = REG_SZ;
    } break;
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt: {
        const qint32 i32 = value.toInt();
        data = (const unsigned char *) &i32;
        size = sizeof(qint32);
        type = REG_DWORD;
    } break;
    case QMetaType::LongLong:
    case QMetaType::ULongLong: {
        const qint64 i64 = value.toLongLong();
        data = (const unsigned char *) &i64;
        size = sizeof(qint64);
        type = REG_QWORD;
    } break;
    default: {
        const QString text = value.toString();
        data = (const unsigned char *) text.utf16();
        size = DWORD(sizeof(wchar_t) * (text.size() + 1)); /* + terminating null character */
        type = expand ? REG_EXPAND_SZ : REG_SZ;
    } break;
    }

    return !RegSetValueEx((HKEY) handle(), (LPCWSTR) name.utf16(), 0, type, data, size);
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
