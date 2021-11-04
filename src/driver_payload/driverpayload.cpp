#include "driverpayload.h"

#include <QCommandLineParser>

void DriverPayload::processArguments(const QStringList &args)
{
    QCommandLineParser parser;

    const QCommandLineOption inputOption(QStringList() << "i"
                                                       << "input",
            "Input file.", "input");
    parser.addOption(inputOption);

    const QCommandLineOption outputOption(QStringList() << "o"
                                                        << "output",
            "Output file.", "output");
    parser.addOption(outputOption);

    const QCommandLineOption payloadOption(QStringList() << "p"
                                                         << "payload",
            "Payload file.", "payload");
    parser.addOption(payloadOption);

    parser.addHelpOption();

    parser.process(args);

    if (!parser.isSet(inputOption) || !parser.isSet(outputOption) || !parser.isSet(payloadOption)) {
        parser.showHelp(1);
        return;
    }

    m_inputFilePath = parser.value(inputOption);
    m_outputFilePath = parser.value(outputOption);
    m_payloadFilePath = parser.value(payloadOption);
}
