#ifndef THREADSTORAGE_H
#define THREADSTORAGE_H

class ThreadStorage final
{
public:
    explicit ThreadStorage();
    ~ThreadStorage();

    void *value() const;
    bool setValue(void *v);

private:
    void createTlsIndex();
    void deleteTlsIndex();

private:
    constexpr static int BadTlsIndex = -1;
    int m_tlsIndex = BadTlsIndex;
};

#endif // THREADSTORAGE_H
