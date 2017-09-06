MakeCert -r -pe -ss "Fort Firewall" -n "CN=FortFirewallTestCert" FortFirewallTest.cer
CertMgr /add FortFirewallTest.cer /s /r localMachine root
Del FortFirewallTest.cer
