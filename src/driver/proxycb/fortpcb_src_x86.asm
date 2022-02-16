IFDEF RAX
ELSE
.model flat, stdcall
ENDIF

EXTERNDEF g_proxyDstCallbacksArray:BYTE

.CODE

ProxyCallbackProc MACRO index:REQ
IFDEF RAX
	push rax
	mov rax, QWORD PTR [g_proxyDstCallbacksArray + index * 8]
	jmp rax
ELSE
	jmp DWORD PTR [g_proxyDstCallbacksArray + index * 4]
ENDIF
ENDM

proxyDstCallback0 PROC
ProxyCallbackProc(0)
proxyDstCallback0 ENDP

proxyDstCallback1 PROC
ProxyCallbackProc(1)
proxyDstCallback1 ENDP

proxyDstCallback2 PROC
ProxyCallbackProc(2)
proxyDstCallback2 ENDP

proxyDstCallback3 PROC
ProxyCallbackProc(3)
proxyDstCallback3 ENDP

proxyDstCallback4 PROC
ProxyCallbackProc(4)
proxyDstCallback4 ENDP

proxyDstCallback5 PROC
ProxyCallbackProc(5)
proxyDstCallback5 ENDP

proxyDstCallback6 PROC
ProxyCallbackProc(6)
proxyDstCallback6 ENDP

proxyDstCallback7 PROC
ProxyCallbackProc(7)
proxyDstCallback7 ENDP

proxyDstCallback8 PROC
ProxyCallbackProc(8)
proxyDstCallback8 ENDP

proxyDstCallback9 PROC
ProxyCallbackProc(9)
proxyDstCallback9 ENDP

proxyDstCallback10 PROC
ProxyCallbackProc(10)
proxyDstCallback10 ENDP

proxyDstCallback11 PROC
ProxyCallbackProc(11)
proxyDstCallback11 ENDP

proxyDstCallback12 PROC
ProxyCallbackProc(12)
proxyDstCallback12 ENDP

proxyDstCallback13 PROC
ProxyCallbackProc(13)
proxyDstCallback13 ENDP

proxyDstCallback14 PROC
ProxyCallbackProc(14)
proxyDstCallback14 ENDP

proxyDstCallback15 PROC
ProxyCallbackProc(15)
proxyDstCallback15 ENDP

proxyDstCallback16 PROC
ProxyCallbackProc(16)
proxyDstCallback16 ENDP

proxyDstCallback17 PROC
ProxyCallbackProc(17)
proxyDstCallback17 ENDP

proxyDstCallback18 PROC
ProxyCallbackProc(18)
proxyDstCallback18 ENDP

proxyDstCallback19 PROC
ProxyCallbackProc(19)
proxyDstCallback19 ENDP

proxyDstCallback20 PROC
ProxyCallbackProc(20)
proxyDstCallback20 ENDP

proxyDstCallback21 PROC
ProxyCallbackProc(21)
proxyDstCallback21 ENDP

proxyDstCallback22 PROC
ProxyCallbackProc(22)
proxyDstCallback22 ENDP

proxyDstCallback23 PROC
ProxyCallbackProc(23)
proxyDstCallback23 ENDP

proxyDstCallback24 PROC
ProxyCallbackProc(24)
proxyDstCallback24 ENDP

proxyDstCallback25 PROC
ProxyCallbackProc(25)
proxyDstCallback25 ENDP

proxyDstCallback26 PROC
ProxyCallbackProc(26)
proxyDstCallback26 ENDP

proxyDstCallback27 PROC
ProxyCallbackProc(27)
proxyDstCallback27 ENDP

proxyDstCallback28 PROC
ProxyCallbackProc(28)
proxyDstCallback28 ENDP

proxyDstCallback29 PROC
ProxyCallbackProc(29)
proxyDstCallback29 ENDP

proxyDstCallback30 PROC
ProxyCallbackProc(30)
proxyDstCallback30 ENDP

proxyDstCallback31 PROC
ProxyCallbackProc(31)
proxyDstCallback31 ENDP

proxyDstCallback32 PROC
ProxyCallbackProc(32)
proxyDstCallback32 ENDP

proxyDstCallback33 PROC
ProxyCallbackProc(33)
proxyDstCallback33 ENDP

proxyDstCallback34 PROC
ProxyCallbackProc(34)
proxyDstCallback34 ENDP

proxyDstCallback35 PROC
ProxyCallbackProc(35)
proxyDstCallback35 ENDP

proxyDstCallback36 PROC
ProxyCallbackProc(36)
proxyDstCallback36 ENDP

proxyDstCallback37 PROC
ProxyCallbackProc(37)
proxyDstCallback37 ENDP

proxyDstCallback38 PROC
ProxyCallbackProc(38)
proxyDstCallback38 ENDP

proxyDstCallback39 PROC
ProxyCallbackProc(39)
proxyDstCallback39 ENDP

proxyDstCallback40 PROC
ProxyCallbackProc(40)
proxyDstCallback40 ENDP

proxyDstCallback41 PROC
ProxyCallbackProc(41)
proxyDstCallback41 ENDP

proxyDstCallback42 PROC
ProxyCallbackProc(42)
proxyDstCallback42 ENDP

proxyDstCallback43 PROC
ProxyCallbackProc(43)
proxyDstCallback43 ENDP

proxyDstCallback44 PROC
ProxyCallbackProc(44)
proxyDstCallback44 ENDP

proxyDstCallback45 PROC
ProxyCallbackProc(45)
proxyDstCallback45 ENDP

proxyDstCallback46 PROC
ProxyCallbackProc(46)
proxyDstCallback46 ENDP

proxyDstCallback47 PROC
ProxyCallbackProc(47)
proxyDstCallback47 ENDP

proxyDstCallback48 PROC
ProxyCallbackProc(48)
proxyDstCallback48 ENDP

proxyDstCallback49 PROC
ProxyCallbackProc(49)
proxyDstCallback49 ENDP

proxyDstCallback50 PROC
ProxyCallbackProc(50)
proxyDstCallback50 ENDP

proxyDstCallback51 PROC
ProxyCallbackProc(51)
proxyDstCallback51 ENDP

proxyDstCallback52 PROC
ProxyCallbackProc(52)
proxyDstCallback52 ENDP

proxyDstCallback53 PROC
ProxyCallbackProc(53)
proxyDstCallback53 ENDP

proxyDstCallback54 PROC
ProxyCallbackProc(54)
proxyDstCallback54 ENDP

proxyDstCallback55 PROC
ProxyCallbackProc(55)
proxyDstCallback55 ENDP

proxyDstCallback56 PROC
ProxyCallbackProc(56)
proxyDstCallback56 ENDP

proxyDstCallback57 PROC
ProxyCallbackProc(57)
proxyDstCallback57 ENDP

proxyDstCallback58 PROC
ProxyCallbackProc(58)
proxyDstCallback58 ENDP

proxyDstCallback59 PROC
ProxyCallbackProc(59)
proxyDstCallback59 ENDP

proxyDstCallback60 PROC
ProxyCallbackProc(60)
proxyDstCallback60 ENDP

proxyDstCallback61 PROC
ProxyCallbackProc(61)
proxyDstCallback61 ENDP

proxyDstCallback62 PROC
ProxyCallbackProc(62)
proxyDstCallback62 ENDP

proxyDstCallback63 PROC
ProxyCallbackProc(63)
proxyDstCallback63 ENDP

END
