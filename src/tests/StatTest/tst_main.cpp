#include "tst_stat.h"

#include <QCoreApplication>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fortmanager.h>

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    QCoreApplication app(argc, argv);

    FortManager::setupResources();

    return RUN_ALL_TESTS();
}
