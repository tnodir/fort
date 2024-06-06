#ifndef SERVICE_TYPES_H
#define SERVICE_TYPES_H

enum ServiceControlCode {
    ServiceControlStop = 128,
    ServiceControlStopRestarting,
    ServiceControlStopUninstall,
};

#endif // SERVICE_TYPES_H
