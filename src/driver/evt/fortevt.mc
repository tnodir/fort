;/* Fort Firewall Driver Event Messages */
;
;#ifndef FORTEVT_H
;#define FORTEVT_H
;

MessageIdTypedef = NTSTATUS

SeverityNames = (
    Success = 0:STATUS_SEVERITY_SUCCESS
    Informational = 1:STATUS_SEVERITY_INFORMATIONAL
    Warning = 2:STATUS_SEVERITY_WARNING
    Error = 3:STATUS_SEVERITY_ERROR
)

FacilityNames = (
    System = 0
    Buffer = 1:FACILITY_BUFFER
    Callout = 2:FACILITY_CALLOUT
    Device = 3:FACILITY_DEVICE
    Driver = 4:FACILITY_DRIVER
    Shaper = 5:FACILITY_SHAPER
    ProcessTree = 6:FACILITY_PROCESS_TREE
)


;/* Buffer */
MessageId=1 Facility=Buffer Severity=Error SymbolicName=FORT_BUFFER_OOM
Language=English
Buffer OOM.
.


;/* Callout */
MessageId=1 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_FLOW_ASSOC_ERROR
Language=English
Classify v4: Flow assoc. error.
.

MessageId=2 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_CONNECT_V4_ERROR
Language=English
Register Connect V4: Error.
.

MessageId=3 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_CONNECT_V6_ERROR
Language=English
Register Connect V6: Error.
.

MessageId=4 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_ACCEPT_V4_ERROR
Language=English
Register Accept V4: Error.
.

MessageId=5 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_ACCEPT_V6_ERROR
Language=English
Register Accept V6: Error.
.

MessageId=6 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_STREAM_V4_ERROR
Language=English
Register Stream V4: Error.
.

MessageId=7 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_STREAM_V6_ERROR
Language=English
Register Stream V6: Error.
.

MessageId=8 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_DATAGRAM_V4_ERROR
Language=English
Register Datagram V4: Error.
.

MessageId=9 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_DATAGRAM_V6_ERROR
Language=English
Register Datagram V6: Error.
.

MessageId=10 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_INBOUND_TRANSPORT_V4_ERROR
Language=English
Register Inbound Transport V4: Error.
.

MessageId=11 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_INBOUND_TRANSPORT_V6_ERROR
Language=English
Register Inbound Transport V6: Error.
.

MessageId=12 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_OUTBOUND_TRANSPORT_V4_ERROR
Language=English
Register Outbound Transport V4: Error.
.

MessageId=13 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_OUTBOUND_TRANSPORT_V6_ERROR
Language=English
Register Outbound Transport V6: Error.
.

MessageId=14 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_CALLOUT_REAUTH_ERROR
Language=English
Callout Reauth: Error.
.

MessageId=15 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_CALLOUT_TIMER_ERROR
Language=English
Callout Timer: Error.
.


;/* Device */
MessageId=1 Facility=Device Severity=Error SymbolicName=FORT_DEVICE_WORKER_REAUTH_ERROR
Language=English
Worker Reauth: Error.
.

MessageId=2 Facility=Device Severity=Error SymbolicName=FORT_DEVICE_DEVICE_CONTROL_ERROR
Language=English
Device Control: Error.
.


;/* Driver */
MessageId=1 Facility=Driver Severity=Error SymbolicName=FORT_DRIVER_ENTRY_ERROR
Language=English
Entry: Error.
.


;/* Shaper */
MessageId=1 Facility=Shaper Severity=Error SymbolicName=FORT_SHAPER_PACKET_INJECTION_ERROR
Language=English
Shaper: Packet injection error.
.

MessageId=2 Facility=Shaper Severity=Error SymbolicName=FORT_SHAPER_PACKET_INJECTION_CALL_ERROR
Language=English
Shaper: Packet injection call error.
.

MessageId=3 Facility=Shaper Severity=Error SymbolicName=FORT_SHAPER_PACKET_CLONE_ERROR
Language=English
Shaper: Packet clone error.
.


;/* ProcessTree */
MessageId=1 Facility=ProcessTree Severity=Error SymbolicName=FORT_PSTREE_UPDATE_ERROR
Language=English
PsTree: Update Error.
.

MessageId=2 Facility=ProcessTree Severity=Error SymbolicName=FORT_PSTREE_ENUM_PROCESSES_ERROR
Language=English
Enum Processes Error.
.


;#endif // FORTEVT_H
