#pragma once

#include <QSignalSpy>

#include <googletest.h>

#include <util/ioccontainer.h>

namespace IocTest {

class A : public IocObject
{
public:
    void setUp() override
    {
        IocObject::setUp();
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

class B : public IocObject
{
public:
    void setUp() override
    {
        IocObject::setUp();
        IoC()->setUpDependency<A>();
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
    container.insert<IocTest::A>(a2);

    ASSERT_EQ(container.resolve<IocTest::A>(), a2);
    ASSERT_TRUE(container.pinToThread());
    ASSERT_EQ(IoC<IocTest::A>(), a2);
}

TEST_F(IocContainerTest, setUp)
{
    IocContainer container;

    auto a2 = new IocTest::A2();
    container.insert<IocTest::A>(a2);

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
    container.insert<IocTest::A>(&mockA);

    auto b = new IocTest::B();
    container.insert<IocTest::B>(b);

    EXPECT_CALL(mockA, setIsA(true)).Times(1);
    container.setUpAll();
    ASSERT_FALSE(mockA.isA());

    EXPECT_CALL(mockA, tearDown()).Times(1);
    container.tearDownAll();

    const int containerSize = container.objects().size();
    container.insert<IocTest::A>(nullptr);
    ASSERT_EQ(container.objects().size(), containerSize);
}
