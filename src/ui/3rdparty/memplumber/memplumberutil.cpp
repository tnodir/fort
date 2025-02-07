#include "memplumberutil.h"

#include <QLoggingCategory>
#include <QTimer>

#include <memplumber.h>

namespace {

const QLoggingCategory LC("memPlumber");

const char *const LOG_FILE_PATH = "memplumber.txt";

}

void MemPlumberUtil::start()
{
    MemPlumber::start();
}

void MemPlumberUtil::stop()
{
    MemPlumber::stop();
}

void MemPlumberUtil::checkAndStop()
{
    QTimer::singleShot(10, [&] {
        size_t memLeakCount;
        uint64_t memLeakSize;
        MemPlumber::memLeakCheck(memLeakCount, memLeakSize, /*verbose=*/true, LOG_FILE_PATH);

        stop();

        qCInfo(LC) << "Number of leaked objects:" << memLeakCount
                   << "Total amount of memory leaked:" << memLeakSize;
    });
}
