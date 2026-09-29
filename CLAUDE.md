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

- Qt is a **custom static build** (`<qt-static>` is the `-prefix` of the scripts below) — see `deploy/qt-build.bat` (win10 x64), `deploy/qt-build-win7.bat`, `deploy/qt-build-arm64.bat`. Many Qt features are disabled (`-no-feature-sql`, `-no-feature-printsupport`, no OpenGL/dbus/icu…), so do not use Qt SQL, QPrinter, QDial, QUndoStack, QMimeDatabase, QDockWidget, etc.
- `CONFIG+=tests` adds the `driver` (user-mode driver test app) and `tests` subprojects; `CONFIG+=driver_payload` adds `driver_payload`. Without them only `ui` + `ui_bin` are built.
- Subproject layout: `ui/` builds the static lib `FortFirewallUILib`, `ui_bin/` links it into `FortFirewall.exe`, tests link the same lib. **New .cpp/.h files must be added explicitly to `SOURCES`/`HEADERS` in `src/ui/FortFirewallUI.pro`.**

### Kernel driver

Built with MSBuild + WDK, not qmake:

```bat
src\driver\msvcbuild-win10-64.bat        :: -> build-driver-win10\x64\fortfw.sys
src\driver\msvcbuild-win7-32.bat         :: msvcbuild.bat <PLAT> <CONFIG> <ARCH>
src\driver\loader\msvcbuild-win10-64.bat :: driver loader (fortfwdl.sys payload wrapper)
```

`fortdrv.vcxproj` compiles **only `fortdrv_amalg.c`** (an amalgamation that `#include`s every driver .c). A new driver source file must be added there *and* to `FortFirewallDriver.pro`.

`src/driver/FortFirewallDriver.pro` builds the driver code as a **user-mode** console app (`driver/test/main.c` plus the `wdm/um_*.c` shims that emulate the kernel APIs) — that is how driver logic is exercised without loading a real driver.

### Tests

GoogleTest/GoogleMock; requires the `GOOGLETEST_DIR` env var pointing at a googletest checkout, otherwise the test subprojects are skipped silently.

```bat
build-win10\tests\UtilTest\UtilTest.exe
build-win10\tests\UtilTest\UtilTest.exe --gtest_filter=ConfUtilTest.*   :: single test/suite
```

Suites: `UtilTest` (bitutil, confutil, dateutil, fileutil, ioccontainer, netutil, ruletextparser, stringutil, wildmatch), `StatTest`, `LogBufferTest`, `LogReaderTest`. `LogReaderTest` needs the loaded kernel driver: run it only manually from a console, never as part of an automated test run. Each `tst_*.h` is included from the suite's `tst_main.cpp` and must also be listed in the suite's `.pro`.

### Translations & deployment

```bat
set QT_HOME=<Qt>\6.9.1\msvc2022_64    :: a regular Qt install with lupdate/lrelease
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

IPC is `QLocalServer`/`QLocalSocket` (`src/ui/control/`): `ControlManager` listens (world-accessible when running as service), `ControlWorker` frames requests, `RpcManager` dispatches them, `src/ui/control/command/controlcommand*.cpp` implement the user-facing CLI commands.

### Configuration pipeline (UI → driver)

`FirewallConf` / `AppGroup` / `Rule` / `Zone` (`src/ui/conf/`) are the in-memory model, persisted to SQLite by `ConfManager` and the specialized `ConfAppManager`, `ConfRuleManager`, `ConfZoneManager`, `ConfGroupManager`. `src/ui/util/conf/confbuffer.cpp` + `confutil.cpp` serialize that model into the **packed binary layout defined in `src/driver/common/fortconf.h`**, which `DriverManager`/`DriverWorker` push to the driver via the `FORT_IOCTL_*` codes in `src/driver/common/fortioctl.h` (`SETCONF`, `ADDAPP`, `SETRULES`, `SETZONES`, `SETGROUPS`, `GETLOG`, …).

`src/driver/common/` is compiled into **both** the UI and the driver (via `src/driver/Driver.pri`). It is the shared contract: changing the `fortconf.h` layout, the IOCTL set, or the log record format requires updating both sides and bumping `DRIVER_VERSION` in `src/version/fort_version.h` (checked in `driver/fortdev.c` against the value written by `confbuffer.cpp`).

Note the distinction between `Group` (`conf/group.h`, rule groups) and `AppGroup` (`conf/appgroup.h`, the app groups with speed limits, max 16).

### Databases

No Qt SQL — a hand-rolled SQLite wrapper in `src/ui/3rdparty/sqlite/` (`SqliteDb`, `SqliteStmt`, `DbQuery`) over the amalgamation in `src/3rdparty/sqlite/`. Separate DBs for conf, traffic stats, connections and the app-info cache; schema versions are applied from `.sql` migrations embedded as Qt resources (`ui/conf/migrations/`, `ui/stat/migrations/*/`, `ui/appinfo/migrations/`).

### Driver internals (`src/driver/`)

`fortdrv.c` entry point; `fortcout.c` WFP callouts registration, `fortcout_ale.c` ALE and `fortcout_pkt.c` packet classify callouts; `fortcnf*.c` the live configuration (conf/rules/groups/zones) with reader-writer locks; `fortbuf.c` the log ring buffer read back by the UI; `fortstat.c` traffic accounting; `fortps.c` process tracking; `fortpool.c`/`forttlsf.c` allocators (TLSF from `src/3rdparty/tlsf`); `fortmod.c` + `loader/` the self-loading module (fortfwdl.sys unpacks the signed payload); `proxycb/` callout trampolines (with .asm variants per arch).

### UI layer

`form/` uses a window + controller pair per feature area (`programswindow.cpp` / `programscontroller.cpp`, and likewise for rules, zones, groups, services, stat, opt, home), with `basecontroller.cpp` resolving IoC dependencies. Table content comes from `model/` (`TableSqlModel`/`TableItemModel` subclasses querying SQLite directly). Background work goes through `util/worker/` (`WorkerManager` + `WorkerObject` + `WorkerJob`) — used by `appinfo/`, `hostinfo/`, `task/` and `stat/`.

## Conventions

- Format with `src/_clang-format` (WebKit-based Qt style, 100 columns, `PointerBindsToType: false`, braces on their own line after functions/classes only).
- Commit subjects are prefixed by area: `UI:`, `Driver:`, `Tests:`, `Deploy:`, `Installer:`, `README:` — e.g. `UI: ConfManager: Refactor save()`.
- User-visible changes get a line in `ChangeLog` under the release heading, referencing the GitHub issue number where applicable.
- App version lives in `src/version/fort_version.h` (`APP_VERSION_*` and `DRIVER_VERSION`).
