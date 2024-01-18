#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <util/ioc/ioccontainer.h>
#include <util/ioc/iocservice.h>

namespace IocTest {

class A : public IocService
{
public:
    void setUp() override
    {
        IocService::setUp();
        setIsA(true);
    }

    virtual bool isA() const { return m_isA; }
    virtual void setIsA(bool v) { m_isA = v; }

private:
    bool m_isA = false;
};

class A2 : public A
{
public:
};

class MockA : public A
{
public:
    MOCK_METHOD0(tearDown, void());
    MOCK_METHOD1(setIsA, void(bool v));
};

class B : public IocService
{
public:
    void setUp() override
    {
        IocService::setUp();
        IoCPinned()->setUpDependency<A>();
    }
};

}

class IocContainerTest : public Test
{
    // Test interface
protected:
    void SetUp();
    void TearDown();
};

inline void IocContainerTest::SetUp() { }

inline void IocContainerTest::TearDown() { }

TEST_F(IocContainerTest, getTypeId)
{
    const int aTypeId = IocContainer::getTypeId<IocTest::A>();
    ASSERT_NE(IocContainer::getTypeId<IocTest::A2>(), 0);
    ASSERT_EQ(IocContainer::getTypeId<IocTest::A>(), aTypeId);
}

TEST_F(IocContainerTest, getPinned)
{
    IocContainer container;

    ASSERT_EQ(IocContainer::getPinned(), nullptr);
    ASSERT_TRUE(container.pinToThread());
    ASSERT_EQ(IocContainer::getPinned(), &container);
}

TEST_F(IocContainerTest, insert)
{
    IocContainer container;

    auto a2 = new IocTest::A2();
    container.setService<IocTest::A>(a2);

    IocTest::B b;
    container.setService<IocTest::B>(b);

    ASSERT_EQ(container.resolve<IocTest::A>(), a2);
    ASSERT_EQ(container.resolve<IocTest::B>(), &b);
    ASSERT_TRUE(container.pinToThread());
    ASSERT_EQ(IoC<IocTest::A>(), a2);
    ASSERT_EQ(IoC<IocTest::B>(), &b);
}

TEST_F(IocContainerTest, setUp)
{
    IocContainer container;

    auto a2 = new IocTest::A2();
    container.setService<IocTest::A>(a2);

    ASSERT_FALSE(a2->isA());
    container.setUpAll();
    ASSERT_TRUE(a2->isA());

    a2->setIsA(false);
    ASSERT_FALSE(a2->isA());
    container.setUpAll();
    ASSERT_FALSE(a2->isA());
}

TEST_F(IocContainerTest, mockSetUp)
{
    IocContainer container;
    ASSERT_TRUE(container.pinToThread());

    NiceMock<IocTest::MockA> mockA;
    container.setService<IocTest::A>(&mockA);

    auto b = new IocTest::B();
    container.setService<IocTest::B>(b);

    EXPECT_CALL(mockA, setIsA(true)).Times(1);
    container.setUpAll();
    ASSERT_FALSE(mockA.isA());

    EXPECT_CALL(mockA, tearDown()).Times(1);
    container.tearDownAll();

    const int containerSize = container.size();
    container.remove<IocTest::A>();
    ASSERT_EQ(container.size(), containerSize);
}
