#ifndef SERVICE_TYPES_H
#define SERVICE_TYPES_H

enum ServiceControlCode {
    ServiceControlStop = 0x0100, // SERVICE_USER_DEFINED_CONTROL
    ServiceControlStopRestarting,
};

#endif // SERVICE_TYPES_H
