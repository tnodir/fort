/* Fort Firewall Driver Trace Events to System Log */

#include "forttrace.h"

#include "fortdev.h"

FORT_API void fort_trace_event(
        NTSTATUS event_code, NTSTATUS status, ULONG error_value, ULONG sequence)
{
    if (KeGetCurrentIrql() > DISPATCH_LEVEL || fort_device() == NULL)
        return;

    PIO_ERROR_LOG_PACKET packet =
            IoAllocateErrorLogEntry(fort_device()->device, sizeof(IO_ERROR_LOG_PACKET));
    if (packet == NULL)
        return;

    packet->MajorFunctionCode = 0;
    packet->RetryCount = 0;
    packet->DumpDataSize = 0;
    packet->NumberOfStrings = 0;
    packet->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
    packet->EventCategory = 0;
    packet->ErrorCode = event_code;
    packet->UniqueErrorValue = error_value;
    packet->FinalStatus = status;
    packet->SequenceNumber = sequence;
    packet->IoControlCode = 0;

    IoWriteErrorLogEntry(packet);
}
