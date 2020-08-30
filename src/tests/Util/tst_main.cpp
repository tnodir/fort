#include "tst_confutil.h"
#include "tst_fileutil.h"
#include "tst_netutil.h"

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
