@echo off

@set BIN_PATH=%1
@set HEADER_PATH=%2

python -c "import sys;a=sys.argv;open(a[2],'wb').write((','.join([hex(b) for b in open(a[1],'rb').read()])).encode('utf-8'))" %BIN_PATH% %HEADER_PATH%
