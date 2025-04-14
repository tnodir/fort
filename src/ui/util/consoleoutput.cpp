#include "consoleoutput.h"

#include <util/osutil.h>

ConsoleOutput::ConsoleOutput(const QString &filePath) : m_file(filePath)
{
    setupOutputType();

    openOutput();
}

ConsoleOutput::~ConsoleOutput()
{
    closeOutput();
}

void ConsoleOutput::setupOutputType()
{
    const QString fileName = m_file.fileName().toLower();

    if (fileName.isEmpty() || fileName == "con") {
        m_outputType = OUT_CONSOLE;
    } else if (fileName == "stdout") {
        m_outputType = OUT_STDOUT;
    } else if (fileName == "stderr") {
        m_outputType = OUT_STDERR;
    } else {
        m_outputType = OUT_FILE;
    }
}

void ConsoleOutput::openOutput()
{
    if (m_outputType != OUT_FILE) {
        OsUtil::attachConsole();
        return;
    }

    m_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
}

void ConsoleOutput::closeOutput()
{
    if (m_outputType != OUT_FILE) {
        OsUtil::freeConsole();
    }
}

void ConsoleOutput::write(const QString &line)
{
    if (m_outputType == OUT_FILE) {
        m_file.write(line.toUtf8());
    } else {
        OsUtil::writeToOutput(line, m_outputType);
    }
}

void ConsoleOutput::write(const QStringList &lines, char sep)
{
    if (lines.isEmpty())
        return;

    write(lines.join(sep) + '\n');
}
