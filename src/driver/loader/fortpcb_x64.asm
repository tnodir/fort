.DATA
g_proxiedCallbacks QWORD 040H dup (?)

.CODE

ProxyCallbackProc MACRO index:REQ
	jmp QWORD PTR [g_proxiedCallbacks + index * 8]
ENDM

proxyCallback0 PROC
ProxyCallbackProc(0)
proxyCallback0 ENDP

proxyCallback1 PROC
ProxyCallbackProc(1)
proxyCallback1 ENDP

proxyCallback2 PROC
ProxyCallbackProc(2)
proxyCallback2 ENDP

proxyCallback3 PROC
ProxyCallbackProc(3)
proxyCallback3 ENDP

proxyCallback4 PROC
ProxyCallbackProc(4)
proxyCallback4 ENDP

END
