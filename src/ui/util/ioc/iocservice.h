#ifndef IOCSERVICE_H
#define IOCSERVICE_H

class IocService
{
public:
    explicit IocService() = default;
    virtual ~IocService() = default;

    virtual void setUp() { }
    virtual void tearDown() { }
};

#endif // IOCSERVICE_H
