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

bool RegKey::setValue(const QString &name, const QVariant &value)
{
    QString text;
    union {
        qint64 i64;
        qint32 i32;
    } num;
    const unsigned char *data = nullptr;
    DWORD size = 0, type = REG_SZ;

    switch (value.userType()) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
    case QMetaType::Nullptr:
        break;
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
        num.i32 = value.toInt();
        data = (const unsigned char *) &num.i32;
        size = sizeof(qint32);
        type = REG_DWORD;
        break;
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        num.i64 = value.toLongLong();
        data = (const unsigned char *) &num.i64;
        size = sizeof(qint64);
        type = REG_QWORD;
        break;
    default:
        text = value.toString();
        data = (const unsigned char *) text.utf16();
        size = sizeof(wchar_t) * (text.size() + 1); /* + terminating null character */
    }

    return !RegSetValueEx((HKEY) handle(), (LPCWSTR) name.utf16(), 0, type, data, size);
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
