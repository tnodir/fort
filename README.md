<p align="center">
  <a href="https://github.com/tnodir/fort">
    <img src="https://github.com/tnodir/fort/blob/master/src/ui/icons/fort-96.png" alt="Icon" height="96">
  </a>
  <h1 align="center">Fort Firewall</h1>
</p>

[![Release](https://img.shields.io/github/release/tnodir/fort.svg)](https://github.com/tnodir/fort/releases/latest)
[![Crowdin](https://badges.crowdin.net/fort-firewall/localized.svg)](https://crowdin.com/project/fort-firewall)
[![CodeScene Code Health](https://codescene.io/projects/5344/status-badges/code-health)](https://codescene.io/projects/5344)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=tnodir_fort&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=tnodir_fort)

Fort is an effective firewall designed for Windows 7 and later versions, offering both simplicity and robustness.

<img width="1032" height="805" alt="2026-10-04_202135" src="https://github.com/user-attachments/assets/3c84159d-7021-4211-aa4e-db76564e88a4" />

## Features

  - Standalone firewall with its own driver, independent of Windows Firewall
  - Auto-Learn mode with alerts for new programs
  - Flexible network rules per program or global by addresses, ports, protocols etc
  - Programs by wildcard paths, parent process or `SvcHost.exe` service names
  - Groups of programs, switchable from the tray or by Time Periods
  - Per-program download/upload Speed Limits
  - Blocklists via "Zones", auto-updated from URLs
  - Connections log, traffic statistics and bandwidth graph
  - Password protection and command-line management

## [System Requirements](https://github.com/tnodir/fort/wiki/User-Guide#system-requirements)

  - Install latest [Visual C++ redistributable packages](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version): [x64](https://aka.ms/vs/17/release/vc_redist.x64.exe), [x86](https://aka.ms/vs/17/release/vc_redist.x86.exe), [ARM64](https://aka.ms/vs/17/release/vc_redist.arm64.exe)
  - Disable [HVCI (Core Isolation: Memory Integrity)](https://github.com/tnodir/fort/wiki/HVCI) on Windows 10+

### Which installer should I download?

 **Windows 10 1809** or newer: | [![x86_64@latest](https://img.shields.io/badge/x86_64-green?label=...-x86_64.exe)](https://github.com/tnodir/fort/releases/latest)
-------------------------------|---------
 **Windows 7** or newer:       | [![x86@latest](https://img.shields.io/badge/x86-green?label=...-x86.exe)](https://github.com/tnodir/fort/releases/latest)

-----

## Wiki

  - [User Guide](https://github.com/tnodir/fort/wiki/Fort-Firewall-User-Guide)
  - [Functionality overview](https://github.com/tnodir/fort/wiki/Functionality-overview)
  - [Rules](https://github.com/tnodir/fort/wiki/Rules)
  - [Frequently Asked Questions (FAQ)](https://github.com/tnodir/fort/wiki/FAQ)
  - [Building](https://github.com/tnodir/fort/wiki/Building)

## Support

  - ⭐ Star it on GitHub, ❤ Rate on [AlternativeTo](https://alternativeto.net/software/fort-firewall/about/)
  - 🐞 [Report bugs](https://github.com/tnodir/fort/issues), ⚡ [Discuss ideas](https://github.com/tnodir/fort/discussions)

### Sponsoring

[![GitHub](https://img.shields.io/badge/GitHub%20Sponsors-blue?logo=GitHub%20Sponsors&style=social)](https://github.com/sponsors/tnodir)
[![Patreon](https://img.shields.io/badge/Patreon%20Patrons-blue?logo=Patreon&style=social)](https://www.patreon.com/bePatron?u=9349286)
[![BuyMeACoffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-blue?logo=BuyMeACoffee&style=social)](https://buymeacoffee.com/tnodir)

## SAST Tools

[PVS-Studio](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

## Attribution

  - [FatCow Free Icons](http://www.fatcow.com/free-icons)

--
Nodir Temirkhodjaev, <nodir.temir@gmail.com>
