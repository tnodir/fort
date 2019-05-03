#ifndef CLASSHELPERS_H
#define CLASSHELPERS_H

// Define a default copy/move constructors and assignment operators
#define CLASS_DEFAULT_COPY_MOVE(Class) \
    Class(const Class &) = default;\
    Class &operator=(const Class &) = default;\
    Class(Class &&) = default;\
    Class &operator=(Class &&) = default;

// Define a delete copy/move constructors and assignment operators
#define CLASS_DELETE_COPY_MOVE(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;\
    Class(Class &&) = delete;\
    Class &operator=(Class &&) = delete;

#endif // CLASSHELPERS_H
