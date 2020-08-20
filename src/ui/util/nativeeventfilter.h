#ifndef NATIVEEVENTFILTER_H
#define NATIVEEVENTFILTER_H

#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QObject>

#include "classhelpers.h"

class NativeEventFilter : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit NativeEventFilter(QObject *parent = nullptr);
    ~NativeEventFilter() override;
    CLASS_DELETE_COPY_MOVE(NativeEventFilter)

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
#endif

    bool registerHotKey(int hotKeyId, Qt::Key keyCode, Qt::KeyboardModifiers modifiers,
            bool autoRepeat = false);
    bool registerHotKey(int hotKeyId, int key, bool autoRepeat = false);

    void unregisterHotKey(int hotKeyId);
    void unregisterHotKeys();

signals:
    void hotKeyPressed(int hotKeyId);
    void environmentChanged();

public slots:

private:
    void setKeyId(int hotKeyId, quint32 nativeMod, quint32 nativeKey);
    void removeKeyId(int hotKeyId);
    int getKeyId(quint32 nativeMod, quint32 nativeKey) const;

    static quint32 nativeKeyCode(Qt::Key keyCode);
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers, bool autoRepeat = false);

private:
    QHash<quint32, int> m_keyIdMap;
};

#endif // NATIVEEVENTFILTER_H
