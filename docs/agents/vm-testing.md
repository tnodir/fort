# Testing the real driver (test-mode VM)

Moved out of `AGENTS.md`: read it when working on this area.

- The machine needs `bcdedit /set testsigning on` and Secure Boot off. A self-signed code-signing certificate is enough (`signtool sign /fd sha256 /f test.pfx fortfw.sys`); test mode doesn't need it imported.
- Install the plain `fortfw.sys`, bypassing the loader/payload, with **demand start**, so a reboot after a crash comes up without it: copy it to `%SystemRoot%\System32\drivers` and `sc create fortfw binPath= %SystemRoot%\System32\drivers\fortfw.sys type= kernel start= demand group= NetworkProvider depend= BFE`, then `sc start fortfw`. (`driver/scripts/install.bat` uses `start= auto`.)
- `FortFirewall.exe -i service` installs and starts `FortFirewallSvc` (depends on `fortfw`; `sc config FortFirewallSvc start= demand` to keep reboots clean). With a service the app doesn't run `check-reinstall.bat`, so it won't reinstall the driver.
- LAN addresses aren't filtered by default and the first minute after install is the learn mode, so remote access (SSH/RDP over LAN) survives.
- Smoke test via the control CLI: `FortFirewall.exe -c filter report`, `-c filter-mode report`, `-c prog allow|block|report <app-path>`. A `report` returns `70 + value index` as the exit code (e.g. filter-mode `learn` = 70, `block` = 72; prog `allow` = 70, `block` = 71), `99` = error. A renamed copy of `curl.exe` works as an unknown program.
