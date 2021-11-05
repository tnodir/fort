#ifndef DRIVERPAYLOAD_H
#define DRIVERPAYLOAD_H

#include <QObject>

class DriverPayload
{
public:
    explicit DriverPayload() = default;

    void processArguments(const QStringList &args);

    bool createOutputFile();

private:
    QString m_inputFilePath;
    QString m_outputFilePath;
    QString m_payloadFilePath;
    QString m_secretFilePath;
};

#endif // DRIVERPAYLOAD_H
