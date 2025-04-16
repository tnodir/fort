#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QFile>
#include <QObject>

#include <util/util_types.h>

class ConsoleOutput
{
public:
    enum OutputType : qint8 {
        OUT_CONSOLE = 0,
        OUT_STDOUT,
        OUT_FILE,
    };

    explicit ConsoleOutput(const QString &filePath = {});

    void write(const QString &line);
    void write(const QStringList &lines, char sep = ' ');

    static bool attachConsole(quint32 processId = -1);
    static bool freeConsole();

    static bool showConsole(bool visible);
    static void writeToConsole(const QString &line);

private:
    void setupStdHandles();
    void setupOutputType();

    bool openOutput();

    void writeToStdOut(const QString &line);

private:
    OutputType m_outputType = OUT_CONSOLE;

    quintptr m_hStdOut = 0;

    QFile m_file;
};

#endif // CONSOLEOUTPUT_H
