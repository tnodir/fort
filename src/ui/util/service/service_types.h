#ifndef SERVICE_TYPES_H
#define SERVICE_TYPES_H

enum ServiceControlCode {
    ServiceControlStopStandard = 1, // SERVICE_CONTROL_STOP
    ServiceControlStop = 128,
    ServiceControlStopRestarting,
    ServiceControlStopUninstall,
};

#endif // SERVICE_TYPES_H
