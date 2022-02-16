IFDEF RAX
ELSE
.model flat, stdcall
ENDIF

EXTERNDEF g_proxyCallbacksArray:BYTE

.CODE

ProxyCallbackProc MACRO index:REQ
IFDEF RAX
	pop rax
	jmp QWORD PTR [g_proxyCallbacksArray + index * 8]
ELSE
	jmp DWORD PTR [g_proxyCallbacksArray + index * 4]
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

proxyCallback5 PROC
ProxyCallbackProc(5)
proxyCallback5 ENDP

proxyCallback6 PROC
ProxyCallbackProc(6)
proxyCallback6 ENDP

proxyCallback7 PROC
ProxyCallbackProc(7)
proxyCallback7 ENDP

proxyCallback8 PROC
ProxyCallbackProc(8)
proxyCallback8 ENDP

proxyCallback9 PROC
ProxyCallbackProc(9)
proxyCallback9 ENDP

proxyCallback10 PROC
ProxyCallbackProc(10)
proxyCallback10 ENDP

proxyCallback11 PROC
ProxyCallbackProc(11)
proxyCallback11 ENDP

proxyCallback12 PROC
ProxyCallbackProc(12)
proxyCallback12 ENDP

proxyCallback13 PROC
ProxyCallbackProc(13)
proxyCallback13 ENDP

proxyCallback14 PROC
ProxyCallbackProc(14)
proxyCallback14 ENDP

proxyCallback15 PROC
ProxyCallbackProc(15)
proxyCallback15 ENDP

proxyCallback16 PROC
ProxyCallbackProc(16)
proxyCallback16 ENDP

proxyCallback17 PROC
ProxyCallbackProc(17)
proxyCallback17 ENDP

proxyCallback18 PROC
ProxyCallbackProc(18)
proxyCallback18 ENDP

proxyCallback19 PROC
ProxyCallbackProc(19)
proxyCallback19 ENDP

proxyCallback20 PROC
ProxyCallbackProc(20)
proxyCallback20 ENDP

proxyCallback21 PROC
ProxyCallbackProc(21)
proxyCallback21 ENDP

proxyCallback22 PROC
ProxyCallbackProc(22)
proxyCallback22 ENDP

proxyCallback23 PROC
ProxyCallbackProc(23)
proxyCallback23 ENDP

proxyCallback24 PROC
ProxyCallbackProc(24)
proxyCallback24 ENDP

proxyCallback25 PROC
ProxyCallbackProc(25)
proxyCallback25 ENDP

proxyCallback26 PROC
ProxyCallbackProc(26)
proxyCallback26 ENDP

proxyCallback27 PROC
ProxyCallbackProc(27)
proxyCallback27 ENDP

proxyCallback28 PROC
ProxyCallbackProc(28)
proxyCallback28 ENDP

proxyCallback29 PROC
ProxyCallbackProc(29)
proxyCallback29 ENDP

proxyCallback30 PROC
ProxyCallbackProc(30)
proxyCallback30 ENDP

proxyCallback31 PROC
ProxyCallbackProc(31)
proxyCallback31 ENDP

proxyCallback32 PROC
ProxyCallbackProc(32)
proxyCallback32 ENDP

proxyCallback33 PROC
ProxyCallbackProc(33)
proxyCallback33 ENDP

proxyCallback34 PROC
ProxyCallbackProc(34)
proxyCallback34 ENDP

proxyCallback35 PROC
ProxyCallbackProc(35)
proxyCallback35 ENDP

proxyCallback36 PROC
ProxyCallbackProc(36)
proxyCallback36 ENDP

proxyCallback37 PROC
ProxyCallbackProc(37)
proxyCallback37 ENDP

proxyCallback38 PROC
ProxyCallbackProc(38)
proxyCallback38 ENDP

proxyCallback39 PROC
ProxyCallbackProc(39)
proxyCallback39 ENDP

proxyCallback40 PROC
ProxyCallbackProc(40)
proxyCallback40 ENDP

proxyCallback41 PROC
ProxyCallbackProc(41)
proxyCallback41 ENDP

proxyCallback42 PROC
ProxyCallbackProc(42)
proxyCallback42 ENDP

proxyCallback43 PROC
ProxyCallbackProc(43)
proxyCallback43 ENDP

proxyCallback44 PROC
ProxyCallbackProc(44)
proxyCallback44 ENDP

proxyCallback45 PROC
ProxyCallbackProc(45)
proxyCallback45 ENDP

proxyCallback46 PROC
ProxyCallbackProc(46)
proxyCallback46 ENDP

proxyCallback47 PROC
ProxyCallbackProc(47)
proxyCallback47 ENDP

proxyCallback48 PROC
ProxyCallbackProc(48)
proxyCallback48 ENDP

proxyCallback49 PROC
ProxyCallbackProc(49)
proxyCallback49 ENDP

proxyCallback50 PROC
ProxyCallbackProc(50)
proxyCallback50 ENDP

proxyCallback51 PROC
ProxyCallbackProc(51)
proxyCallback51 ENDP

proxyCallback52 PROC
ProxyCallbackProc(52)
proxyCallback52 ENDP

proxyCallback53 PROC
ProxyCallbackProc(53)
proxyCallback53 ENDP

proxyCallback54 PROC
ProxyCallbackProc(54)
proxyCallback54 ENDP

proxyCallback55 PROC
ProxyCallbackProc(55)
proxyCallback55 ENDP

proxyCallback56 PROC
ProxyCallbackProc(56)
proxyCallback56 ENDP

proxyCallback57 PROC
ProxyCallbackProc(57)
proxyCallback57 ENDP

proxyCallback58 PROC
ProxyCallbackProc(58)
proxyCallback58 ENDP

proxyCallback59 PROC
ProxyCallbackProc(59)
proxyCallback59 ENDP

proxyCallback60 PROC
ProxyCallbackProc(60)
proxyCallback60 ENDP

proxyCallback61 PROC
ProxyCallbackProc(61)
proxyCallback61 ENDP

proxyCallback62 PROC
ProxyCallbackProc(62)
proxyCallback62 ENDP

proxyCallback63 PROC
ProxyCallbackProc(63)
proxyCallback63 ENDP

END
