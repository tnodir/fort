#include "tst_bitutil.h"
#include "tst_confutil.h"
#include "tst_dateutil.h"
#include "tst_fileutil.h"
#include "tst_ioccontainer.h"
#include "tst_netutil.h"
#include "tst_ruletextparser.h"
#include "tst_stringutil.h"
#include "tst_wildmatch.h"

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
