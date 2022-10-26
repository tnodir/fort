# Fort Firewall

[![Release](https://img.shields.io/github/release/tnodir/fort.svg)](https://github.com/tnodir/fort/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/tnodir/fort/total.svg?maxAge=86400)](https://github.com/tnodir/fort/releases)
[![Screenshots](https://img.shields.io/static/v1.svg?label=screenshots&message=11&color=yellow&logo=imgur)](https://imgur.com/a/fZbAbfy)
[![License](https://img.shields.io/static/v1.svg?label=license&message=GPL-v3&color=orange&logo=gnu)](https://www.gnu.org/licenses/gpl-3.0)
[![Crowdin](https://badges.crowdin.net/fort-firewall/localized.svg)](https://crowdin.com/project/fort-firewall)
[![Canny](https://img.shields.io/static/v1.svg?label=canny&message=Vote%20for%20features!&color=blue&logo=canny)](https://fort-firewall.canny.io/feature-requests)

Fort is a simple firewall for Windows 7+.

## Features

  - Filter by network addresses, application groups
  - Filter by `SvcHost.exe` service names
  - Support wildcards in program path names
  - Application group speed limits
  - Blocklists via "Zones"
  - Stores traffic statistics
  - Graphical display of bandwidth
  - Has own kernel driver, based on Windows Filtering Platform (WFP)

## Supported OS versions

 Asset                  | OS      | Version           | Architectures | Description
------------------------|---------|-------------------|---------------|---------------------------------------
 \*-windows-x86.\*      | Windows | 7 SP1 and later   | x86, x64      | 32/64-bit Windows 7, 8, 8.1, 10, 11
 \*-windows-x86.\*      | Windows | 10 and later      | ARM64         | ARM64 Windows 10, 11
 \*-windows10-x86_64.\* | Windows | 10 2004 and later | x64           | 64-bit Windows 10 2004-22H2, 11

-----

## Requirements

  - Disabled [HVCI (Core Isolation)](https://github.com/tnodir/fort/discussions/40)
  - [Visual C++ x86 (32-bit!) redistributable](https://aka.ms/vs/17/release/vc_redist.x86.exe)

## Wiki

  - [Frequently Asked Questions (FAQ)](https://github.com/tnodir/fort/wiki/FAQ)
  - [Building](https://github.com/tnodir/fort/wiki/Building)

## Code Quality Monitor

[![Code quality status](https://codescene.io/projects/5344/status.svg)](https://codescene.io/projects/5344/jobs/latest-successful/results)

## SAST Tools

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=github&utm_medium=organic&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

--
Nodir Temirkhodjaev, <nodir.temir@gmail.com>
