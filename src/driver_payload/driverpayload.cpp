#include "driverpayload.h"

#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QFile>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <bcrypt.h>

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

constexpr void writeUInt16(char *cp, int offset, quint16 v)
{
    *((quint16 *) (cp + offset)) = v;
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

bool getCoffHeaderOffset(const QByteArray &data, int &outCoffHeaderOffset)
{
    const char *cp = data.data();

    // Check the input DOS header: "MZ"
    if (cp[0] != 'M' || cp[1] != 'Z') {
        qCritical() << "DOS Header error: Invalid signature";
        return false;
    }

    // Check the input PE header offset
    const quint32 peOffset = readUInt32(cp, 0x3C);
    if (peOffset + 64 > data.size()) {
        qCritical() << "DOS Header error: Invalid PE Header Offset" << peOffset;
        return false;
    }

    // Check the input PE header: "PE\0\0"
    const char *pe = cp + peOffset;
    if (*pe++ != 'P' || *pe++ != 'E' || *pe++ != '\0' || *pe++ != '\0') {
        qCritical() << "PE Header error: Invalid signature at offset:" << peOffset;
        return false;
    }

    outCoffHeaderOffset = pe - cp;

    qDebug() << "PE Header offset:" << peOffset << "COFF Header offset" << outCoffHeaderOffset;

    return true;
}

bool getCertTableSize(const QByteArray &data, int &outCertEntrySizeOffset, int &outCertTableOffset,
        int &outCertTableSize)
{
    // Get a pointer to COFF Header
    int coffHeaderOffset;
    if (!getCoffHeaderOffset(data, coffHeaderOffset))
        return false;

    const char *coffHeader = data.data() + coffHeaderOffset;

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

    if (certTableSize == 0 || certTableOffset + certTableSize != data.size()) {
        qCritical().nospace() << "Certificate table error: Not at the end of input file (offset: "
                              << certTableOffset << " size:" << certTableSize
                              << "). Expected file size: " << (certTableOffset + certTableSize);
        return false;
    }

    // Check Certificate table's size from its table
    if (certTableSize != readUInt32(data.constData(), certTableOffset)) {
        qCritical() << "Certificate table error: Size mismatch";
        return false;
    }

    outCertEntrySizeOffset = coffHeaderOffset + CERTIFICATE_ENTRY_SIZE_OFFSET;
    outCertTableOffset = certTableOffset;
    outCertTableSize = certTableSize;

    return true;
}

bool signHash(BCRYPT_KEY_HANDLE keyHandle, const QByteArray &digest, QByteArray &outSignature)
{
    NTSTATUS status;

    BCRYPT_PKCS1_PADDING_INFO padInfo;
    padInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

    ULONG blobLen;
    status = BCryptSignHash(keyHandle, &padInfo, (PUCHAR) digest.data(), digest.size(), NULL, 0,
            &blobLen, BCRYPT_PAD_PKCS1);
    if (status) {
        qCritical() << "Sign Hash error: Size:" << status;
        return false;
    }

    QByteArray blob(blobLen, Qt::Uninitialized);

    status = BCryptSignHash(keyHandle, &padInfo, (PUCHAR) digest.data(), digest.size(),
            (PUCHAR) blob.data(), blobLen, &blobLen, BCRYPT_PAD_PKCS1);
    if (status) {
        qCritical() << "Sign Hash error:" << status;
        return false;
    }

    outSignature = blob;

    return true;
}

bool exportKey(BCRYPT_KEY_HANDLE keyHandle, LPCWSTR blobType, const QString &filePath)
{
    NTSTATUS status;

    ULONG blobLen;
    status = BCryptExportKey(keyHandle, NULL, blobType, NULL, 0, &blobLen, 0);
    if (status) {
        qCritical() << "Export Key error: Size:" << status << filePath;
        return false;
    }

    QByteArray blob(blobLen, Qt::Uninitialized);

    status = BCryptExportKey(keyHandle, NULL, blobType, (PUCHAR) blob.data(), blobLen, &blobLen, 0);
    if (status) {
        qCritical() << "Export Key error:" << status << filePath;
        return false;
    }

    return writeFile(filePath, { blob });
}

bool createKeyPair(
        BCRYPT_KEY_HANDLE &keyHandle, BCRYPT_ALG_HANDLE algHandle, const QString &secretFilePath)
{
    NTSTATUS status;

    status = BCryptGenerateKeyPair(algHandle, &keyHandle, 4096, 0);
    if (status) {
        qCritical() << "Create Key error: Generate:" << status;
        return false;
    }

    status = BCryptFinalizeKeyPair(keyHandle, 0);
    if (status) {
        qCritical() << "Create Key error: Finalize:" << status;
        return false;
    }

    return exportKey(keyHandle, BCRYPT_RSAPRIVATE_BLOB, secretFilePath)
            && exportKey(keyHandle, BCRYPT_RSAPUBLIC_BLOB, secretFilePath + ".pub");
}

bool importKeyPair(
        BCRYPT_KEY_HANDLE &keyHandle, BCRYPT_ALG_HANDLE algHandle, const QString &secretFilePath)
{
    NTSTATUS status;

    const QByteArray blob = readFile(secretFilePath, 128 * 1024);
    if (blob.isEmpty())
        return false;

    status = BCryptImportKeyPair(algHandle, NULL, BCRYPT_RSAPRIVATE_BLOB, &keyHandle,
            (PUCHAR) blob.data(), blob.size(), 0);
    if (status) {
        qCritical() << "Import Key error:" << status;
        return false;
    }

    return true;
}

bool openSecretFile(
        BCRYPT_KEY_HANDLE &keyHandle, BCRYPT_ALG_HANDLE algHandle, const QString &secretFilePath)
{
    if (!FileUtil::fileExists(secretFilePath)) {
        return createKeyPair(keyHandle, algHandle, secretFilePath);
    } else {
        return importKeyPair(keyHandle, algHandle, secretFilePath);
    }
}

bool createPayloadSignature(
        const QByteArray &data, const QString &secretFilePath, QByteArray &outSignature)
{
    NTSTATUS status;

    BCRYPT_ALG_HANDLE algHandle;
    status = BCryptOpenAlgorithmProvider(&algHandle, BCRYPT_RSA_ALGORITHM, NULL, 0);
    if (status) {
        qCritical() << "Payload Sign error: Open Algorithm:" << status;
        return false;
    }

    BCRYPT_KEY_HANDLE keyHandle;
    bool ok = openSecretFile(keyHandle, algHandle, secretFilePath);
    if (ok) {
        const QByteArray digest = QCryptographicHash::hash(data, QCryptographicHash::Sha256);

        ok = signHash(keyHandle, digest, outSignature);

        BCryptDestroyKey(keyHandle);
    }

    BCryptCloseAlgorithmProvider(algHandle, 0);

    return ok;
}

}

void DriverPayload::processArguments(const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Append payload to the signed executable file."
                                     "The result is stored in the output file.");

    const QCommandLineOption inputOption(QStringList() << "i"
                                                       << "input",
            "Input file path.", "input");
    parser.addOption(inputOption);

    const QCommandLineOption outputOption(QStringList() << "o"
                                                        << "output",
            "Output file path.", "output");
    parser.addOption(outputOption);

    const QCommandLineOption payloadOption(QStringList() << "p"
                                                         << "payload",
            "Payload file path.", "payload");
    parser.addOption(payloadOption);

    const QCommandLineOption secretOption(QStringList() << "s"
                                                        << "secret",
            "Secret file path.", "secret");
    parser.addOption(secretOption);

    parser.addHelpOption();

    parser.process(args);

    if (!parser.isSet(inputOption) || !parser.isSet(outputOption) || !parser.isSet(payloadOption)
            || !parser.isSet(secretOption)) {
        parser.showHelp(1);
        return;
    }

    m_inputFilePath = parser.value(inputOption);
    m_outputFilePath = parser.value(outputOption);
    m_payloadFilePath = parser.value(payloadOption);
    m_secretFilePath = parser.value(secretOption);
}

bool DriverPayload::createOutputFile()
{
    // Read input & payload files
    QByteArray inData = readFile(m_inputFilePath, 1 * 1024 * 1024);
    QByteArray payloadData = readFile(m_payloadFilePath, 3 * 1024 * 1024);
    if (inData.isEmpty() || payloadData.isEmpty())
        return false;

    // Get the Certificate entry section's info
    int certEntrySizeOffset;
    int certTableOffset;
    int certTableSize;
    if (!getCertTableSize(inData, certEntrySizeOffset, certTableOffset, certTableSize))
        return false;

    // Payload's empty certificate header
    const QByteArray payloadHeader(8, '\0');

    // Adjust padding of payload by required alignment
    adjustPayloadPadding(payloadData);

    // Payload Signature
    QByteArray payloadSignature;
    if (!createPayloadSignature(payloadData, m_secretFilePath, payloadSignature))
        return false;

    const int signatureSize = payloadSignature.size();
    adjustPayloadPadding(payloadSignature);

    // Payload Info
    QByteArray payloadInfo(8, '\0');
    {
        char *cp = payloadInfo.data();
        writeUInt16(cp, 0, signatureSize);
        writeUInt16(cp, 2, payloadSignature.size());
        writeUInt32(cp, 4, payloadData.size());
    }

    // Update the Certificate entry
    {
        const int newCertTableSize = certTableSize + payloadHeader.size() + payloadData.size()
                + payloadSignature.size() + payloadInfo.size();
        char *cp = inData.data();
        writeUInt32(cp, certEntrySizeOffset, newCertTableSize);
        writeUInt32(cp, certTableOffset, newCertTableSize);
    }

    // Write the input & payload data into output file
    if (!writeFile(m_outputFilePath,
                { inData, payloadHeader, payloadSignature, payloadData, payloadInfo }))
        return false;

    qDebug() << "Success:" << m_outputFilePath;

    return true;
}
