#include "tst_logbuffer.h"

#include <QCoreApplication>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    QCoreApplication app(argc, argv);

    return RUN_ALL_TESTS();
}
