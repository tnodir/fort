# Fort Firewall

[![Release](https://img.shields.io/github/release/tnodir/fort.svg)](https://github.com/tnodir/fort/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/tnodir/fort/total.svg?maxAge=86400)](https://github.com/tnodir/fort/releases)
[![Screenshots](https://img.shields.io/static/v1.svg?label=screenshots&message=11&color=yellow&logo=imgur)](https://imgur.com/a/fZbAbfy)
[![License](https://img.shields.io/github/license/tnodir/fort.svg?logo=gnu)](https://www.gnu.org/licenses/gpl-3.0)
[![Crowdin](https://badges.crowdin.net/fort-firewall/localized.svg)](https://crowdin.com/project/fort-firewall)

Fort is a simple firewall for Windows 7+.

## Features

  - Filter by network addresses, application groups
  - Support wildcards in program path names
  - Application group speed limits
  - Stores traffic statistics
  - Graphical display of bandwidth
  - Has own kernel driver, based on Windows Filtering Platform (WFP)

## Supported OS versions

 Asset                  | OS      | Version           | Architectures | Description
------------------------|---------|-------------------|---------------|---------------------------------------
 \*-windows-x86.\*      | Windows | 7 SP1 and later   | x86, x64      | 32/64-bit Windows 7, 8, 8.1, 10, 11
 \*-windows10-x86_64.\* | Windows | 10 2004 and later | x64           | 64-bit Windows 10 2004-21H2, 11

-----

## FAQ

### Windows 7 SP1 64-bit fails to install not digitally signed drivers

See "[SHA-2 Code Signing Support for Windows 7](https://docs.microsoft.com/en-us/security-updates/SecurityAdvisories/2015/3033929)".

### What the difference is between "Internet Addresses" and "Allowed Internet Addresses" on the IPV4 Addresses tab?

1. All FW rules act on "Internet Addresses" only.
LAN addresses are immediately allowed by FW and not checked by app groups or speed limiter.

For example here you can describe Internet addresses as:
- "Include All" addresses,
- but exclude 127.0.0.0/8, 192.168.0.0/16.

2. "Allowed Internet Addresses" may be used for example:
- to block only some addresses:
    - "Include All" addresses,
    - but exclude facebook.com: "31.13.72.36".
- to allow only some addresses:
    - "Exclude All" addresses,
    - but include wikipedia.com: "91.198.174.192".

Filtered apps will be shown in the "Blocked" tab. 

### Do the App rules take precedence over the Allowed Internet rules?

#### Filtering steps:
1) If **address** is 127.* or 255.255.255.255 and "Filter Local Addresses" is turned off, then _**PERMIT**_
2) If "Filter Enabled" is turned off, then _**PERMIT**_
3) If "Stop Traffic" is turned on, then _**BLOCK**_
4) If **address** is not from "Internet Addresses", then _**PERMIT**_
5) If "Stop Internet Traffic" is turned on, then _**BLOCK**_
6) If **address** is not from "Allowed Internet Addresses", then _**BLOCK**_
7) If **app path** is allowed, then _**PERMIT**_
8) Log about blocked event and _**BLOCK**_

## Code Quality Monitor

[![Code quality status](https://codescene.io/projects/5344/status.svg)](https://codescene.io/projects/5344/jobs/latest-successful/results)

--
Nodir Temirkhodjaev, <nodir.temir@gmail.com>
