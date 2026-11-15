#include "dnsetwlistener.h"

#include <QLoggingCategory>
#include <QOperatingSystemVersion>
#include <QThread>
#include <QtEndian>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <evntcons.h>
#include <evntrace.h>
#include <tdh.h>
#include <windns.h>

// Declared for Windows 8+ only
#ifndef EVENT_TRACE_USE_MS_FLUSH_TIMER
#    define EVENT_TRACE_USE_MS_FLUSH_TIMER 0x00000010
#endif

namespace {

const QLoggingCategory LC("dns.etwListener");

// Microsoft-Windows-DNS-Client
inline constexpr GUID dnsClientProviderGuid = { 0x1c95126e, 0x7eea, 0x49a9,
    { 0xa3, 0xfe, 0xa3, 0x78, 0xb0, 0x3d, 0xdb, 0x4d } };

inline constexpr USHORT queryCompletedEventId = 3008;

inline constexpr wchar_t sessionName[] = L"FortFirewallDns";

struct SessionProperties
{
    EVENT_TRACE_PROPERTIES props;
    wchar_t name[std::size(sessionName)];
};

SessionProperties makeSessionProperties()
{
    SessionProperties sp {};

    sp.props.Wnode.BufferSize = sizeof(SessionProperties);
    sp.props.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    sp.props.Wnode.ClientContext = 1; // QueryPerformanceCounter
    sp.props.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    sp.props.LoggerNameOffset = offsetof(SessionProperties, name);

    // Deliver the events soon: the programs connect right after the answer
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8) {
        sp.props.LogFileMode |= EVENT_TRACE_USE_MS_FLUSH_TIMER;
        sp.props.FlushTimer = 100; // milliseconds
    } else {
        sp.props.FlushTimer = 1; // seconds
    }

    return sp;
}

void stopTraceSession(TRACEHANDLE handle)
{
    SessionProperties sp = makeSessionProperties();

    ControlTraceW(handle, sessionName, &sp.props, EVENT_TRACE_CONTROL_STOP);
}

ULONG startTraceSession(TRACEHANDLE &handle)
{
    SessionProperties sp = makeSessionProperties();

    const ULONG res = StartTraceW(&handle, sessionName, &sp.props);
    if (res != ERROR_ALREADY_EXISTS)
        return res;

    // The session of a crashed process
    stopTraceSession(/*handle=*/0);

    sp = makeSessionProperties();

    return StartTraceW(&handle, sessionName, &sp.props);
}

void processTrace(TRACEHANDLE traceHandle)
{
    const ULONG res = ProcessTrace(&traceHandle, 1, nullptr, nullptr);

    if (res != ERROR_SUCCESS && res != ERROR_CANCELLED) {
        qCWarning(LC) << "ProcessTrace error:" << res;
    }
}

QByteArray eventProperty(PEVENT_RECORD event, const wchar_t *name)
{
    PROPERTY_DATA_DESCRIPTOR descriptor {};
    descriptor.PropertyName = ULONGLONG(name);
    descriptor.ArrayIndex = ULONG_MAX;

    ULONG size = 0;
    if (TdhGetPropertySize(event, 0, nullptr, 1, &descriptor, &size) != ERROR_SUCCESS)
        return {};

    QByteArray data(size, Qt::Uninitialized);

    if (TdhGetProperty(event, 0, nullptr, 1, &descriptor, size, PBYTE(data.data()))
            != ERROR_SUCCESS)
        return {};

    return data;
}

QString eventPropertyString(PEVENT_RECORD event, const wchar_t *name)
{
    const QByteArray data = eventProperty(event, name);

    const auto chars = reinterpret_cast<const wchar_t *>(data.constData());
    const size_t len = wcsnlen(chars, data.size() / sizeof(wchar_t));

    return QString::fromWCharArray(chars, qsizetype(len));
}

quint32 eventPropertyUInt32(PEVENT_RECORD event, const wchar_t *name, quint32 defaultValue)
{
    const QByteArray data = eventProperty(event, name);
    if (data.size() != sizeof(quint32))
        return defaultValue;

    return qFromLittleEndian<quint32>(data.constData());
}

bool isAddressQueryType(quint32 queryType)
{
    return queryType == DNS_TYPE_A || queryType == DNS_TYPE_AAAA;
}

ULONG enableProvider(TRACEHANDLE sessionHandle, PENABLE_TRACE_PARAMETERS params)
{
    return EnableTraceEx2(sessionHandle, &dnsClientProviderGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            TRACE_LEVEL_INFORMATION, /*MatchAnyKeyword=*/0, /*MatchAllKeyword=*/0, /*Timeout=*/0,
            params);
}

// The provider writes several events per query: only the completed queries' ones are delivered
ULONG enableProviderEvents(TRACEHANDLE sessionHandle)
{
    EVENT_FILTER_EVENT_ID eventIds {};
    eventIds.FilterIn = TRUE;
    eventIds.Count = 1;
    eventIds.Events[0] = queryCompletedEventId;

    EVENT_FILTER_DESCRIPTOR filterDesc {};
    filterDesc.Ptr = ULONGLONG(&eventIds);
    filterDesc.Size = sizeof(eventIds);
    filterDesc.Type = EVENT_FILTER_TYPE_EVENT_ID;

    ENABLE_TRACE_PARAMETERS params {};
    params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    params.EnableFilterDesc = &filterDesc;
    params.FilterDescCount = 1;

    if (enableProvider(sessionHandle, &params) == ERROR_SUCCESS)
        return ERROR_SUCCESS;

    // The events' filter is of Windows 8.1+
    return enableProvider(sessionHandle, /*params=*/nullptr);
}

}

DnsEtwListener::DnsEtwListener(QObject *parent) : QObject(parent) { }

DnsEtwListener::~DnsEtwListener()
{
    close();
}

bool DnsEtwListener::open()
{
    if (!(startSession() && openTrace())) {
        close();
        return false;
    }

    m_traceThread.reset(QThread::create(processTrace, TRACEHANDLE(m_traceHandle)));
    m_traceThread->start();

    return true;
}

void DnsEtwListener::close()
{
    stopSession();
    closeTrace();

    if (m_traceThread) {
        m_traceThread->wait();
        m_traceThread.reset();
    }
}

bool DnsEtwListener::startSession()
{
    TRACEHANDLE sessionHandle = 0;

    ULONG res = startTraceSession(sessionHandle);
    if (res != ERROR_SUCCESS) {
        qCWarning(LC) << "StartTrace error:" << res;
        return false;
    }

    m_sessionHandle = sessionHandle;

    res = enableProviderEvents(sessionHandle);

    if (res != ERROR_SUCCESS) {
        qCWarning(LC) << "EnableTrace error:" << res;
        return false;
    }

    return true;
}

void DnsEtwListener::stopSession()
{
    if (m_sessionHandle == 0)
        return;

    stopTraceSession(m_sessionHandle);
    m_sessionHandle = 0;
}

bool DnsEtwListener::openTrace()
{
    EVENT_TRACE_LOGFILEW logFile {};
    logFile.LoggerName = const_cast<LPWSTR>(sessionName);
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.EventRecordCallback = &DnsEtwListener::onEventRecord;
    logFile.Context = this;

    const TRACEHANDLE traceHandle = OpenTraceW(&logFile);

    if (traceHandle == INVALID_PROCESSTRACE_HANDLE) {
        qCWarning(LC) << "OpenTrace error:" << GetLastError();
        return false;
    }

    m_traceHandle = traceHandle;

    return true;
}

void DnsEtwListener::closeTrace()
{
    if (m_traceHandle == 0)
        return;

    CloseTrace(m_traceHandle);
    m_traceHandle = 0;
}

void DnsEtwListener::processEvent(PEVENT_RECORD event)
{
    if (event->EventHeader.EventDescriptor.Id != queryCompletedEventId)
        return;

    if (eventPropertyUInt32(event, L"QueryStatus", /*defaultValue=*/1) != ERROR_SUCCESS)
        return; // failed query

    const quint32 queryType = eventPropertyUInt32(event, L"QueryType", /*defaultValue=*/0);
    if (!isAddressQueryType(queryType))
        return;

    DnsClientAnswer answer;
    answer.setProcessId(event->EventHeader.ProcessId);
    answer.setName(eventPropertyString(event, L"QueryName"));
    answer.readQueryResults(eventPropertyString(event, L"QueryResults"));

    if (answer.addresses().isEmpty())
        return;

    emit answerReceived(answer);
}

void DnsEtwListener::onEventRecord(PEVENT_RECORD event)
{
    auto listener = static_cast<DnsEtwListener *>(event->UserContext);

    listener->processEvent(event);
}
