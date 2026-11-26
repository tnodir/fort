# IoC container, RPC mirroring and IPC

Moved out of `AGENTS.md`: read it when working on this area.

## IoC container

`IocContainer` (`src/ui/util/ioc/ioccontainer.h`) is a thread-pinned, type-id-indexed service locator (max 32 services). Services derive from `IocService` and get `setUp()`/`tearDown()` lifecycle. The `setUp()`s run in the order of the registration (`setupServices()`): a service using another's state in its `setUp()` takes it by `Fort::dependency<T>()`, which sets that one up first (cf. `TaskManager::setUp()` and the Zones' types of `ZoneListModel`). Typed accessors live in `Fort::` namespace helpers (`src/ui/fortglobal.h`, e.g. `Fort::confManager()`, `IoC<T>()`).

## Services' registration: master and client

`setupServices()` in `src/ui/fortmanager.cpp` is the key place to read: for a **master** it registers the real managers (`ConfManager`, `StatManager`, `DriverManager`, …); for a **client** it registers `*Rpc` subclasses from `src/ui/rpc/` that implement the same interface by marshalling calls over IPC to the service. **Any new manager method that a client can invoke needs a matching override in its `…Rpc` class and a `Control::Command` entry.**

## IPC and the control commands

IPC is `QLocalServer`/`QLocalSocket` (`src/ui/control/`): `ControlManager` listens (world-accessible when running as service), `ControlWorker` frames requests, `RpcManager` dispatches them, `src/ui/control/command/controlcommand*.cpp` implement the user-facing CLI commands. `ControlManager::connectToAnyServer()` tries the UI process first, so with a service and a running tray a `-c` command runs in the UI **client** (the `*Rpc` managers, a read-only conf DB): a manager method that a `ControlCommand*` calls to change state must have its `…Rpc` override; test the control commands with the tray running too.
