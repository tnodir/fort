# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Fort Firewall — a Windows firewall consisting of a Qt 6 (C++20, widgets) user-mode application and its own WFP kernel driver (`fortfw.sys`, C). Windows 7 and later; targets are `win7` (x86/x64), `win10` (x64) and `win10-arm64`.

Sources live under `src/`. Build trees (`build-*/`), binaries and `*.qm` files are git-ignored.

## Build

Everything is qmake + MSVC; builds are out-of-source, one directory per target. Run from a Visual Studio command prompt.

```bat
:: Configure once per build dir
cd build-win10
<qt-static>\bin\qmake.exe -o Makefile ..\src\FortFirewall.pro -spec win32-msvc ^
    "CONFIG-=qml_debug" "CONFIG+=driver_payload" "CONFIG+=tests"
:: Build
jom      :: or nmake
```

- Qt is a **custom static build** (`<qt-static>` is the `-prefix` of the scripts below) — see `deploy/qt-build.bat` (win10 x64), `deploy/qt-build-win7.bat`, `deploy/qt-build-arm64.bat`. Many Qt features are disabled (`-no-feature-sql`, `-no-feature-printsupport`, no OpenGL/dbus/icu…), so do not use Qt SQL, QPrinter, QDial, QUndoStack, QMimeDatabase, QDockWidget, etc. Only `qtbase` is built: no Qt Quick/QML (scene graph), Charts or other modules — draw with QPainter or Graphics View (`QGraphicsView`/`QGraphicsScene`, enabled). It is configured `-release` only, and the release builds (as well as the `msvcbuild-*.bat` driver builds) define `NDEBUG`, so `assert()` isn't compiled at all and a typo inside one goes unnoticed. For a debug build (asserts on) use a regular Qt install (`QT_HOME`, which has the debug libs) in a separate build dir: `%QT_HOME%\bin\qmake.exe -o Makefile ..\src\FortFirewall.pro -spec win32-msvc "CONFIG+=debug" "CONFIG-=release" "CONFIG+=tests"` + `jom`; run its binaries with `%QT_HOME%\bin` in `PATH`.
- `CONFIG+=tests` adds the `driver` (user-mode driver test app) and `tests` subprojects; `CONFIG+=driver_payload` adds `driver_payload`. Without them only `ui` + `ui_bin` are built.
- Subproject layout: `ui/` builds the static lib `FortFirewallUILib`, `ui_bin/` links it into `FortFirewall.exe`, tests link the same lib. **New .cpp/.h files must be added explicitly to `SOURCES`/`HEADERS` in `src/ui/FortFirewallUI.pro`.**

### Kernel driver

Built with MSBuild + WDK, not qmake:

```bat
src\driver\msvcbuild-win10-64.bat        :: -> build-driver-win10\x64\fortfw.sys
src\driver\msvcbuild-win7-32.bat         :: msvcbuild.bat <PLAT> <CONFIG> <ARCH>
src\driver\loader\msvcbuild-win10-64.bat :: driver loader (fortfwdl.sys payload wrapper)
```

With an EWDK instead of an installed WDK, set up the environment non-interactively (`LaunchBuildEnv.cmd` opens a `cmd /k`): `cmd /c "<EWDK>\BuildEnv\SetupBuildEnv.cmd amd64 && src\driver\msvcbuild-win10-64.bat"`. The `win7` targets need an older EWDK (WDK 10.0.20348 builds them; the 26100 one is for `win10`/arm64).

`fortdrv.vcxproj` compiles **only `fortdrv_amalg.c`** (an amalgamation that `#include`s every driver .c). A new driver source file must be added there *and* to `FortFirewallDriver.pro`. In the amalgamation `FORT_API` is `static` (`driver/common/common.h`), so a function called from another driver .c needs `FORT_API` and a prototype in its header; the user-mode build below compiles each .c separately (`FORT_API` = `extern`) and catches a missing one.

`src/driver/FortFirewallDriver.pro` builds the driver code as a **user-mode** console app (`driver/test/main.c` plus the `wdm/um_*.c` shims that emulate the kernel APIs) — that is how driver logic is exercised without loading a real driver.

The driver writes its errors to the System event log by `TRACE(event_code, status, error_value, sequence)` (`forttrace.c`; the Config info events only with the "Trace Driver Events" option). The event codes are defined in `driver/evt/fortevt.mc` (a facility's MessageIds start at its number × 10): after a change regenerate `fortevt.h` and `FORTEVT_MSG00001.bin` by `mc -z FORTEVT fortevt.mc` in `driver/evt/` (`mcbuild.bat`) and convert the header's CRLF to LF. `driver/common/` is compiled into the UI too, so `TRACE` there is guarded by `FORT_DRIVER` (cf. `fortprov.c`). A user's events: `wevtutil qe System "/q:*[System[Provider[@Name='fortfw']]]" /f:xml /rd:true /c:50`; in an event's `<Binary>` (the `IO_ERROR_LOG_PACKET` header, little-endian) the 4th DWORD is the event code, the 5th `error_value`, the 6th `status` (e.g. `0D0000C0` = `STATUS_INVALID_PARAMETER`).

### Tests

GoogleTest/GoogleMock; requires the `GOOGLETEST_DIR` env var pointing at a googletest checkout, otherwise the test subprojects are skipped silently.

```bat
build-win10\tests\UtilTest\UtilTest.exe
build-win10\tests\UtilTest\UtilTest.exe --gtest_filter=ConfUtilTest.*   :: single test/suite
```

Suites: `UtilTest` (bitutil, confutil, connfilter, dateutil, fileutil, filterline, formatutil, ioccontainer, netutil, ruletextparser, stringutil, timeperiod, wildmatch), `StatTest`, `LogBufferTest`, `LogReaderTest`. `LogReaderTest` needs the loaded kernel driver: run it only manually from a console, never as part of an automated test run. Each `tst_*.h` is included from the suite's `tst_main.cpp` and must also be listed in the suite's `.pro`. Run a test executable with its build folder as the current directory: `UtilTest` writes files (`zones/`) relative to it.

### Testing the real driver (test-mode VM)

- The machine needs `bcdedit /set testsigning on` and Secure Boot off. A self-signed code-signing certificate is enough (`signtool sign /fd sha256 /f test.pfx fortfw.sys`); test mode doesn't need it imported.
- Install the plain `fortfw.sys`, bypassing the loader/payload, with **demand start**, so a reboot after a crash comes up without it: copy it to `%SystemRoot%\System32\drivers` and `sc create fortfw binPath= %SystemRoot%\System32\drivers\fortfw.sys type= kernel start= demand group= NetworkProvider depend= BFE`, then `sc start fortfw`. (`driver/scripts/install.bat` uses `start= auto`.)
- `FortFirewall.exe -i service` installs and starts `FortFirewallSvc` (depends on `fortfw`; `sc config FortFirewallSvc start= demand` to keep reboots clean). With a service the app doesn't run `check-reinstall.bat`, so it won't reinstall the driver.
- LAN addresses aren't filtered by default and the first minute after install is the learn mode, so remote access (SSH/RDP over LAN) survives.
- Smoke test via the control CLI: `FortFirewall.exe -c filter report`, `-c filter-mode report`, `-c prog allow|block|report <app-path>`. A `report` returns `70 + value index` as the exit code (e.g. filter-mode `learn` = 70, `block` = 72; prog `allow` = 70, `block` = 71), `99` = error. A renamed copy of `curl.exe` works as an unknown program.

### Translations & deployment

```bat
set QT_HOME=<Qt>\<version>\msvc2022_64    :: a regular Qt install with lupdate/lrelease
src\scripts\i18n\update_ts.bat     :: lupdate all src/ui/i18n/*.ts
src\scripts\i18n\release_ts.bat    :: lrelease -> *.qm (merges i18n/qt/qtbase_*.ts)

deploy\setup-deployment-win10.bat  :: stage files per deploy/deployment.json into deploy/build
deploy\create-installer.bat        :: Inno Setup (deploy/FortFirewall.iss)
src\scripts\driver\driver-prepare.bat  :: clear certs, build driver payloads
```

## Architecture

### Process roles

One executable, several roles selected by command line / settings (`FortSettings`, `src/ui_bin/main.cpp`):

- **Service** (`isService()`) — runs as a Windows service, owns the driver and the databases, no GUI (`WindowManagerFake`).
- **UI client** — the tray/GUI process. When a service is present (`hasService()`) it is *not* master.
- **Control client** (`hasControlCommand()`) — a short-lived process that forwards a CLI command to the running instance and exits.

`isMaster() == !hasService() || isService()` decides which implementations get registered.

### IoC container + RPC mirroring

`IocContainer` (`src/ui/util/ioc/ioccontainer.h`) is a thread-pinned, type-id-indexed service locator (max 32 services). Services derive from `IocService` and get `setUp()`/`tearDown()` lifecycle. Typed accessors live in `Fort::` namespace helpers (`src/ui/fortglobal.h`, e.g. `Fort::confManager()`, `IoC<T>()`).

`setupServices()` in `src/ui/fortmanager.cpp` is the key place to read: for a **master** it registers the real managers (`ConfManager`, `StatManager`, `DriverManager`, …); for a **client** it registers `*Rpc` subclasses from `src/ui/rpc/` that implement the same interface by marshalling calls over IPC to the service. **Any new manager method that a client can invoke needs a matching override in its `…Rpc` class and a `Control::Command` entry.**

IPC is `QLocalServer`/`QLocalSocket` (`src/ui/control/`): `ControlManager` listens (world-accessible when running as service), `ControlWorker` frames requests, `RpcManager` dispatches them, `src/ui/control/command/controlcommand*.cpp` implement the user-facing CLI commands. `ControlManager::connectToAnyServer()` tries the UI process first, so with a service and a running tray a `-c` command runs in the UI **client** (the `*Rpc` managers, a read-only conf DB): a manager method that a `ControlCommand*` calls to change state must have its `…Rpc` override; test the control commands with the tray running too.

### Configuration pipeline (UI → driver)

`FirewallConf` / `App` / `Group` / `SpeedLimit` / `TimePeriod` / `Rule` / `Zone` (`src/ui/conf/`) are the in-memory model, persisted to SQLite by `ConfManager` and the specialized `ConfAppManager`, `ConfRuleManager`, `ConfZoneManager`, `ConfGroupManager`, `ConfSpeedLimitManager`, `ConfTimePeriodManager`. `src/ui/util/conf/confbuffer.cpp` + `confutil.cpp` serialize that model into the **packed binary layout defined in `src/driver/common/fortconf.h`**, which `DriverManager`/`DriverWorker` push to the driver via the `FORT_IOCTL_*` codes in `src/driver/common/fortioctl.h` (`SETCONF`, `ADDAPP`, `SETRULES`, `SETZONES`, `SETGROUPS`, `SETSPEEDLIMITS`, `GETLOG`, …).

`src/driver/common/` is compiled into **both** the UI and the driver (via `src/driver/Driver.pri`). It is the shared contract: changing the `fortconf.h` layout, the IOCTL set, or the log record format requires updating both sides and bumping `DRIVER_VERSION` in `src/version/fort_version.h` (checked in `driver/fortdev.c` against the value written by `confbuffer.cpp`).

Programs belong to `Group`s (`conf/group.h`, max 32: enabled, exclusive, an optional Time Period and an optional Rule) by the `app.groups_mask` bit mask, and reference one-direction `SpeedLimit`s (`conf/speedlimit.h`, max 32) by `app.in_limit_id` / `app.out_limit_id`. They replace the old App. Groups, which `ConfManager`'s DB migration converts.

A Program's Network Filters are rule text lines in `app.filters_text` (one filter per line), edited by `FilterEditDialog` (`form/rule/`; the Edit Rule dialog uses it too, to append a line to the Rule's text). Its fields build the read-only line by `FilterLineText` (`util/conf/filterlinetext.h`: named sections, e.g. `IP(1.1.1.1):Port(80):Act(Block)`; also by a connection's `Conn`, for the Connections tab's "Add Filter" and "Copy as Filter"), the dialog gets one; `FilterLine` (`util/conf/filterline.h`) holds it and parses it by its `RuleTextParser`, giving the `RuleFilter`s by type to fill the fields. The Program's Terminating Rule (`TerminatingRuleSelector`, as in the Edit Rule dialog; Allow, Block or Drop, with an alert) is the last line, after the `# Terminating` comment line, e.g. `Act(Block):Opt(Alert)` (`ConfUtil::parseAppFiltersText()` / `appFiltersText()` give the text's `Rule`: the filters' lines and the Terminating flags). The driver gets them as a `FORT_CONF_RULE` (without zones, set and Time Period) after the path in the app entry (`app_entry.rule_size`, so ADDAPP updates them) and checks them after the Program's Rule (reason `FORT_CONN_REASON_PROGRAM_FILTER`); the rule is checked without the conf's lock (`conn->app.rule`), so an exe entry with a rule is never changed in place nor freed before the conf (`fort_conf_ref_exe_free_entry()`).

Groups, Speed Limits and Rules may refer to a `TimePeriod` (`conf/timeperiod.h`, max 64: a list of intervals, each with its week days and time from/to) by `period_id`, applied while their `period_enabled` is set. `ConfTimePeriodManager` tracks the Time Periods' activity by minutes. For the Groups and Speed Limits it is folded into their enabled masks written to the driver (`writeGroupFlags()` / `writeSpeedLimitFlags()`). The Rules are checked by the driver: a rule's `period_id` (0 - always active) against the active Time Periods' mask, sent in `FORT_CONF_IO.periods` with `SETCONF` and updated by `SETPERIODS` on the activity's change.

The Global Before/After Apps rules are sent to the driver as two synthetic rules (`ConfRuleManager::walkRules()`) with the ids after the max rule id (up to `FORT_CONF_RULE_ID_MAX`): don't reuse the deleted rules' free ids for them.

### Databases

No Qt SQL — a hand-rolled SQLite wrapper in `src/ui/3rdparty/sqlite/` (`SqliteDb`, `SqliteStmt`, `DbQuery`) over the amalgamation in `src/3rdparty/sqlite/`. Separate DBs for conf, traffic stats, connections and the app-info cache; schema versions are applied from `.sql` migrations embedded as Qt resources (`ui/conf/migrations/`, `ui/stat/migrations/*/`, `ui/appinfo/migrations/`).

### Driver internals (`src/driver/`)

`fortdrv.c` entry point; `fortcout.c` WFP callouts registration, `fortcout_ale.c` ALE and `fortcout_pkt.c` packet classify callouts; `common/fortconf_conn.c` the ALE connection's decision (local addresses, flags, addresses, Filter Mode, App, Zones, Rules, Groups), shared with the UI (`DriverCommon::confConnFilter()`, used by the Filter Simulator: `ConfManager::simulateConn()` writes the conf buffers as for the driver, a client asks the service by RPC) and accessing the conf via callbacks, so the driver keeps its locks; `fortcnf*.c` the live configuration (conf/rules/groups/zones) with reader-writer locks; `fortbuf.c` the log ring buffer read back by the UI; `fortpkt.c` packets' cloning and re-injection, `fortpkt_shaper.c` the Speed Limits' shaper queues, `fortpkt_pending.c` the pended (Ask to Connect) packets; `fortstat.c` traffic accounting; `fortps.c` process tracking; `fortpool.c`/`forttlsf.c` allocators (TLSF from `src/3rdparty/tlsf`); `fortmod.c` + `loader/` the self-loading module (fortfwdl.sys unpacks the signed payload); `proxycb/` callout trampolines (with .asm variants per arch).

The traffic is counted by the transport callouts (`fortcout_pkt.c`) in Fort's sublayer (the max weight, so before the other drivers' callouts): a packet re-injected by another driver (`FWPS_PACKET_INJECTED_BY_OTHER`, e.g. by NetLimiter's shaper) is skipped as its original packet is counted (unless the "Collect Traffic re-injected by other drivers" option, `log_stat_reinjected`, is set: e.g. the inbound packets diverted by WinDivert at the IP packet layer don't reach the transport layer), a packet blocked by a higher sublayer (BLOCK without the write right) too. 3.20.0–3.20.2 counted it in `FWPM_SUBLAYER_INSPECTION` (the lowest weight, after the other callouts), which missed the traffic of a driver absorbing the packets and re-injecting them elsewhere (#768): their filters are only deleted now. The WFP sublayers' weights seem to be made unique by BFE (a requested weight 0 became 10: 0…8 are built-in, a third-party one took 9); only `FWP_ACTION_CALLOUT_INSPECTION` filters without `FWPM_FILTER_FLAG_PERMIT_IF_CALLOUT_UNREGISTERED` are allowed in `FWPM_SUBLAYER_INSPECTION` (else `STATUS_FWP_INVALID_FLAGS`).

### UI layer

`form/` uses a window + controller pair per feature area (`programswindow.cpp` / `programscontroller.cpp`, and likewise for rules, zones, groups, services, traffic, connections, opt, home), with `basecontroller.cpp` resolving IoC dependencies. Table content comes from `model/` (`TableSqlModel`/`TableItemModel` subclasses querying SQLite directly). Background work goes through `util/worker/` (`WorkerManager` + `WorkerObject` + `WorkerJob`) — used by `appinfo/`, `hostinfo/`, `task/` and `stat/`.

`WindowManager` holds one window per `WindowCode` (`FormPointer`). Other windows are created by their owner with `Qt::WA_DeleteOnClose`: a dialog's child window (e.g. its Program Connections window) stays above it and is closed with it explicitly (the dialog may be only hidden and reused), an independent one (the Programs window's Program Connections windows) closes on `WindowManager::aboutToCloseAllWindows()` (on quit and on the settings' import). `FormPointer` calls a window's `saveWindowState()` on its closing; such a window, if it keeps its state, calls it on its `aboutToClose` itself (cf. `AppAlertConnsWindow`).

The traffic graph (`form/graph/`) has no 3rd-party chart library: `GraphWindow` (the window, its flags and options) feeds `StatManager::trafficAdded` into `GraphPlot`, a `QGraphicsView` whose scene items (grid, axes, tick labels, in/out bars, text speed) are laid out in the viewport's pixels. It keeps the per-second points itself (a delayed second is merged, not cleared) and is replotted once per second by `GraphWindow`'s timer, not while hidden; the axes are re-laid out only when the value range, the size or the tick labels change. Between the replots the bars scroll by a device pixel (a second is the "Bar width" option's pixels (5 by default) rounded to the device pixels, the last second at the right edge; the bars are laid out in device pixels too, widths and heights, so they don't jump or gap with a fractional scale like 125%; a second's in and out bars overlap, the smaller one in front, their sum above in the "Total" color; the "Show" options hide the Total, Download and Upload graphs (the Total by default: a hidden one's bars are empty, its line is hidden; the Total line's area is filled by a gradient, without it the Download and Upload ones are, by fill-only `GraphLineItem`s below both lines) and the value range is by the visible ones); the last complete second's bars rise and the value range's change is animated (`GraphAnimation`, ~30 frames per second, not `QVariantAnimation`'s ~60), so the view keeps the default minimal viewport update. The bars are drawn by `GraphBarsItem` as rectangles (`QPainter::drawRects()`): filling a `QPainterPath` of them is ~60× slower. The "Graph type" option (`IniUser::graphWindowGraphType()`) switches the bars to the Line type: `GraphLineItem`'s monotone cubic curves of 2 device pixels, drawn as antialiased cosmetic lines of a device pixel with offsets (a thick antialiased line is ~10× slower); the lines have no rising points. The bars and the lines are `GraphCachedItem`s: painted to a pixmap aligned to the device pixels on their change, the scrolled ones only draw it (`QGraphicsItem::DeviceCoordinateCache` is slower: it's regenerated on the scroll). Without traffic nothing is scrolled, animated or repainted. The scroll and the animations are switched off by the graph's "Animation" option (`IniUser::graphWindowAnimation()`, `GraphPlot::setAnimated()`): then the graph is repainted once per second.

## Conventions

- Format with `src/_clang-format` (WebKit-based Qt style, 100 columns, `PointerBindsToType: false`, braces on their own line after functions/classes only).
- Keep the UI compatible with Qt 6.1: guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(…)` (e.g. no `QFlags::toInt()`).
- Keep each function's cyclomatic complexity below 9: CodeScene reports a "Complex Method" otherwise. Every `case`/`default` label, `if`, loop, `continue`, `&&`, `||` and `?:` adds one (`break` doesn't); split long `switch`es into helpers or a function table indexed by the enum (e.g. `fort_conf_rule_filter_check_funcList` in `driver/common/fortconf.c`). Likewise keep at most 4 arguments per function ("Excess Number of Function Arguments"), grouping them into a struct if needed (cf. `FORT_CALLOUT_ARG`), and at most one `&&`/`||` in an `if`/`while` condition ("Complex Conditional"): move the rest into a named helper or early returns. Nest a conditional or loop in at most one of a function's top-level branches/loop bodies ("Bumpy Road"); move the others into helpers.
- Build on the existing code instead of duplicating it: reuse or split an existing function rather than adding a near-copy, and work with the existing parsers' objects (e.g. `RuleTextParser`'s `RuleFilter`s for a rule text, the `ValueRange`s like `ActionRange`/`OptionRange` for a filter's values and their `FORT_RULE_FILTER_*`/`FORT_CONN_FILTER_RESULT_*` flags) rather than adding own enums, name tables or value structs for the same data; extend the existing class a little if needed (cf. `RuleTextParser::setText()`).
- Keep a value apart from its processing: when a class mixes copyable data with a non-copyable or stateful helper (e.g. a `QObject`-based parser and its results), split the data into a light value class held by a member of the processing class (cf. `FilterLineText` in `FilterLine`). Pass the value class around by `const &` and create the processing object locally where it's needed; don't work around the mix with `mutable` members, copies of the data or rvalue-reference parameters.
- Make a window's variant a derived class rather than a mode flag inside the window (cf. `ProgramAlertWindow` of `ProgramEditDialog`, `AppConnsWindow` of `ConnectionsWindow`). The base's constructor only builds the UI and connects the controller (a protected one, when the base's public constructor finishes its own setup, as `ConnectionsWindow`'s does); the derived constructor does its own `setupUi()`, then `initialize()` and `setupFormWindow()`: virtual functions aren't dispatched to the derived class in the base's constructor. The variant overrides `windowCode()`, `restoreWindowState()`/`saveWindowState()` and the `retranslateWindowTitle()` slot; the base offers generic helpers for it (e.g. `ConnectionsWindow::hideEditing()`, `headerLayout()`) instead of knowing the variant. Likewise a model's variant overrides the base's protected virtual hooks (cf. `AppConnSearchModel` of `ConnSearchModel`: `isSearching()`, `lastConnsIdMin()`, `sqlSearchWhere()`, `fillSearchVars()`), and the window creating it passes it to its base window's protected constructor. A window's variant creates the variants of its own windows by a virtual factory (cf. `ProgramEditDialog::createConnsWindow()`: the Alert Program window's one creates `AppAlertConnsWindow`).
- Order a class's or struct's data members by ascending size: `bool`s and other 1-byte ones first (check an enum's underlying type: many are `: qint8`), then `quint16`, 32-bit ints/enums, pointers, objects (`QString`, containers, structs).
- Put the activating one of paired functions first, in the header and in the .cpp alike: start → stop, open → close, enable → disable, show → hide, setUp → tearDown.
- Wrap an `if` body in braces when it returns a computed expression (incl. a struct field); simple returns (`return;`, a constant, a plain local variable) stay without braces. The blank line after the `{` of an `if` with a multi-line condition comes from clang-format: don't add or remove it by hand.
- Declare a constant in an anonymous `namespace { }` as `inline constexpr` (Qt Creator's navigation finds it then); function-local constants stay plain `constexpr`.
- Keep a form's code in the order of its controls: the members in the header, their creation in the `setup*Layout()` functions and their lines in `initialize()`, `retranslateUi()`, `fill*()`. Moving a control on the form moves all of them.
- Implement the requested UI behavior only: ask before adding a convenience nobody asked for (switching tabs, moving the focus or selection, enabling/disabling controls by other state, guessed defaults or values derived from heuristics), even if it seems obvious; mention it as an option instead.
- Commit subjects are prefixed by area: `UI:`, `Driver:`, `Tests:`, `Deploy:`, `Installer:`, `README:` — e.g. `UI: ConfManager: Refactor save()`.
- `ChangeLog` is maintained by hand at release time; don't edit it in feature commits.
- App version lives in `src/version/fort_version.h` (`APP_VERSION_*` and `DRIVER_VERSION`).
