# AGENTS.md

This file provides guidance to AI coding agents when working with code in this repository.

## Project

Fort Firewall — a Windows firewall consisting of a Qt 6 (C++20, widgets) user-mode application and its own WFP kernel driver (`fortfw.sys`, C). Windows 7 and later; targets are `win7` (x86/x64), `win10` (x64) and `win10-arm64`.

Sources live under `src/`. Build trees (`build-*/`), binaries and `*.qm` files are git-ignored.

The details of single areas are in `docs/agents/*.md`, each named below where its area is: read one before working on its area. Put a new area's details (more than a paragraph) there too, with a pointer here: this file is read on every request.

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
- `scripts\make\` builds all the targets from a plain console, setting up the compilers' environments themselves: `ui\make-all.bat` (`build-win7`, `build-win10`, `build-win10-arm64`; one by `ui\build-<target>.bat [make args]`), `driver\make-all.bat` (the drivers by the EWDKs), `driver-loader\make-all.bat` (the loaders of the built drivers), `deploy\make-all.bat` (the installers in `deploy\out` and their GnuPG signatures `*.exe.sig`; one by `deploy\build-<target>.bat`); they stop at the first error. The machine's paths (EWDKs, VS `vcvars*.bat`, static Qts, jom, the payloads' key, Inno Setup, `gpg.exe`, the signing key and its passphrase's file) are set in `config.bat`.
- Subproject layout: `ui/` builds the static lib `FortFirewallUILib`, `ui_bin/` links it into `FortFirewall.exe`, tests link the same lib. **New .cpp/.h files must be added explicitly to `SOURCES`/`HEADERS` in `src/ui/FortFirewallUI.pro`.**
- Never run two builds of the same build dir in parallel (a shell `jom` and Qt Creator's build of it, or two shell ones): they fight over the same files.

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

The driver's errors in the System event log (`TRACE()`, `forttrace.c`, the event codes of `driver/evt/fortevt.mc` and their regeneration, `TRACE` in `driver/common/`, reading a user's events): read `docs/agents/driver.md` before changing or debugging them.

### Tests

GoogleTest/GoogleMock; requires the `GOOGLETEST_DIR` env var pointing at a googletest checkout, otherwise the test subprojects are skipped silently.

```bat
build-win10\tests\UtilTest\UtilTest.exe
build-win10\tests\UtilTest\UtilTest.exe --gtest_filter=ConfUtilTest.*   :: single test/suite
```

Suites: `UtilTest` (bitutil, confutil, connfilter, dateutil, fileutil, filterline, formatutil, ioccontainer, netutil, ruletextparser, stringutil, timeperiod, wildmatch), `StatTest`, `LogBufferTest`, `LogReaderTest`. `LogReaderTest` needs the loaded kernel driver: run it only manually from a console, never as part of an automated test run. Each `tst_*.h` is included from the suite's `tst_main.cpp` and must also be listed in the suite's `.pro`. Run a test executable with its build folder as the current directory: `UtilTest` writes files (`zones/`) relative to it.

- Never wait by the wall clock (`Sleep()`, `QThread::sleep()`, `Start-Sleep` in the VM scripts) for a result: wait for a state or an event the code under test must produce, or for the answer to a later command proving the earlier one was processed. An absence ("no error is reported") is checked after an event guaranteed to come after the moment in question; where there is none, skip the check and say why.
- Don't raise a shared timeout to make one flaky test pass: fix the test.
- An assertion that can't fail is worse than none: before claiming a test covers a fix, disable the fix and see the test fail.

### Testing the real driver (test-mode VM)

- Read `docs/agents/vm-testing.md` before testing on a machine: test signing, installing the plain `fortfw.sys` and the service with demand start, the LAN access, the control CLI's smoke test.
- Run a built `FortFirewall.exe` only on such a machine, not where Fort is installed and running: one instance per session is allowed (the `APP_BASE` mutex), so a second one only shows the running instance's window and exits, and a `-c` command controls the running instance, i.e. the installed firewall. Stop the processes you started by their PID, never by the name (`taskkill /im`, `Stop-Process -Name`): the installed Fort's processes have the same one.

### Translations & deployment

```bat
set QT_HOME=<Qt>\<version>\msvc2022_64    :: a regular Qt install with lupdate/lrelease
scripts\i18n\update_ts.bat         :: lupdate all src/ui/i18n/*.ts
scripts\i18n\release_ts.bat        :: lrelease -> *.qm (merges i18n/qt/qtbase_*.ts)

deploy\setup-deployment-win10.bat  :: stage files per deploy/deployment.json into deploy/build
deploy\create-installer.bat        :: Inno Setup 7 (deploy/FortFirewall.iss)
scripts\make\driver-loader\make-all.bat      :: clear certs, wrap build-driver-*\*\fortfw.sys into the loaders
scripts\make\deploy\make-all.bat             :: stage + installer + gpg *.exe.sig for win7, win10, win10-arm64
```

The installer (Inno Setup 7, `deploy/FortFirewall.iss`, `create-installer.bat`: the architectures, the file names' suffixes, the 64-bit install mode, the auto-update's parameters, `SuppressibleMsgBox()`, the `.isl` files' code pages): read `docs/agents/installer.md` before changing it.

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

The IP Addresses tab's two `AddressGroup`s (`FirewallConf::addressGroups()`, the LAN and Internet lists, `AddressesArea`), a Program's Network Filters (`app.filters_text`, `FilterEditDialog`, `FilterLine`, `FilterLineText`, the Terminating Rule's line) and the Time Periods (`period_id`, `ConfTimePeriodManager`, `SETPERIODS`): read `docs/agents/conf-model.md` before changing them.

Programs belong to `Group`s (`conf/group.h`, max 32: enabled, exclusive, an optional Time Period and an optional Rule) by the `app.groups_mask` bit mask, and reference one-direction `SpeedLimit`s (`conf/speedlimit.h`, max 32) by `app.in_limit_id` / `app.out_limit_id`. They replace the old App. Groups, which `ConfManager`'s DB migration converts.

Ask to Connect pends a blocked connection till the user's decision (`fortpkt_pending.c`, `PendingConnManager`, the Paused Connections and Alert Program windows, `BLOCKPENDING`/`REAUTHPENDING`/`ALLOWPENDING`/`ALLOWPROC`, Allow Process): read `docs/agents/ask-to-connect.md` before changing it.

The Global Before/After Apps rules are sent to the driver as two synthetic rules (`ConfRuleManager::walkRules()`) with the ids after the max rule id (up to `FORT_CONF_RULE_ID_MAX`): don't reuse the deleted rules' free ids for them.

A Program's or Group's Rule is an App Rule or a Preset Rule (`RulesWindow` selects by a mask of `Rule::ruleTypeBit()`; the driver doesn't know the rule types). The default Preset Rules (`ui/conf/rule/presets.json`, `PresetRulesManager`, `rule.preset_code`, the file's `version` and `since`): read `docs/agents/preset-rules.md` before changing them.

### Databases

No Qt SQL — a hand-rolled SQLite wrapper in `src/ui/3rdparty/sqlite/` (`SqliteDb`, `SqliteStmt`, `DbQuery`) over the amalgamation in `src/3rdparty/sqlite/`. Separate DBs for conf, traffic stats, connections and the app-info cache; schema versions are applied from `.sql` migrations embedded as Qt resources (`ui/conf/migrations/`, `ui/stat/migrations/*/`, `ui/appinfo/migrations/`).

### Driver internals (`src/driver/`)

`fortdrv.c` entry point; `fortcout.c` WFP callouts registration, `fortcout_ale.c` ALE and `fortcout_pkt.c` packet classify callouts; `common/fortconf_conn.c` the ALE connection's decision (local addresses, flags, addresses, Filter Mode, App, Zones, Rules, Groups), shared with the UI (`DriverCommon::confConnFilter()`, used by the Filter Simulator: `ConfManager::simulateConn()` writes the conf buffers as for the driver, a client asks the service by RPC) and accessing the conf via callbacks, so the driver keeps its locks; `fortcnf*.c` the live configuration (conf/rules/groups/zones) with reader-writer locks; `fortbuf.c` the log ring buffer read back by the UI; `fortpkt.c` packets' cloning and re-injection, `fortpkt_shaper.c` the Speed Limits' shaper queues, `fortpkt_pending.c` the pended (Ask to Connect) packets; `fortstat.c` traffic accounting; `fortps.c` process tracking, `fortpsname.c` the processes' names, interned with their references (the equal names are the same one); `fortpool.c`/`forttlsf.c` allocators (TLSF from `src/3rdparty/tlsf`); `fortmod.c` + `loader/` the self-loading module (fortfwdl.sys unpacks the signed payload); `proxycb/` callout trampolines (with .asm variants per arch).

The ALE callout decides a connection by `fort_callout_ale_check_conf()`: `fort_conf_conn_flags_filtered()` (Filter Enabled, Block All/Internet/LAN, the LAN and Internet address groups) returns TRUE when it decides the connection alone, then the App isn't checked: no flow context (no traffic, no Speed Limits) and no new App logged. Else `fort_callout_ale_check_app()` finds the App's data and `fort_conf_conn_app_allowed()` decides (an `act.blocked` already unset is allowed at once: the Filter Disabled with the traffic collected, an allowed Internet address); an allowed connection gets its flow context, a blocked one may be asked (`fort_callout_ale_ask()`), an unknown App is logged as new (`fort_callout_ale_log_app_path()`). Then the connection is logged by the conf's and the App's log flags (never with `FORT_CONN_REASON_UNKNOWN`) and classified. The UI's `DriverCommon::confConnFilter()` runs the same checks without the flows and the logging, so its result doesn't show whether the flags or the App decided: a test of that calls `fort_conf_conn_flags_filtered()` (`ConnFilterTest::connFlagsFiltered()`). A Zones callback (`fort_devconf_zones_ip_included()`) takes the conf's spin lock on each call: call it only with a zones mask (cf. `fort_conf_conn_zone_filtered()`).

Traffic accounting and the WFP provider (`log_stat` and the flows' contexts, Fort's sublayer and the packets re-injected by other drivers, the reauth transaction and `FORT_DEVICE_PROV_MASK`, `fort_prov_reregister()` and the Windows Insider builds' bug, the sublayers' weights): read `docs/agents/driver.md` before changing them.

### UI layer

`form/` uses a window + controller pair per feature area (`programswindow.cpp` / `programscontroller.cpp`, and likewise for rules, zones, groups, services, traffic, connections, opt, home), with `basecontroller.cpp` resolving IoC dependencies. Table content comes from `model/` (`TableSqlModel`/`TableItemModel` subclasses querying SQLite directly). Background work goes through `util/worker/` (`WorkerManager` + `WorkerObject` + `WorkerJob`) — used by `appinfo/`, `hostinfo/`, `task/` and `stat/`.

`WindowManager` holds one window per `WindowCode` (`FormPointer`). Its confirmation boxes (`showConfirmBox()`, `showQuestionBox()`, `showChoiceBox()`) are application-modal and call back by the context object passed, the caller: the box is closed with it, without calling back. Other windows are created by their owner with `Qt::WA_DeleteOnClose`: a dialog's child window (e.g. its Program Connections window) stays above it and is closed with it explicitly (the dialog may be only hidden and reused), an independent one (the Programs window's Program Connections windows) closes on `WindowManager::aboutToCloseAllWindows()` (on quit and on the settings' import). `FormPointer` calls a window's `saveWindowState()` on its closing; such a window, if it keeps its state, calls it on its `aboutToClose` itself (cf. `AppAlertConnsWindow`).

The traffic graph (`form/graph/`: `GraphWindow`, `GraphPlot` on a `QGraphicsView`, no chart library; the bars, lines, scrolling and animations in device pixels): read `docs/agents/traffic-graph.md` before changing it.

## Conventions

- Format with `src/_clang-format` (WebKit-based Qt style, 100 columns, `PointerBindsToType: false`, braces on their own line after functions/classes only).
- Keep the UI compatible with Qt 6.1: guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(…)` (e.g. no `QFlags::toInt()`).
- Keep the code clean of CodeScene's Code Health issues. The thresholds, as CodeScene measures them:
  - "Complex Method": cyclomatic complexity 9+. Every `case`/`default` label, `if`, loop, `continue`, `&&`, `||` and `?:` adds one (`break` doesn't); split long `switch`es into helpers or a function table indexed by the enum (e.g. `fort_conf_rule_filter_check_funcList` in `driver/common/fortconf.c`).
  - "Excess Number of Function Arguments": 5+ arguments; group them into a struct (cf. `FORT_CALLOUT_ARG`). The WFP callouts' fixed signatures are exempt.
  - "Complex Conditional": 2+ `&&`/`||` in one `if`/`while` condition; move the rest into a named helper or early returns.
  - "Bumpy Road": 2+ bumps, a bump being a body (an `if`/`else` branch, a loop's or `switch`'s body) of a function's top-level statement which contains another conditional or loop; move the nested ones into helpers.
  - "Duplicated Function Blocks" / "High Degree of Code Duplication": a pair of a file's functions of 10+ lines of code (non-empty, non-comment lines, the signature included) of the same structure: their token sequences match by ~96%+ with the names and numbers generalized, and they share 40%+ of their identifiers (e.g. a handler per IOCTL checking the input's size and calling its setter, a function per group of filters filling them field by field, a reader per log entry type, the two `switch*ConnsWindow()` of `ProgramEditDialog`). Lists of different calls (e.g. `FwpmFilterDeleteByKey0()` of different GUIDs) share too few identifiers to count. Keep the per-item data in a table and process it by one function (cf. `fortDeviceControlProcess_list`'s `in_len` in `driver/fortdev.c`, the `FORT_PROV_GROUP_ARGS` in `driver/common/fortprov.c`, `g_calloutArgs` in `driver/fortcout.c`; a `static const` table refers to the GUIDs by `const GUID *`), or move the common steps into helpers until each function is under 10 lines (cf. `LogBuffer::appendEntry()`/`takeEntry()`, `DriverCommon::logAppHeaderRead()` returning a struct instead of out-parameters).
- Optimize the design first, not the size of the diff: refactor the existing code when the change needs it rather than fitting the change around it. Before reporting a change done, review its diff for near-copies, names whose meaning the change narrowed and asymmetries with the existing counterpart, and propose the improvements, not only the minimal change.
- Adding a counterpart of an existing concept (Allow for Block, inbound for outbound), make it symmetrical: its own enum value, log reason, text and icon; rename the existing one if the change narrows its meaning (cf. `FORT_CONN_REASON_IP_BLOCK_INET` / `IP_ALLOW_INET`). A stored enum's values stay: append the new one before its `*_COUNT`.
- Build on the existing code instead of duplicating it: reuse or split an existing function rather than adding a near-copy, and work with the existing parsers' objects (e.g. `RuleTextParser`'s `RuleFilter`s for a rule text, the `ValueRange`s like `ActionRange`/`OptionRange` for a filter's values and their `FORT_RULE_FILTER_*`/`FORT_CONN_FILTER_RESULT_*` flags) rather than adding own enums, name tables or value structs for the same data; extend the existing class a little if needed (cf. `RuleTextParser::setText()`). Before writing a generic helper (on widgets, actions, menus, strings, files, dates…), grep the utility classes for one doing the same: `form/controls/controlutil.h` (`ControlUtil`, e.g. `setActionTexts()` for a menu's or action group's texts) and `util/*util.h` (`GuiUtil`, `StringUtil`, `FileUtil`, `DateUtil`, `FormatUtil`, `TextAreaUtil`…). When a utility class wraps one side of an API, add the counterpart there instead of calling the API directly, even a one-liner (cf. `GuiUtil::getClipboardText()` for `setClipboardData()`). Likewise follow the file's own idiom for a repeated call: a form showing error boxes from several places calls them through its anonymous namespace's `showErrorMessage()` with the form's error title (cf. `RulesController`, `FilterEditDialog`); add it when adding a second `windowManager()->showErrorBox()`.
- Keep a value apart from its processing: when a class mixes copyable data with a non-copyable or stateful helper (e.g. a `QObject`-based parser and its results), split the data into a light value class held by a member of the processing class (cf. `FilterLineText` in `FilterLine`). Pass the value class around by `const &` and create the processing object locally where it's needed; don't work around the mix with `mutable` members, copies of the data or rvalue-reference parameters.
- Make a window's variant a derived class rather than a mode flag inside the window (cf. `ProgramAlertWindow` of `ProgramEditDialog`, `AppConnsWindow` of `ConnectionsWindow`). The base's constructor only builds the UI and connects the controller (a protected one, when the base's public constructor finishes its own setup, as `ConnectionsWindow`'s does); the derived constructor does its own `setupUi()`, then `initialize()` and `setupFormWindow()`: virtual functions aren't dispatched to the derived class in the base's constructor. The variant overrides `windowCode()`, `restoreWindowState()`/`saveWindowState()` and the `retranslateWindowTitle()` slot; the base offers generic helpers for it (e.g. `ConnectionsWindow::hideEditing()`, `headerLayout()`) instead of knowing the variant. Likewise a model's variant overrides the base's protected virtual hooks (cf. `AppConnSearchModel` of `ConnSearchModel`: `isSearching()`, `lastConnsIdMin()`, `sqlSearchWhere()`, `fillSearchVars()`), and the window creating it passes it to its base window's protected constructor. A window's variant creates the variants of its own windows by a virtual factory (cf. `ProgramEditDialog::createConnsWindow()`: the Alert Program window's one creates `AppAlertConnsWindow`).
- Extract a group of controls repeated in a form into a widget class in the form's subfolder (cf. `AddressesArea` in `form/opt/pages/addresses/`), with signals for the user's edits only (a value set by the code under a `QSignalBlocker`), rather than the form's helpers taking getter/setter `std::function`s. Check the history for a removed class of the same role before writing a new one.
- Order a class's or struct's data members by ascending size: `bool`s and other 1-byte ones first (check an enum's underlying type: many are `: qint8`), then `quint16`, 32-bit ints/enums, pointers, objects (`QString`, containers, structs).
- Keep a field in the same place in all the lists repeating it: the struct, the DB table's columns, the SQL's `SELECT`/`INSERT` and the row's filling, an RPC's variant list, the header's declarations and the .cpp's definitions. Add a new field next to its kin by the struct's (or the table's) order, not at a list's end (cf. `terminated` after `alerted` in `Conn`, the conn DB, `ConnListModel`'s `SELECT` and `fillConnRow()`, `FortLogConnFlag` and `LogBuffer`; `act.terminated` before `act.zone_id` in `ConfManagerRpc`'s Filter Simulator's list; `terminatedText()` before `inheritedText()`).
- Put the activating one of paired functions first, in the header and in the .cpp alike: start → stop, open → close, enable → disable, show → hide, setUp → tearDown.
- Wrap an `if` body in braces when it returns a computed expression (incl. a struct field); simple returns (`return;`, a constant, a plain local variable) stay without braces. The blank line after the `{` of an `if` with a multi-line condition comes from clang-format: don't add or remove it by hand.
- Order the includes by groups separated by a blank line: the file's own header (in a .cpp), the Qt ones (`<QObject>`), the standard library's (`<memory>`, `<functional>`), then the project's ones.
- Declare a constant in an anonymous `namespace { }` as `inline constexpr` (Qt Creator's navigation finds it then); function-local constants stay plain `constexpr`.
- Don't keep a pointer to a member function in a field of a constant table's struct: MSVC 19.51 emits such a field of a class with multiple inheritance (e.g. `WindowManager`: `QObject` + `IocService`) in 12 bytes while its code reads it as 16, so the second item is garbage (a crash). Use a plain function pointer, e.g. a captureless lambda (cf. `windowShows` in `controlcommandwindow.cpp`); an array of the member pointers alone is laid out right (cf. `LogManager::processLogEntry()`).
- Keep a form's code in the order of its controls: the members in the header, their creation in the `setup*Layout()` functions and their lines in `initialize()`, `retranslateUi()`, `fill*()`. Moving a control on the form moves all of them.
- Comment only what isn't evident from the code. Don't describe the change being made in the source (that's the commit message's place), nor leave notes for the reviewer or issue numbers; a workaround for another product's bug names it. When editing an existing comment, keep its wording close to the original.
- Implement the requested UI behavior only: ask before adding a convenience nobody asked for (switching tabs, moving the focus or selection, enabling/disabling controls by other state, guessed defaults or values derived from heuristics), even if it seems obvious; mention it as an option instead.
- While a feature is designed, commit its steps as they go, redesigns included. Only when asked, rebuild the branch's commit series additively: introduce the final design (structures, signatures, helpers) in the earliest commit that needs it, so a later commit doesn't delete or rewrite the code an earlier one added.
- Commit subjects are prefixed by area: `UI:`, `Driver:`, `Tests:`, `Deploy:`, `Installer:`, `README:` — e.g. `UI: ConfManager: Refactor save()`.
- `ChangeLog` is maintained by hand at release time; don't edit it in feature commits.
- App version lives in `src/version/fort_version.h` (`APP_VERSION_*` and `DRIVER_VERSION`).
- A script processing many files or commits (a history rewrite, a check over the branch) doesn't run `git` per file: a process costs ~50–100 ms on Windows. Read the objects by one `git cat-file --batch`, list the commits' changed files by one `git log --name-only`, write the commits by one `git fast-import` (a branch's 28 commits rebuilt in seconds instead of minutes).
