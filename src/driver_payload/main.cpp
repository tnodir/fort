#include <QCoreApplication>

#include "driverpayload.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    DriverPayload payload;
    payload.processArguments(QCoreApplication::arguments());

    if (!payload.createOutputFile())
        return 2;

    return 0;
}
