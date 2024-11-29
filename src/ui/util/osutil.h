#ifndef OSUTIL_H
#define OSUTIL_H

#include <QObject>

enum BeepType {
    BeepError = 0x10,
    BeepQuestion = 0x20,
    BeepWarning = 0x30,
    BeepInfo = 0x40,
    BeepSimple = -1
};

enum SoundType {
    SoundDefault = 0,
};

class OsUtil
{
public:
    static QString pidToPath(quint32 pid, bool isKernelPath = false);

    static bool openFolder(const QString &filePath);
    static bool openUrl(const QUrl &url);
    static bool openUrlOrFolder(const QString &path);

    static void *createMutex(const char *name, bool &isSingleInstance);
    static void closeMutex(void *mutexHandle);

    static quint32 lastErrorCode();
    static QString errorMessage(quint32 errorCode = lastErrorCode());

    static QString userName();
    static bool isUserAdmin();

    static bool beep(BeepType type = BeepSimple);
    static bool playSound(SoundType type = SoundDefault);

    static void showConsole(bool visible);
    static void writeToConsole(const QString &line);

    static bool setCurrentThreadName(const QString &name);
    static void setThreadIsBusy(bool on);

    static bool allowOtherForegroundWindows();
    static bool excludeWindowFromCapture(QWidget *window, bool on = true);

    static bool registerAppRestart();

    static void beginRestartClients();
    static void endRestartClients();
    static void restartClient();

    static void startService(const QString &serviceName);

    static void restart();
    static void quit(const QString &reason);

    static bool runCommand(const QString &command, const QString &workingDir = {});
};

#endif // OSUTIL_H
