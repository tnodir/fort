# Fort Firewall

[![Release](https://img.shields.io/github/release/tnodir/fort.svg)](https://github.com/tnodir/fort/releases/latest) [![Downloads](https://img.shields.io/github/downloads/tnodir/fort/total.svg?maxAge=86400)](https://github.com/tnodir/fort/releases) [![Screenshots](https://img.shields.io/static/v1.svg?label=screenshots&message=11&color=gray)](https://imgur.com/a/fZbAbfy) [![License](https://img.shields.io/github/license/tnodir/fort.svg)](https://www.gnu.org/licenses/gpl-3.0)

Fort is a simple firewall for Windows 7+.

## Features

  - Filter by network addresses, application groups
  - Application group speed limits
  - Stores hourly, daily, monthly and yearly statistics
  - Graphical display of bandwidth
  - No alert popups
  - Based on Windows Filtering Platform (WFP)

## Supported OS versions

OS              | Version                       | Architectures
----------------|-------------------------------|--------------
Windows         | 7 SP1+, 8.1, 10               | x86, x64

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
2) If "Stop Traffic" is turned on, then _**BLOCK**_
3) If "Filter Enabled" is turned off or **address** is not from "Internet Addresses", then _**PERMIT**_
4) If "Stop Internet Traffic" is turned on, then _**BLOCK**_
5) If **address** is not from "Allowed Internet Addresses", then _**goto 7)**_
6) If **app path** is allowed, then _**PERMIT**_
7) If "Show Blocked Applications" is turned on, then log about blocked event and _**BLOCK**_

--
Nodir Temirkhodjaev, <nodir.temir@gmail.com>
