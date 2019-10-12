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

    bool nativeEventFilter(const QByteArray &eventType,
                           void *message, qintptr *result) override;

    bool registerHotKey(int hotKeyId,
                        Qt::Key keyCode,
                        Qt::KeyboardModifiers modifiers,
                        bool autoRepeat = false);
    bool registerHotKey(int hotKeyId,
                        const QKeySequence &shortcut,
                        bool autoRepeat = false);

    void unregisterHotKey(int hotKeyId);
    void unregisterHotKeys();

signals:
    void hotKeyPressed(int hotKeyId);

public slots:

private:
    void setKeyId(int hotKeyId, quint32 nativeMod, quint32 nativeKey);
    void removeKeyId(int hotKeyId);
    int getKeyId(quint32 nativeMod, quint32 nativeKey) const;

    static quint32 nativeKeyCode(Qt::Key keyCode);
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers,
                                   bool autoRepeat = false);

private:
    QHash <quint32, int> m_keyIdMap;
};

#endif // NATIVEEVENTFILTER_H
