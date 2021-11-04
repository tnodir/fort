#include <QCoreApplication>

#include "driverpayload.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DriverPayload payload;
    payload.processArguments(QCoreApplication::arguments());

    return 0;
}
