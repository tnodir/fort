	AREA fortpcb_dst, CODE

	IMPORT g_proxyCallbacksArray

	MACRO
	ProxyCallbackProc $index
		adrl x9, g_proxyCallbacksArray
		mov x10, #$index
		ldr x9, [x9, x10, lsl#3]
		br x9
	MEND

	EXPORT proxyCallback0
proxyCallback0
	ProxyCallbackProc 0

	EXPORT proxyCallback1
proxyCallback1
	ProxyCallbackProc 1

	EXPORT proxyCallback2
proxyCallback2
	ProxyCallbackProc 2

	EXPORT proxyCallback3
proxyCallback3
	ProxyCallbackProc 3

	EXPORT proxyCallback4
proxyCallback4
	ProxyCallbackProc 4

	EXPORT proxyCallback5
proxyCallback5
	ProxyCallbackProc 5

	EXPORT proxyCallback6
proxyCallback6
	ProxyCallbackProc 6

	EXPORT proxyCallback7
proxyCallback7
	ProxyCallbackProc 7

	EXPORT proxyCallback8
proxyCallback8
	ProxyCallbackProc 8

	EXPORT proxyCallback9
proxyCallback9
	ProxyCallbackProc 9

	EXPORT proxyCallback10
proxyCallback10
	ProxyCallbackProc 10

	EXPORT proxyCallback11
proxyCallback11
	ProxyCallbackProc 11

	EXPORT proxyCallback12
proxyCallback12
	ProxyCallbackProc 12

	EXPORT proxyCallback13
proxyCallback13
	ProxyCallbackProc 13

	EXPORT proxyCallback14
proxyCallback14
	ProxyCallbackProc 14

	EXPORT proxyCallback15
proxyCallback15
	ProxyCallbackProc 15

	EXPORT proxyCallback16
proxyCallback16
	ProxyCallbackProc 16

	EXPORT proxyCallback17
proxyCallback17
	ProxyCallbackProc 17

	EXPORT proxyCallback18
proxyCallback18
	ProxyCallbackProc 18

	EXPORT proxyCallback19
proxyCallback19
	ProxyCallbackProc 19

	EXPORT proxyCallback20
proxyCallback20
	ProxyCallbackProc 20

	EXPORT proxyCallback21
proxyCallback21
	ProxyCallbackProc 21

	EXPORT proxyCallback22
proxyCallback22
	ProxyCallbackProc 22

	EXPORT proxyCallback23
proxyCallback23
	ProxyCallbackProc 23

	EXPORT proxyCallback24
proxyCallback24
	ProxyCallbackProc 24

	EXPORT proxyCallback25
proxyCallback25
	ProxyCallbackProc 25

	EXPORT proxyCallback26
proxyCallback26
	ProxyCallbackProc 26

	EXPORT proxyCallback27
proxyCallback27
	ProxyCallbackProc 27

	EXPORT proxyCallback28
proxyCallback28
	ProxyCallbackProc 28

	EXPORT proxyCallback29
proxyCallback29
	ProxyCallbackProc 29

	EXPORT proxyCallback30
proxyCallback30
	ProxyCallbackProc 30

	EXPORT proxyCallback31
proxyCallback31
	ProxyCallbackProc 31

	EXPORT proxyCallback32
proxyCallback32
	ProxyCallbackProc 32

	EXPORT proxyCallback33
proxyCallback33
	ProxyCallbackProc 33

	EXPORT proxyCallback34
proxyCallback34
	ProxyCallbackProc 34

	EXPORT proxyCallback35
proxyCallback35
	ProxyCallbackProc 35

	EXPORT proxyCallback36
proxyCallback36
	ProxyCallbackProc 36

	EXPORT proxyCallback37
proxyCallback37
	ProxyCallbackProc 37

	EXPORT proxyCallback38
proxyCallback38
	ProxyCallbackProc 38

	EXPORT proxyCallback39
proxyCallback39
	ProxyCallbackProc 39

	EXPORT proxyCallback40
proxyCallback40
	ProxyCallbackProc 40

	EXPORT proxyCallback41
proxyCallback41
	ProxyCallbackProc 41

	EXPORT proxyCallback42
proxyCallback42
	ProxyCallbackProc 42

	EXPORT proxyCallback43
proxyCallback43
	ProxyCallbackProc 43

	EXPORT proxyCallback44
proxyCallback44
	ProxyCallbackProc 44

	EXPORT proxyCallback45
proxyCallback45
	ProxyCallbackProc 45

	EXPORT proxyCallback46
proxyCallback46
	ProxyCallbackProc 46

	EXPORT proxyCallback47
proxyCallback47
	ProxyCallbackProc 47

	EXPORT proxyCallback48
proxyCallback48
	ProxyCallbackProc 48

	EXPORT proxyCallback49
proxyCallback49
	ProxyCallbackProc 49

	EXPORT proxyCallback50
proxyCallback50
	ProxyCallbackProc 50

	EXPORT proxyCallback51
proxyCallback51
	ProxyCallbackProc 51

	EXPORT proxyCallback52
proxyCallback52
	ProxyCallbackProc 52

	EXPORT proxyCallback53
proxyCallback53
	ProxyCallbackProc 53

	EXPORT proxyCallback54
proxyCallback54
	ProxyCallbackProc 54

	EXPORT proxyCallback55
proxyCallback55
	ProxyCallbackProc 55

	EXPORT proxyCallback56
proxyCallback56
	ProxyCallbackProc 56

	EXPORT proxyCallback57
proxyCallback57
	ProxyCallbackProc 57

	EXPORT proxyCallback58
proxyCallback58
	ProxyCallbackProc 58

	EXPORT proxyCallback59
proxyCallback59
	ProxyCallbackProc 59

	EXPORT proxyCallback60
proxyCallback60
	ProxyCallbackProc 60

	EXPORT proxyCallback61
proxyCallback61
	ProxyCallbackProc 61

	EXPORT proxyCallback62
proxyCallback62
	ProxyCallbackProc 62

	EXPORT proxyCallback63
proxyCallback63
	ProxyCallbackProc 63

	END
