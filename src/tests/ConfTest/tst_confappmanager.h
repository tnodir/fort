#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <sqlite/dbquery.h>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <util/ioc/ioccontainer.h>

class ConfAppManagerTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

void ConfAppManagerTest::SetUp() { }

void ConfAppManagerTest::TearDown() { }

TEST_F(ConfAppManagerTest, addAlertedApp)
{
    IocContainer ioc;
    ioc.pinToThread();

    ConfManager confManager(":memory:");
    ConfAppManager confAppManager;

    ioc.set<ConfManager>(confManager);
    ioc.set<ConfAppManager>(confAppManager);

    confManager.setUp();

    const auto alertsCount = [&] {
        return DbQuery(confManager.sqliteDb()).sql("SELECT COUNT(*) FROM app_alert;").execute();
    };

    QSignalSpy spy(&confAppManager, &ConfAppManager::appAlerted);

    ASSERT_TRUE(confAppManager.addAlertedApp(1));
    ASSERT_EQ(alertsCount(), 1);

    ASSERT_TRUE(spy.wait());
    ASSERT_EQ(spy.count(), 1);

    // The alerted app's next connection. The signal is delayed by a timer, so its absence has no
    // event to be checked after.
    ASSERT_TRUE(confAppManager.addAlertedApp(1));
    ASSERT_EQ(alertsCount(), 1);

    ASSERT_TRUE(confAppManager.addAlertedApp(2));
    ASSERT_EQ(alertsCount(), 2);
}
