Work as Administrator!

1. Turn on the test signing mode: "bcdedit.exe /set TESTSIGNING ON"
2. Create test certificate: "Makecert -r -n "CN=FortFirewallTestCert" -ss "FortFirewall" -sr LocalMachine"
3. Run -> mmc -> File -> Add or remove...
- Select "Certificates" -> Add -> Computer -> Local machine -> OK;
- Certificates (Local machine) -> Copy certificate from "FortFirewall" into "Trusted root certification centers".
4. Sign the driver: "signtool sign /a /v /s FortFirewall /n "FortFirewallTestCert" fortfw64.sys"
