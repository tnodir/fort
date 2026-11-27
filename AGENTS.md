# AGENTS.md

This file provides guidance to AI coding agents when working with code in this repository.

## Project

Fort Firewall — a Windows firewall consisting of a Qt 6 (C++20, widgets) user-mode application and its own WFP kernel driver (`fortfw.sys`, C). Windows 7 and later; targets are `win7` (x86/x64), `win10` (x64) and `win10-arm64`.

Sources live under `src/`. Build trees (`build/build-*/`), binaries and `*.qm` files are git-ignored.

The details of single areas are in `docs/agents/*.md`, each named below where its area is: read one before working on its area. Put a new area's details (more than a paragraph) there too, with a pointer here: this file is read on every request.

## Build

Everything is qmake + MSVC; builds are out-of-source, one directory per target. Run from a Visual Studio command prompt.

```bat
:: Configure once per build dir
cd build\build-win10
<qt-static>\bin\qmake.exe -o Makefile ..\..\src\FortFirewall.pro -spec win32-msvc ^
    "CONFIG-=qml_debug" "CONFIG+=driver_payload" "CONFIG+=tests"
:: Build
jom      :: or nmake
```

- Qt is a **custom static build** (`<qt-static>` is the `-prefix` of the scripts below) — see `deploy/qt-build.bat` (win10 x64), `deploy/qt-build-win7.bat`, `deploy/qt-build-arm64.bat`. Many Qt features are disabled (`-no-feature-sql`, `-no-feature-printsupport`, no OpenGL/dbus/icu…), so do not use Qt SQL, QPrinter, QDial, QUndoStack, QMimeDatabase, etc. Only `qtbase` is built: no Qt Quick/QML (scene graph), Charts or other modules — draw with QPainter or Graphics View (`QGraphicsView`/`QGraphicsScene`, enabled). It is configured `-release` only, and the release builds (as well as the `msvcbuild-*.bat` driver builds) define `NDEBUG`, so `assert()` isn't compiled at all and a typo inside one goes unnoticed. For a debug build (asserts on) use a regular Qt install (`QT_HOME`, which has the debug libs) in a separate build dir: `%QT_HOME%\bin\qmake.exe -o Makefile ..\..\src\FortFirewall.pro -spec win32-msvc "CONFIG+=debug" "CONFIG-=release" "CONFIG+=tests"` + `jom`; run its binaries with `%QT_HOME%\bin` in `PATH`.
- `CONFIG+=tests` adds the `driver` (user-mode driver test app) and `tests` subprojects; `CONFIG+=driver_payload` adds `driver_payload`. Without them only `ui` + `ui_bin` are built.
- `scripts\make\` builds all the targets from a plain console, setting up the compilers' environments themselves: `ui\make-all.bat` (`build\build-win7`, `build\build-win10`, `build\build-win10-arm64`; one by `ui\build-<target>.bat [make args]`), `driver\make-all.bat` (the drivers by the EWDKs), `driver-loader\make-all.bat` (the loaders of the built drivers), `deploy\make-all.bat` (the installers in `deploy\out` and their GnuPG signatures `*.exe.sig`; one by `deploy\build-<target>.bat`); they stop at the first error. The machine's paths (EWDKs, VS `vcvars*.bat`, static Qts, jom, the payloads' key, Inno Setup, `gpg.exe`, the signing key and its passphrase's file) are set in `config.bat`.
- Subproject layout: `ui/` builds the static lib `FortFirewallUILib`, `ui_bin/` links it into `FortFirewall.exe`, tests link the same lib. **New .cpp/.h files must be added explicitly to `SOURCES`/`HEADERS` in `src/ui/FortFirewallUI.pro`.** `dns/` builds the static lib `FortFirewallDnsLib` (the DNS forwarder's messages, listener and resolvers: the c-ares one and the DNS over HTTPS one, with c-ares in `dns/3rdparty/`; only Qt Core/Network and `driver/common`; its files in `dns/dns/`, included as `<dns/dnsquery.h>`), which `ui` includes and links (`FortFirewallUI.pri`); its files go to `src/dns/FortFirewallDns.pro`.
- Never run two builds of the same build dir in parallel (a shell `jom` and Qt Creator's build of it, or two shell ones): they fight over the same files.

### Kernel driver

Built with MSBuild + WDK, not qmake:

```bat
src\driver\msvcbuild-win10-64.bat        :: -> build\build-driver-win10\x64\fortfw.sys
src\driver\msvcbuild-win7-32.bat         :: msvcbuild.bat <PLAT> <CONFIG> <ARCH>
src\driver\loader\msvcbuild-win10-64.bat :: driver loader (fortfwdl.sys payload wrapper)
```

With an EWDK instead of an installed WDK, set up the environment non-interactively (`LaunchBuildEnv.cmd` opens a `cmd /k`): `cmd /c "<EWDK>\BuildEnv\SetupBuildEnv.cmd amd64 && src\driver\msvcbuild-win10-64.bat"`. The `win7` targets need an older EWDK (WDK 10.0.20348 builds them; the 26100 one is for `win10`/arm64).

`fortdrv.vcxproj` compiles **only `fortdrv_amalg.c`** (an amalgamation that `#include`s every driver .c). A new driver source file must be added there *and* to `FortFirewallDriver.pro`. In the amalgamation `FORT_API` is `static` (`driver/common/common.h`), so a function called from another driver .c needs `FORT_API` and a prototype in its header; the user-mode build below compiles each .c separately (`FORT_API` = `extern`) and catches a missing one.

`src/driver/FortFirewallDriver.pro` builds the driver code as a **user-mode** console app (`driver/test/main.c`, which `#include`s the tests' `driver/test/tst_*.c`, plus the `wdm/um_*.c` shims that emulate the kernel APIs) — that is how driver logic is exercised without loading a real driver.

The driver's errors in the System event log (`TRACE()`, `forttrace.c`, the event codes of `driver/evt/fortevt.mc` and their regeneration, `TRACE` in `driver/common/`, reading a user's events): read `docs/agents/driver.md` before changing or debugging them.

### Tests

GoogleTest/GoogleMock; requires the `GOOGLETEST_DIR` env var pointing at a googletest checkout, otherwise the test subprojects are skipped silently.

```bat
build\build-win10\tests\UtilTest\UtilTest.exe
build\build-win10\tests\UtilTest\UtilTest.exe --gtest_filter=ConfUtilTest.*   :: single test/suite
```

Suites: `UtilTest` (bitutil, confutil, connfilter, dateutil, fileutil, filterline, formatutil, ioccontainer, netutil, ruletextparser, stringutil, timeperiod, wildmatch), `ConfTest` (the conf DB's managers), `DnsTest` (the DNS forwarder's messages and resolver), `ActivityTest` (the Activity DB's `ActivityManager`), `StatTest`, `LogBufferTest`, `LogReaderTest`. `LogReaderTest` needs the loaded kernel driver: run it only manually from a console, never as part of an automated test run. Each `tst_*.h` is included from the suite's `tst_main.cpp` and must also be listed in the suite's `.pro`. Run a test executable with its build folder as the current directory: `UtilTest` writes files (`zones/`) relative to it.

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
scripts\make\driver-loader\make-all.bat      :: clear certs, wrap build\build-driver-*\*\fortfw.sys into the loaders
scripts\make\deploy\make-all.bat             :: stage + installer + gpg *.exe.sig for win7, win10, win10-arm64
```

The terms' translations are in the wiki's glossary (the repository `https://github.com/tnodir/fort.wiki`, its local path is in `CLAUDE.local.md`; `i18n/Glossary.md` with the terms' meaning and rules, `i18n/Glossary-<lang>.md` per language): translate a `.ts` by its terms and style. Don't change a glossary's term silently: propose the change, then update the language's page and all the term's strings of the `.ts` together.

The installer (Inno Setup 7, `deploy/FortFirewall.iss`, `create-installer.bat`: the architectures, the file names' suffixes, the 64-bit install mode, the auto-update's parameters, `SuppressibleMsgBox()`, the `.isl` files' code pages): read `docs/agents/installer.md` before changing it.

## Architecture

### Process roles

One executable, several roles selected by command line / settings (`FortSettings`, `src/ui_bin/main.cpp`):

- **Service** (`isService()`) — runs as a Windows service, owns the driver and the databases, no GUI (`WindowManagerFake`).
- **UI client** — the tray/GUI process. When a service is present (`hasService()`) it is *not* master.
- **Control client** (`hasControlCommand()`) — a short-lived process that forwards a CLI command to the running instance and exits.

`isMaster() == !hasService() || isService()` decides which implementations get registered.

### IoC container + RPC mirroring

`IocContainer` (`src/ui/util/ioc/`, its `setUp()`s' order and `Fort::dependency<T>()`, the `Fort::` accessors), `setupServices()` in `src/ui/fortmanager.cpp` (a master's real managers, a client's `*Rpc` ones of `src/ui/rpc/`), the IPC and the control commands (`src/ui/control/`; with the tray running a `-c` command runs in the UI client): read `docs/agents/ioc-rpc.md` before adding a service, a control command or a manager method a client can invoke (**it needs its `…Rpc` override and a `Control::Command` entry**).

### Configuration pipeline (UI → driver)

The in-memory model (`src/ui/conf/`, `ConfManager` and the `Conf*Manager`s), its serialization (`confbuffer.cpp`, `confutil.cpp`) into `fortconf.h`'s layout pushed by the `FORT_IOCTL_*` codes, the shared `src/driver/common/` (**changing its layout, the IOCTL set or the log records needs both sides and a `DRIVER_VERSION` bump**), the Groups and Speed Limits (`app.groups_mask`, `app.in_limit_id`/`out_limit_id`), the Global Before/After Apps rules' ids: read `docs/agents/conf-pipeline.md` before changing them.

The IP Addresses tab's two `AddressGroup`s (`FirewallConf::addressGroups()`, the LAN and Internet lists, `AddressesArea`), a Program's Network Filters and Rules (`app.filters_text`, `FilterEditDialog`, `FilterLine`, `FilterLineText`, the Terminating Rule's line, `app_rule`, the Rules tab), the Time Periods (`period_id`, `ConfTimePeriodManager`, `SETPERIODS`) and the Hosts Zones and the `host()` rule filter (`HostRange`, `HostRefs`, `ADDHOSTIPS`, `FORT_HOST_IPS`, the service's DNS forwarder `DnsManager`, the Connections' host names by the DNS Client's ETW `DnsClientManager`): read `docs/agents/conf-model.md` before changing them.

Ask to Connect pends a blocked connection till the user's decision (`fortpkt_pending.c`, `PendingConnManager`, the Paused Connections and Alert Program windows, `BLOCKPENDING`/`REAUTHPENDING`/`ALLOWPENDING`/`ALLOWPROC`, Allow Process), and the initial Auto-Learn Filter Mode switches to it by a timer (`autoLearnSeconds`, `IniOptions::initialAutoLearnSeconds()`): read `docs/agents/ask-to-connect.md` before changing them.

The rule types (an App Rule or a Preset Rule, `Rule::ruleTypeBit()`) and the default Preset Rules (`ui/conf/rule/presets.json`, `PresetRulesManager`, `rule.preset_code`, the file's `version` and `since`): read `docs/agents/preset-rules.md` before changing them.

### Databases

No Qt SQL: `SqliteDb`/`SqliteStmt`/`DbQuery` (`src/ui/3rdparty/sqlite/`), a DB per area with its `.sql` migrations, the transactions (only by `SqliteDb`'s functions, a writing one by `beginWriteTransaction()`): read `docs/agents/databases.md` before working with a DB.

### Driver internals (`src/driver/`)

The driver's files and the ALE callout's decision of a connection (`fort_callout_ale_check_conf()`: `fort_conf_conn_flags_filtered()`, then the App's check by `fort_conf_conn_app_allowed()`; `common/fortconf_conn.c`, shared with the UI's Filter Simulator; the Zones callback's spin lock): read `docs/agents/driver-internals.md` before changing the driver.

Traffic accounting and the WFP provider (`log_stat` and the flows' contexts, Fort's sublayer and the packets re-injected by other drivers, the reauth transaction and `FORT_DEVICE_PROV_MASK`, `fort_prov_reregister()` and the Windows Insider builds' bug, the sublayers' weights) and the flows' log (a CONN per flow by `FORT_FLOW_CONN_LOGGED`, `FORT_LOG_CONN_ACTIVITY`, the flows' bytes by FLOW_TRAF, FLOW_CLOSED with the last ones): read `docs/agents/driver.md` before changing them.

### UI layer

The forms' window + controller pairs (`form/`), the models (`model/`), the background workers (`util/worker/`), `WindowManager`'s windows and confirmation boxes and the windows' lifetimes (`Qt::WA_DeleteOnClose`, `FormPointer`, `saveWindowState()`), the Options window's working copy of the conf (the options following a change saved elsewhere, `FirewallConf::FlagGroup`s): read `docs/agents/ui-layer.md` before changing a window.

The Main window (`form/main/`: `MainWindow`, its menu bar on the tray's actions, the list windows docked into it as `PanelDock`s or moved out as separate windows, `FormPointer`'s docked state, the docks' saved layout): read `docs/agents/main-window.md` before changing it or a dockable window's header, shortcuts or geometry.

The Activity window (`form/activity/`, `ActivityWindow`, `ActivityTreeModel`: the live processes and their allowed flows with the speeds, read from `ActivityManager`'s DB, which the master writes from the driver's CONN/FLOW_TRAF/FLOW_CLOSED log entries; a client reads the file read-only): read `docs/agents/activity.md` before changing it.

The traffic graphs (`form/graph/`: `GraphWindow`, `GraphPlot` on a `QGraphicsView`, no chart library; the bars, lines, scrolling and animations in device pixels; up to 8 graphs by id, the Graphs window; the taskbar's graphs: `TaskbarGraphWindow`, `TaskbarHost`'s embedding, layering and z-order): read `docs/agents/traffic-graph.md` before changing it.

## Conventions

- Format with `src/_clang-format` (WebKit-based Qt style, 100 columns, `PointerBindsToType: false`, braces on their own line after functions/classes only).
- Keep the UI compatible with Qt 6.1: guard newer APIs with `#if QT_VERSION >= QT_VERSION_CHECK(…)` (e.g. no `QFlags::toInt()`).
- Keep the code clean of CodeScene's Code Health issues: a function's cyclomatic complexity 9+, 5+ arguments, 2+ `&&`/`||` in a condition, a Bumpy Road (2+ bodies with nested conditionals), 70+ lines; a file's 1000+ lines; a pair of a file's near-identical functions of 10+ lines. Check the changed files by `python <dev-tools>/codescene/cc-check.py --changed --if-braces` (`<dev-tools>`: a clone of `https://github.com/tnodir/dev-tools`, its local path is in `CLAUDE.local.md`; from the repository's root, before committing; `pip install lizard`). Read `docs/agents/codescene.md` (how each one is counted and fixed) before splitting a function or a file for them.
- Optimize the design first, not the size of the diff: refactor the existing code when the change needs it rather than fitting the change around it. Before reporting a change done, review its diff for near-copies, names whose meaning the change narrowed and asymmetries with the existing counterpart, and propose the improvements, not only the minimal change.
- Adding a counterpart of an existing concept (Allow for Block, inbound for outbound), make it symmetrical: its own enum value, log reason, text and icon; rename the existing one if the change narrows its meaning (cf. `FORT_CONN_REASON_IP_BLOCK_INET` / `IP_ALLOW_INET`). A stored enum's values stay: append the new one before its `*_COUNT`.
- Build on the existing code instead of duplicating it: reuse or split an existing function rather than adding a near-copy, and work with the existing parsers' objects (e.g. `RuleTextParser`'s `RuleFilter`s for a rule text, the `ValueRange`s like `ActionRange`/`OptionRange` for a filter's values and their `FORT_RULE_FILTER_*`/`FORT_CONN_FILTER_RESULT_*` flags) rather than adding own enums, name tables or value structs for the same data; extend the existing class a little if needed (cf. `RuleTextParser::setText()`). Before writing a generic helper (on widgets, actions, menus, strings, files, dates…), grep the utility classes for one doing the same: `form/controls/controlutil.h` (`ControlUtil`, e.g. `setActionTexts()` for a menu's or action group's texts) and `util/*util.h` (`GuiUtil`, `StringUtil`, `FileUtil`, `DateUtil`, `FormatUtil`, `TextAreaUtil`…). When a utility class wraps one side of an API, add the counterpart there instead of calling the API directly, even a one-liner (cf. `GuiUtil::getClipboardText()` for `setClipboardData()`). Likewise follow the file's own idiom for a repeated call: a form showing error boxes from several places calls them through its anonymous namespace's `showErrorMessage()` with the form's error title (cf. `RulesController`, `FilterEditDialog`); add it when adding a second `windowManager()->showErrorBox()`.
- Keep a value apart from its processing: when a class mixes copyable data with a non-copyable or stateful helper (e.g. a `QObject`-based parser and its results), split the data into a light value class held by a member of the processing class (cf. `FilterLineText` in `FilterLine`). Pass the value class around by `const &` and create the processing object locally where it's needed; don't work around the mix with `mutable` members, copies of the data or rvalue-reference parameters.
- Make a window's or model's variant a derived class rather than a mode flag inside it (cf. `ProgramAlertWindow` of `ProgramEditDialog`): read `docs/agents/ui-layer.md`'s "Variants" before adding one.
- Extract a group of controls repeated in a form into a widget class in the form's subfolder (cf. `AddressesArea` in `form/opt/pages/addresses/`), with signals for the user's edits only (a value set by the code under a `QSignalBlocker`), rather than the form's helpers taking getter/setter `std::function`s. Check the history for a removed class of the same role before writing a new one.
- Order a class's or struct's data members by ascending size: `bool`s and other 1-byte ones first (check an enum's underlying type: many are `: qint8`), then `quint16`, 32-bit ints/enums, pointers, objects (`QString`, containers, structs).
- Keep a field in the same place in all the lists repeating it: the struct, the DB table's columns, the SQL's `SELECT`/`INSERT` and the row's filling, an RPC's variant list, the header's declarations and the .cpp's definitions. Add a new field next to its kin by the struct's (or the table's) order, not at a list's end (cf. `terminated` after `alerted` in `Conn`, the conn DB, `ConnListModel`'s `SELECT` and `fillConnRow()`, `FortLogConnFlag` and `LogBuffer`; `act.terminated` before `act.zone_id` in `ConfManagerRpc`'s Filter Simulator's list; `terminatedText()` before `inheritedText()`).
- Put the activating one of paired functions first, in the header and in the .cpp alike: start → stop, open → close, enable → disable, show → hide, setUp → tearDown.
- Wrap an `if` body in braces, unless it is a simple jump: `return;`, a return of a constant or a plain local variable, `continue;`, `break;`; a body returning a computed expression (incl. a struct field) or doing anything else (a call, an assignment) has them. The blank line after the `{` of an `if` with a multi-line condition comes from clang-format: don't add or remove it by hand.
- Order the includes by groups separated by a blank line: the file's own header (in a .cpp), the Qt ones (`<QObject>`), the standard library's (`<memory>`, `<functional>`), then the project's ones.
- Declare a constant in an anonymous `namespace { }` as `inline constexpr` (Qt Creator's navigation finds it then); function-local constants stay plain `constexpr`.
- Don't keep a pointer to a member function in a field of a constant table's struct: MSVC 19.51 emits such a field of a class with multiple inheritance (e.g. `WindowManager`: `QObject` + `IocService`) in 12 bytes while its code reads it as 16, so the second item is garbage (a crash). Use a plain function pointer, e.g. a captureless lambda (cf. `windowShows` in `controlcommandwindow.cpp`); an array of the member pointers alone is laid out right (cf. `LogManager::processLogEntry()`).
- Keep a form's code in the order of its controls: the members in the header, their creation in the `setup*Layout()` functions and their lines in `initialize()`, `retranslateUi()`, `fill*()`. Moving a control on the form moves all of them.
- Don't show a control before it has a parent: a control of a `setup*Layout()`'s `QLayout` is parentless till the layout is set on a widget, and its `setVisible(true)`/`show()` shows it as a top-level window for a moment (a small window flashing on the form's opening). Hiding it before is fine: it stays hidden in the layout. A visibility update run at the setup and on a signal makes its first call after the layout is set (cf. `ProgramsWindow::updateSortStateCounts()`, `DnsPage::setupUi()`).
- Comment only what isn't evident from the code. Don't describe the change being made in the source (that's the commit message's place), nor leave notes for the reviewer or issue numbers; a workaround for another product's bug names it. When editing an existing comment, keep its wording close to the original.
- Implement the requested UI behavior only: ask before adding a convenience nobody asked for (switching tabs, moving the focus or selection, enabling/disabling controls by other state, guessed defaults or values derived from heuristics), even if it seems obvious; mention it as an option instead.
- While a feature is designed, commit its steps as they go, redesigns included. Only when asked, rebuild the branch's commit series additively: introduce the final design (structures, signatures, helpers) in the earliest commit that needs it, so a later commit doesn't delete or rewrite the code an earlier one added.
- Commit subjects are prefixed by area: `UI:`, `Driver:`, `Tests:`, `Deploy:`, `Installer:`, `README:` — e.g. `UI: ConfManager: Refactor save()`.
- `ChangeLog` is maintained by hand at release time; don't edit it in feature commits.
- App version lives in `src/version/fort_version.h` (`APP_VERSION_*` and `DRIVER_VERSION`).
- A script processing many files or commits (a history rewrite, a check over the branch) doesn't run `git` per file: a process costs ~50–100 ms on Windows. Read the objects by one `git cat-file --batch`, list the commits' changed files by one `git log --name-only`, write the commits by one `git fast-import` (a branch's 28 commits rebuilt in seconds instead of minutes).
