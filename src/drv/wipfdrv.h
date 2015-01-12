#ifndef WIPFDRV_H
#define WIPFDRV_H

#define NT_DEVICE_NAME		L"\\Device\\wipf"
#define DOS_DEVICE_NAME		L"\\DosDevices\\wipf"

#define wipf_request_complete_info(irp, status, info) \
  do { \
    (irp)->IoStatus.Status = (status); \
    (irp)->IoStatus.Information = (info); \
    IoCompleteRequest((irp), IO_NO_INCREMENT); \
  } while(0)

#define wipf_request_complete(irp, status) \
  wipf_request_complete_info((irp), (status), 0)

#endif WIPFDRV_H
