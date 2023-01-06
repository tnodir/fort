/* Fort Firewall Driver Event Messages */

#ifndef FORTEVT_H
#define FORTEVT_H

/* Buffer */
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_BUFFER                  0x1
#define FACILITY_CALLOUT                 0x2
#define FACILITY_DEVICE                  0x3
#define FACILITY_DRIVER                  0x4
#define FACILITY_SHAPER                  0x5
#define FACILITY_PROCESS_TREE            0x6


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: FORT_BUFFER_OOM
//
// MessageText:
//
// Buffer OOM.
//
#define FORT_BUFFER_OOM                  ((NTSTATUS)0xC0010001L)

/* Callout */
//
// MessageId: FORT_CALLOUT_FLOW_ASSOC_ERROR
//
// MessageText:
//
// Classify v4: Flow assoc. error.
//
#define FORT_CALLOUT_FLOW_ASSOC_ERROR    ((NTSTATUS)0xC0020001L)

//
// MessageId: FORT_CALLOUT_REGISTER_CONNECT_V4_ERROR
//
// MessageText:
//
// Register Connect V4: Error.
//
#define FORT_CALLOUT_REGISTER_CONNECT_V4_ERROR ((NTSTATUS)0xC0020002L)

//
// MessageId: FORT_CALLOUT_REGISTER_CONNECT_V6_ERROR
//
// MessageText:
//
// Register Connect V6: Error.
//
#define FORT_CALLOUT_REGISTER_CONNECT_V6_ERROR ((NTSTATUS)0xC0020003L)

//
// MessageId: FORT_CALLOUT_REGISTER_ACCEPT_V4_ERROR
//
// MessageText:
//
// Register Accept V4: Error.
//
#define FORT_CALLOUT_REGISTER_ACCEPT_V4_ERROR ((NTSTATUS)0xC0020004L)

//
// MessageId: FORT_CALLOUT_REGISTER_ACCEPT_V6_ERROR
//
// MessageText:
//
// Register Accept V6: Error.
//
#define FORT_CALLOUT_REGISTER_ACCEPT_V6_ERROR ((NTSTATUS)0xC0020005L)

//
// MessageId: FORT_CALLOUT_REGISTER_STREAM_V4_ERROR
//
// MessageText:
//
// Register Stream V4: Error.
//
#define FORT_CALLOUT_REGISTER_STREAM_V4_ERROR ((NTSTATUS)0xC0020006L)

//
// MessageId: FORT_CALLOUT_REGISTER_STREAM_V6_ERROR
//
// MessageText:
//
// Register Stream V6: Error.
//
#define FORT_CALLOUT_REGISTER_STREAM_V6_ERROR ((NTSTATUS)0xC0020007L)

//
// MessageId: FORT_CALLOUT_REGISTER_DATAGRAM_V4_ERROR
//
// MessageText:
//
// Register Datagram V4: Error.
//
#define FORT_CALLOUT_REGISTER_DATAGRAM_V4_ERROR ((NTSTATUS)0xC0020008L)

//
// MessageId: FORT_CALLOUT_REGISTER_DATAGRAM_V6_ERROR
//
// MessageText:
//
// Register Datagram V6: Error.
//
#define FORT_CALLOUT_REGISTER_DATAGRAM_V6_ERROR ((NTSTATUS)0xC0020009L)

//
// MessageId: FORT_CALLOUT_REGISTER_INBOUND_TRANSPORT_V4_ERROR
//
// MessageText:
//
// Register Inbound Transport V4: Error.
//
#define FORT_CALLOUT_REGISTER_INBOUND_TRANSPORT_V4_ERROR ((NTSTATUS)0xC002000AL)

//
// MessageId: FORT_CALLOUT_REGISTER_INBOUND_TRANSPORT_V6_ERROR
//
// MessageText:
//
// Register Inbound Transport V6: Error.
//
#define FORT_CALLOUT_REGISTER_INBOUND_TRANSPORT_V6_ERROR ((NTSTATUS)0xC002000BL)

//
// MessageId: FORT_CALLOUT_REGISTER_OUTBOUND_TRANSPORT_V4_ERROR
//
// MessageText:
//
// Register Outbound Transport V4: Error.
//
#define FORT_CALLOUT_REGISTER_OUTBOUND_TRANSPORT_V4_ERROR ((NTSTATUS)0xC002000CL)

//
// MessageId: FORT_CALLOUT_REGISTER_OUTBOUND_TRANSPORT_V6_ERROR
//
// MessageText:
//
// Register Outbound Transport V6: Error.
//
#define FORT_CALLOUT_REGISTER_OUTBOUND_TRANSPORT_V6_ERROR ((NTSTATUS)0xC002000DL)

//
// MessageId: FORT_CALLOUT_CALLOUT_REAUTH_ERROR
//
// MessageText:
//
// Callout Reauth: Error.
//
#define FORT_CALLOUT_CALLOUT_REAUTH_ERROR ((NTSTATUS)0xC002000EL)

//
// MessageId: FORT_CALLOUT_CALLOUT_TIMER_ERROR
//
// MessageText:
//
// Callout Timer: Error.
//
#define FORT_CALLOUT_CALLOUT_TIMER_ERROR ((NTSTATUS)0xC002000FL)

/* Device */
//
// MessageId: FORT_DEVICE_WORKER_REAUTH_ERROR
//
// MessageText:
//
// Worker Reauth: Error.
//
#define FORT_DEVICE_WORKER_REAUTH_ERROR  ((NTSTATUS)0xC0030001L)

//
// MessageId: FORT_DEVICE_DEVICE_CONTROL_ERROR
//
// MessageText:
//
// Device Control: Error.
//
#define FORT_DEVICE_DEVICE_CONTROL_ERROR ((NTSTATUS)0xC0030002L)

/* Driver */
//
// MessageId: FORT_DRIVER_ENTRY_ERROR
//
// MessageText:
//
// Entry: Error.
//
#define FORT_DRIVER_ENTRY_ERROR          ((NTSTATUS)0xC0040001L)

/* Shaper */
//
// MessageId: FORT_SHAPER_PACKET_INJECTION_ERROR
//
// MessageText:
//
// Shaper: Packet injection error.
//
#define FORT_SHAPER_PACKET_INJECTION_ERROR ((NTSTATUS)0xC0050001L)

//
// MessageId: FORT_SHAPER_PACKET_INJECTION_CALL_ERROR
//
// MessageText:
//
// Shaper: Packet injection call error.
//
#define FORT_SHAPER_PACKET_INJECTION_CALL_ERROR ((NTSTATUS)0xC0050002L)

/* ProcessTree */
//
// MessageId: FORT_PSTREE_UPDATE_ERROR
//
// MessageText:
//
// PsTree: Update Error.
//
#define FORT_PSTREE_UPDATE_ERROR         ((NTSTATUS)0xC0060001L)

//
// MessageId: FORT_PSTREE_ENUM_PROCESSES_ERROR
//
// MessageText:
//
// Enum Processes Error.
//
#define FORT_PSTREE_ENUM_PROCESSES_ERROR ((NTSTATUS)0xC0060002L)

#endif // FORTEVT_H
