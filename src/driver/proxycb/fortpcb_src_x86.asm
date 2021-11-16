IFDEF RAX
ELSE
.model flat, stdcall
ENDIF

.DATA
IFDEF RAX
	g_proxyDstProcs QWORD 040H dup (?)
ELSE
	g_proxyDstProcs DWORD 040H dup (?)
ENDIF

.CODE

ProxyCallbackProc MACRO index:REQ
IFDEF RAX
	push rax
	mov rax, [g_proxyDstProcs + index * 8]
	jmp rax
ELSE
	jmp DWORD PTR [g_proxyDstProcs + index * 4]
ENDIF
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
