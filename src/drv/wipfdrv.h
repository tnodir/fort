#ifndef WIPFDRV_H
#define WIPFDRV_H

#define NT_DEVICE_NAME		L"\\Device\\wipf"
#define DOS_DEVICE_NAME		L"\\DosDevices\\wipf"

#define WIPF_POOL_TAG	'WIPF'

#define wipf_request_complete(irp, status) \
  do { \
    (irp)->IoStatus.Status = (status); \
    (irp)->IoStatus.Information = 0; \
    IoCompleteRequest((irp), IO_NO_INCREMENT); \
  } while(0)

#endif WIPFDRV_H
