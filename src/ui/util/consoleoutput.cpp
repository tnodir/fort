#include "consoleoutput.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <util/osutil.h>

namespace {

BOOL WINAPI consoleCtrlHandler(DWORD /*ctrlType*/)
{
    OsUtil::quit("console control");

    Sleep(100); // Let the process exit gracefully
    return TRUE;
}

}

ConsoleOutput::ConsoleOutput(const QString &filePath) : m_file(filePath)
{
    setupStdHandles();
    setupOutputType();

    openOutput();
}

void ConsoleOutput::setupStdHandles()
{
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (GetFileType(handle) == FILE_TYPE_DISK) {
        m_hStdOut = quintptr(handle);
    }
}

void ConsoleOutput::setupOutputType()
{
    const QString fileName = m_file.fileName().toLower();

    if (fileName.isEmpty()) {
        m_outputType = (m_hStdOut != 0) ? OUT_STDOUT : OUT_CONSOLE;
    } else {
        m_outputType = OUT_FILE;
    }
}

bool ConsoleOutput::openOutput()
{
    if (m_outputType == OUT_FILE) {
        return m_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    }

    if (m_outputType == OUT_CONSOLE) {
        return attachConsole();
    }

    return true;
}

void ConsoleOutput::write(const QString &line)
{
    switch (m_outputType) {
    case OUT_CONSOLE: {
        writeToConsole(line);
    } break;
    case OUT_STDOUT: {
        writeToStdOut(line);
    } break;
    case OUT_FILE: {
        m_file.write(line.toUtf8());
    } break;
    }
}

void ConsoleOutput::write(const QStringList &lines, char sep)
{
    if (lines.isEmpty())
        return;

    write(lines.join(sep) + '\n');
}

bool ConsoleOutput::attachConsole(quint32 processId)
{
    return AttachConsole(processId);
}

bool ConsoleOutput::freeConsole()
{
    return FreeConsole();
}

bool ConsoleOutput::showConsole(bool visible)
{
    // Close the console window
    if (!visible) {
        FreeConsole();
        return true;
    }

    // Show the console window
    if (!AllocConsole())
        return false;

    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);

    // Disable close button of console window
    {
        const HMENU hMenu = GetSystemMenu(GetConsoleWindow(), false);
        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    }

    return true;
}

void ConsoleOutput::writeToConsole(const QString &line)
{
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD nw;
    WriteConsoleW(handle, line.utf16(), line.size(), &nw, nullptr);
}

void ConsoleOutput::writeToStdOut(const QString &line)
{
    const auto buf = line.toUtf8();

    DWORD nw;
    WriteFile(HANDLE(m_hStdOut), buf.data(), buf.size(), &nw, nullptr);
}
