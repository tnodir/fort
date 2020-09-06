#ifndef FORTDRV_H
#define FORTDRV_H

#define NT_DEVICE_NAME		L"\\Device\\fortfw"
#define DOS_DEVICE_NAME		L"\\DosDevices\\fortfw"

#define FORT_DRIVER

#define fort_request_complete_info(irp, status, info) \
  do { \
    (irp)->IoStatus.Status = (status); \
    (irp)->IoStatus.Information = (info); \
    IoCompleteRequest((irp), IO_NO_INCREMENT); \
  } while(0)

#define fort_request_complete(irp, status) \
  fort_request_complete_info((irp), (status), 0)

#endif // FORTDRV_H
