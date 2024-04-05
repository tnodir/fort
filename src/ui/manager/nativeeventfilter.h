#ifndef NATIVEEVENTFILTER_H
#define NATIVEEVENTFILTER_H

#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QObject>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

class NativeEventFilter : public QObject, public QAbstractNativeEventFilter, public IocService
{
    Q_OBJECT

public:
    explicit NativeEventFilter(QObject *parent = nullptr);
    ~NativeEventFilter() override;
    CLASS_DELETE_COPY_MOVE(NativeEventFilter)

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

    bool registerHotKey(int hotKeyId, Qt::Key keyCode, Qt::KeyboardModifiers modifiers);
    bool registerHotKey(int hotKeyId, QKeyCombination key);

    void unregisterHotKeys();

    bool registerSessionNotification(quintptr winId);
    void unregisterSessionNotification(quintptr winId);

signals:
    void hotKeyPressed(int hotKeyId);
    void environmentChanged();
    void sessionLocked();
    void driveListChanged();

private:
    void setKeyId(int hotKeyId, quint32 nativeMod, quint32 nativeKey);
    bool removeKeyId(int hotKeyId);
    int getKeyId(quint32 nativeMod, quint32 nativeKey) const;

    inline void processWmHotKey(void *message);
    inline void processWmSettingChange(void *message);
    inline void processWmWtsSessionChange(void *message);
    inline void processWmDeviceChange(void *message);

    static quint32 nativeKeyCode(Qt::Key keyCode);
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);
    static quint32 autoRepeatModifier(bool autoRepeat = false);

private:
    QHash<quint32, int> m_keyIdMap;
};

#endif // NATIVEEVENTFILTER_H
