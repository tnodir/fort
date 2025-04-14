#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QFile>
#include <QObject>

#include <util/util_types.h>

class ConsoleOutput
{
public:
    explicit ConsoleOutput(const QString &filePath = {});
    virtual ~ConsoleOutput();

    void write(const QString &line);
    void write(const QStringList &lines, char sep = ' ');

private:
    void setupOutputType();

    void openOutput();
    void closeOutput();

private:
    OutputType m_outputType = OUT_CONSOLE;

    QFile m_file;
};

#endif // CONSOLEOUTPUT_H
