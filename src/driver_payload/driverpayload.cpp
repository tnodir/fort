#include "driverpayload.h"

#include <QCommandLineParser>
#include <QFile>

#include <util/fileutil.h>

namespace {

constexpr quint16 readUInt16(const char *cp, int offset)
{
    return *((quint16 *) (cp + offset));
}

constexpr quint32 readUInt32(const char *cp, int offset)
{
    return *((quint32 *) (cp + offset));
}

constexpr void writeUInt32(char *cp, int offset, quint32 v)
{
    *((quint32 *) (cp + offset)) = v;
}

QByteArray readFile(const QString &filePath, int maxSize, int minSize = 1024)
{
    const QByteArray data = FileUtil::readFileData(filePath, maxSize + 4);

    if (data.size() < minSize || data.size() > maxSize) {
        qCritical() << "File read error:" << filePath << "Invalid size:" << data.size()
                    << "Expected min size:" << minSize << "max size:" << maxSize;
        return {};
    }

    return data;
}

bool writeFile(const QString &filePath, const QByteArrayList &dataList)
{
    const QByteArray data = dataList.join();

    if (!FileUtil::writeFileData(filePath, data)) {
        qCritical() << "File write error:" << filePath;
        return false;
    }

    return true;
}

void adjustPayloadPadding(QByteArray &data)
{
    constexpr int PAYLOAD_ALIGNMENT = 8;
    const int paddingSize = PAYLOAD_ALIGNMENT - (data.size() % PAYLOAD_ALIGNMENT);
    for (int i = 0; i < paddingSize; ++i) {
        data.append('\0');
    }
}

const char *getCoffHeader(const QByteArray &data)
{
    const char *cp = data.data();

    // Check the input DOS header: "MZ"
    if (cp[0] != 'M' || cp[1] != 'Z') {
        qCritical() << "DOS Header error: Invalid signature";
        return nullptr;
    }

    // Check the input PE header offset
    const quint32 peOffset = readUInt32(cp, 0x3C);
    if (peOffset + 64 > data.size()) {
        qCritical() << "DOS Header error: Invalid PE Header Offset" << peOffset;
        return nullptr;
    }

    // Check the input PE header: "PE\0\0"
    const char *pe = cp + peOffset;
    if (*pe++ != 'P' || *pe++ != 'E' || *pe++ != '\0' || *pe++ != '\0') {
        qCritical() << "PE Header error: Invalid signature at offset:" << peOffset;
        return nullptr;
    }

    qDebug() << "PE Header offset:" << peOffset << "COFF Header offset" << (pe - cp);

    return pe;
}

}

void DriverPayload::processArguments(const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Append payload to the signed executable file."
                                     "The result is stored in the output file.");

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

bool DriverPayload::createOutputFile()
{
    // Read input & payload files
    QByteArray inData = readFile(m_inputFilePath, 1 * 1024 * 1024);
    QByteArray payloadData = readFile(m_payloadFilePath, 3 * 1024 * 1024);
    if (inData.isEmpty() || payloadData.isEmpty())
        return false;

    // Get a pointer to COFF Header
    const char *coffHeader = getCoffHeader(inData);
    if (!coffHeader)
        return false;

    // Get the COFF magic number
    constexpr int COFF_MAGIC_OFFSET = 20;

    const quint16 magicNo = readUInt16(coffHeader, COFF_MAGIC_OFFSET);

    // Check the COFF magic number
    constexpr int COFF_MAGIC_PE32 = 0x10b;
    constexpr int COFF_MAGIC_PE32_PLUS = 0x20b;

    if (magicNo != COFF_MAGIC_PE32 && magicNo != COFF_MAGIC_PE32_PLUS) {
        qCritical() << "COFF magic number error:" << Qt::hex << magicNo;
        return false;
    }

    const bool isPE32 = (magicNo == COFF_MAGIC_PE32);

    // Get the Certificate entry section's offset & size
    const int CERTIFICATE_ENTRY_OFFSET = COFF_MAGIC_OFFSET + 128 + (isPE32 ? 0 : 16);
    const int CERTIFICATE_ENTRY_SIZE_OFFSET = CERTIFICATE_ENTRY_OFFSET + 4;

    const quint32 certTableOffset = readUInt32(coffHeader, CERTIFICATE_ENTRY_OFFSET);
    const quint32 certTableSize = readUInt32(coffHeader, CERTIFICATE_ENTRY_SIZE_OFFSET);

    if (certTableSize == 0 || certTableOffset + certTableSize != inData.size()) {
        qCritical().nospace() << "Certificate table error: Not at the end of input file (offset: "
                              << certTableOffset << " size:" << certTableSize
                              << "). Expected file size: " << (certTableOffset + certTableSize);
        return false;
    }

    // Check Certificate table's size from its table
    if (certTableSize != readUInt32(inData.constData(), certTableOffset)) {
        qCritical() << "Certificate table error: Size mismatch";
        return false;
    }

    // Payload's empty certificate header
    const QByteArray payloadHeader(8, '\0');

    // Adjust padding of payload by required alignment
    adjustPayloadPadding(payloadData);

    // Update the Certificate entry
    {
        char *cp = const_cast<char *>(coffHeader);
        const int newCertTableSize = certTableSize + payloadHeader.size() + payloadData.size();
        writeUInt32(cp, CERTIFICATE_ENTRY_SIZE_OFFSET, newCertTableSize);
        writeUInt32(cp, certTableOffset, newCertTableSize);
    }

    // Write the input & payload data into output file
    return writeFile(m_outputFilePath, { inData, payloadHeader, payloadData });
}
