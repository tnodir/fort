# <img src="https://github.com/tnodir/fort/blob/master/src/ui/icons/fort.png" alt="Icon" width="32" height="32"> Fort Firewall

[![Release](https://img.shields.io/github/release/tnodir/fort.svg)](https://github.com/tnodir/fort/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/tnodir/fort/total.svg?maxAge=86400)](https://github.com/tnodir/fort/releases)
[![Screenshots](https://img.shields.io/static/v1.svg?label=screenshots&message=11&color=yellow&logo=imgur)](https://imgur.com/a/fZbAbfy)
[![License](https://img.shields.io/static/v1.svg?label=license&message=GPL-v3&color=orange&logo=gnu)](https://www.gnu.org/licenses/gpl-3.0)
[![Crowdin](https://badges.crowdin.net/fort-firewall/localized.svg)](https://crowdin.com/project/fort-firewall)
[![CodeScene Code Health](https://codescene.io/projects/5344/status-badges/code-health)](https://codescene.io/projects/5344)

Fort is an effective firewall designed for Windows 7 and later versions, offering both simplicity and robustness.

![image](https://github.com/tnodir/fort/assets/77551811/9906513e-5e58-4e40-bb69-29be40715dc8)

## Features

  - Filter by network addresses, application groups and more
  - Filter by `SvcHost.exe` service names
  - Support for wildcards in application path names
  - Speed limit application groups
  - Blocklists via "Zones"
  - Saves traffic statistics
  - Graphical display of the bandwidth
  - Has its own kernel driver based on the Windows Filtering Platform (WFP)

## Supported OS versions

 Asset                  | OS      | Version           | Architectures | Description
------------------------|---------|-------------------|---------------|---------------------------------------
 \*-windows-x86.\*      | Windows | 7 SP1 and later   | x86, x64      | 32/64-bit Windows 7, 8, 8.1, 10, 11
 \*-windows10-arm64.\*  | Windows | 10 2004 and later | ARM64         | ARM64 Windows 10 2004+, 11
 \*-windows10-x86_64.\* | Windows | 10 2004 and later | x64           | 64-bit Windows 10 2004+, 11

-----

## Requirements

  - Disabled [HVCI (Core Isolation)](https://github.com/tnodir/fort/discussions/40)
  - [Visual C++ x86 (32-bit!) redistributable package](https://aka.ms/vs/17/release/vc_redist.x86.exe)

## Wiki

  - [First Start guide](https://github.com/tnodir/fort/wiki/First-Start-guide)
  - [Functionality overview](https://github.com/tnodir/fort/wiki/Functionality-overview)
  - [Frequently Asked Questions (FAQ)](https://github.com/tnodir/fort/wiki/FAQ)
  - [Building](https://github.com/tnodir/fort/wiki/Building)

## Support

Please support the project:

  - ⭐ Star it on GitHub, ❤ Rate on [AlternativeTo](https://alternativeto.net/software/fort-firewall/about/)
  - 🐞 [Report bugs](https://github.com/tnodir/fort/issues), ⚡ [Discuss ideas](https://github.com/tnodir/fort/discussions)

### Donation

  - [Become a Sponsor!](https://github.com/sponsors/tnodir)
  - [<img src="https://github.com/edent/SuperTinyIcons/blob/master/images/svg/patreon.svg" width="18" height="18"> Become a Patron!](https://www.patreon.com/bePatron?u=9349286)

## SAST Tools

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=github&utm_medium=organic&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

## Credits

  - [FatCow Free Icons](http://www.fatcow.com/free-icons)

--
Nodir Temirkhodjaev, <nodir.temir@gmail.com>
