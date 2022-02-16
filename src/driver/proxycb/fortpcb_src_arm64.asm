	AREA fortpcb_src, CODE

	IMPORT g_proxyDstCallbacksArray

	MACRO
	ProxyCallbackProc $index
		adrl x9, g_proxyDstCallbacksArray
		mov x10, #$index
		ldr x9, [x9, x10, lsl#3]
		br x9
	MEND

	EXPORT proxyDstCallback0
proxyDstCallback0
	ProxyCallbackProc 0

	EXPORT proxyDstCallback1
proxyDstCallback1
	ProxyCallbackProc 1

	EXPORT proxyDstCallback2
proxyDstCallback2
	ProxyCallbackProc 2

	EXPORT proxyDstCallback3
proxyDstCallback3
	ProxyCallbackProc 3

	EXPORT proxyDstCallback4
proxyDstCallback4
	ProxyCallbackProc 4

	EXPORT proxyDstCallback5
proxyDstCallback5
	ProxyCallbackProc 5

	EXPORT proxyDstCallback6
proxyDstCallback6
	ProxyCallbackProc 6

	EXPORT proxyDstCallback7
proxyDstCallback7
	ProxyCallbackProc 7

	EXPORT proxyDstCallback8
proxyDstCallback8
	ProxyCallbackProc 8

	EXPORT proxyDstCallback9
proxyDstCallback9
	ProxyCallbackProc 9

	EXPORT proxyDstCallback10
proxyDstCallback10
	ProxyCallbackProc 10

	EXPORT proxyDstCallback11
proxyDstCallback11
	ProxyCallbackProc 11

	EXPORT proxyDstCallback12
proxyDstCallback12
	ProxyCallbackProc 12

	EXPORT proxyDstCallback13
proxyDstCallback13
	ProxyCallbackProc 13

	EXPORT proxyDstCallback14
proxyDstCallback14
	ProxyCallbackProc 14

	EXPORT proxyDstCallback15
proxyDstCallback15
	ProxyCallbackProc 15

	EXPORT proxyDstCallback16
proxyDstCallback16
	ProxyCallbackProc 16

	EXPORT proxyDstCallback17
proxyDstCallback17
	ProxyCallbackProc 17

	EXPORT proxyDstCallback18
proxyDstCallback18
	ProxyCallbackProc 18

	EXPORT proxyDstCallback19
proxyDstCallback19
	ProxyCallbackProc 19

	EXPORT proxyDstCallback20
proxyDstCallback20
	ProxyCallbackProc 20

	EXPORT proxyDstCallback21
proxyDstCallback21
	ProxyCallbackProc 21

	EXPORT proxyDstCallback22
proxyDstCallback22
	ProxyCallbackProc 22

	EXPORT proxyDstCallback23
proxyDstCallback23
	ProxyCallbackProc 23

	EXPORT proxyDstCallback24
proxyDstCallback24
	ProxyCallbackProc 24

	EXPORT proxyDstCallback25
proxyDstCallback25
	ProxyCallbackProc 25

	EXPORT proxyDstCallback26
proxyDstCallback26
	ProxyCallbackProc 26

	EXPORT proxyDstCallback27
proxyDstCallback27
	ProxyCallbackProc 27

	EXPORT proxyDstCallback28
proxyDstCallback28
	ProxyCallbackProc 28

	EXPORT proxyDstCallback29
proxyDstCallback29
	ProxyCallbackProc 29

	EXPORT proxyDstCallback30
proxyDstCallback30
	ProxyCallbackProc 30

	EXPORT proxyDstCallback31
proxyDstCallback31
	ProxyCallbackProc 31

	EXPORT proxyDstCallback32
proxyDstCallback32
	ProxyCallbackProc 32

	EXPORT proxyDstCallback33
proxyDstCallback33
	ProxyCallbackProc 33

	EXPORT proxyDstCallback34
proxyDstCallback34
	ProxyCallbackProc 34

	EXPORT proxyDstCallback35
proxyDstCallback35
	ProxyCallbackProc 35

	EXPORT proxyDstCallback36
proxyDstCallback36
	ProxyCallbackProc 36

	EXPORT proxyDstCallback37
proxyDstCallback37
	ProxyCallbackProc 37

	EXPORT proxyDstCallback38
proxyDstCallback38
	ProxyCallbackProc 38

	EXPORT proxyDstCallback39
proxyDstCallback39
	ProxyCallbackProc 39

	EXPORT proxyDstCallback40
proxyDstCallback40
	ProxyCallbackProc 40

	EXPORT proxyDstCallback41
proxyDstCallback41
	ProxyCallbackProc 41

	EXPORT proxyDstCallback42
proxyDstCallback42
	ProxyCallbackProc 42

	EXPORT proxyDstCallback43
proxyDstCallback43
	ProxyCallbackProc 43

	EXPORT proxyDstCallback44
proxyDstCallback44
	ProxyCallbackProc 44

	EXPORT proxyDstCallback45
proxyDstCallback45
	ProxyCallbackProc 45

	EXPORT proxyDstCallback46
proxyDstCallback46
	ProxyCallbackProc 46

	EXPORT proxyDstCallback47
proxyDstCallback47
	ProxyCallbackProc 47

	EXPORT proxyDstCallback48
proxyDstCallback48
	ProxyCallbackProc 48

	EXPORT proxyDstCallback49
proxyDstCallback49
	ProxyCallbackProc 49

	EXPORT proxyDstCallback50
proxyDstCallback50
	ProxyCallbackProc 50

	EXPORT proxyDstCallback51
proxyDstCallback51
	ProxyCallbackProc 51

	EXPORT proxyDstCallback52
proxyDstCallback52
	ProxyCallbackProc 52

	EXPORT proxyDstCallback53
proxyDstCallback53
	ProxyCallbackProc 53

	EXPORT proxyDstCallback54
proxyDstCallback54
	ProxyCallbackProc 54

	EXPORT proxyDstCallback55
proxyDstCallback55
	ProxyCallbackProc 55

	EXPORT proxyDstCallback56
proxyDstCallback56
	ProxyCallbackProc 56

	EXPORT proxyDstCallback57
proxyDstCallback57
	ProxyCallbackProc 57

	EXPORT proxyDstCallback58
proxyDstCallback58
	ProxyCallbackProc 58

	EXPORT proxyDstCallback59
proxyDstCallback59
	ProxyCallbackProc 59

	EXPORT proxyDstCallback60
proxyDstCallback60
	ProxyCallbackProc 60

	EXPORT proxyDstCallback61
proxyDstCallback61
	ProxyCallbackProc 61

	EXPORT proxyDstCallback62
proxyDstCallback62
	ProxyCallbackProc 62

	EXPORT proxyDstCallback63
proxyDstCallback63
	ProxyCallbackProc 63

	END
