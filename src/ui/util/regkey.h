#ifndef REGKEY_H
#define REGKEY_H

#include <QObject>
#include <QVariant>

#include <util/classhelpers.h>

class RegKey
{
public:
    enum Root : qint8 { HKCR = 0, HKCU, HKLM, HKU };

    enum OpenFlag : qint8 {
        ReadOnly = 0x01,
        ReadWrite = 0x02,
        Create = 0x04,
        Notify = 0x08,
        Native64Key = 0x10,
        Native32Key = 0x20,
        DefaultReadOnly = (ReadOnly | Native64Key),
        DefaultReadWrite = (ReadWrite | Native64Key),
        DefaultCreate = (ReadWrite | Create | Native64Key)
    };

    // Sync with REG_NOTIFY_CHANGE_*
    enum NotifyFlag : qint8 {
        NotifyChangeName = 0x01,
        NotifyChangeAttributes = 0x02,
        NotifyChangeLastSet = 0x04,
        NotifyChangeSecurity = 0x08,
    };

protected:
    using RegHandle = void *;
    explicit RegKey(RegHandle parentHandle, const QString &subKey, quint32 flags);

public:
    explicit RegKey(Root root, const QString &subKey = QString(), quint32 flags = DefaultReadOnly);
    explicit RegKey(const RegKey &parent, const QString &subKey = QString(),
            quint32 flags = DefaultReadOnly);
    ~RegKey();
    CLASS_DELETE_COPY_MOVE(RegKey)

    bool isNull() const { return m_handle == nullptr; }

    bool removeKey(const QString &subKey);
    bool clearTree(const QString &subKey = QString());
    bool removeRecursively(const QString &subKey);
    bool removeValue(const QString &name);
    bool setValue(const QString &name, const QVariant &value, bool expand = false);
    bool setDefaultValue(const QVariant &value) { return setValue(QString(), value); }
    QVariant value(const QString &name, bool *expand = nullptr) const;
    bool contains(const QString &name) const;

    bool notify(bool watchSubtree, quint32 filterFlags, qintptr eventHandle = 0,
            bool isAsynchronous = true);

private:
    RegHandle handle() const { return m_handle; }

    static RegKey::RegHandle predefinedRootHandle(Root root);

private:
    RegHandle m_handle = nullptr;
};

#endif // REGKEY_H
