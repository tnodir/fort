;/* Fort Firewall Driver Event Messages */
;
;#ifndef FORTEVT_H
;#define FORTEVT_H
;

MessageIdTypedef = NTSTATUS

SeverityNames = (
    Success = 0:STATUS_SEVERITY_SUCCESS
    Info = 1:STATUS_SEVERITY_INFORMATIONAL
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
    Config = 7:FACILITY_CONFIG
)


;/* Buffer */
MessageId=10 Facility=Buffer Severity=Error SymbolicName=FORT_BUFFER_OOM
Language=English
Buffer OOM.
.


;/* Callout */
MessageId=20 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_FLOW_ASSOC_ERROR
Language=English
Classify v4: Flow assoc. error.
.

MessageId=21 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_REGISTER_ERROR
Language=English
Callout Register: Error.
.

MessageId=22 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_CALLOUT_REAUTH_ERROR
Language=English
Callout Reauth: Error.
.

MessageId=23 Facility=Callout Severity=Error SymbolicName=FORT_CALLOUT_CALLOUT_TIMER_ERROR
Language=English
Callout Timer: Error.
.


;/* Device */
MessageId=30 Facility=Device Severity=Error SymbolicName=FORT_DEVICE_DEVICE_CONTROL_ERROR
Language=English
Device Control: Error.
.


;/* Driver */
MessageId=40 Facility=Driver Severity=Error SymbolicName=FORT_DRIVER_ENTRY_ERROR
Language=English
Entry: Error.
.


;/* Shaper */
MessageId=50 Facility=Shaper Severity=Error SymbolicName=FORT_SHAPER_PACKET_INJECTION_ERROR
Language=English
Shaper: Packet injection error.
.

MessageId=51 Facility=Shaper Severity=Error SymbolicName=FORT_SHAPER_PACKET_INJECTION_CALL_ERROR
Language=English
Shaper: Packet injection call error.
.

MessageId=52 Facility=Shaper Severity=Error SymbolicName=FORT_SHAPER_PACKET_CLONE_ERROR
Language=English
Shaper: Packet clone error.
.


;/* ProcessTree */
MessageId=60 Facility=ProcessTree Severity=Error SymbolicName=FORT_PSTREE_UPDATE_ERROR
Language=English
PsTree: Update Error.
.

MessageId=61 Facility=ProcessTree Severity=Error SymbolicName=FORT_PSTREE_ENUM_PROCESSES_ERROR
Language=English
Enum Processes Error.
.


;/* Config */
MessageId=1000 Facility=Config Severity=Info SymbolicName=FORT_CONFIG_FILTER_OFF
Language=English
Config: Filter Off.
.

MessageId=1001 Facility=Config Severity=Info SymbolicName=FORT_CONFIG_FILTER_ON
Language=English
Config: Filter On.
.

MessageId=1010 Facility=Config Severity=Info SymbolicName=FORT_CONFIG_FILTER_MODE_AUTO_LEARN
Language=English
Config: Filter Mode: Auto-Learn.
.

MessageId=1011 Facility=Config Severity=Info SymbolicName=FORT_CONFIG_FILTER_MODE_ASK
Language=English
Config: Filter Mode: Ask.
.

MessageId=1012 Facility=Config Severity=Info SymbolicName=FORT_CONFIG_FILTER_MODE_BLOCK
Language=English
Config: Filter Mode: Block.
.

MessageId=1013 Facility=Config Severity=Info SymbolicName=FORT_CONFIG_FILTER_MODE_ALLOW
Language=English
Config: Filter Mode: Allow.
.

MessageId=1014 Facility=Config Severity=Info SymbolicName=FORT_CONFIG_FILTER_MODE_IGNORE
Language=English
Config: Filter Mode: Ignore.
.


;#endif // FORTEVT_H
