# Configuration pipeline (UI → driver)

Moved out of `AGENTS.md`: read it when working on this area.

## The model and its serialization

`FirewallConf` / `App` / `Group` / `SpeedLimit` / `TimePeriod` / `Rule` / `Zone` (`src/ui/conf/`) are the in-memory model, persisted to SQLite by `ConfManager` and the specialized `ConfAppManager`, `ConfRuleManager`, `ConfZoneManager`, `ConfGroupManager`, `ConfSpeedLimitManager`, `ConfTimePeriodManager`. `src/ui/util/conf/confbuffer.cpp` + `confutil.cpp` serialize that model into the **packed binary layout defined in `src/driver/common/fortconf.h`**, which `DriverManager`/`DriverWorker` push to the driver via the `FORT_IOCTL_*` codes in `src/driver/common/fortioctl.h` (`SETCONF`, `ADDAPP`, `SETRULES`, `SETZONES`, `SETGROUPS`, `SETSPEEDLIMITS`, `GETLOG`, …).

## The shared contract

`src/driver/common/` is compiled into **both** the UI and the driver (via `src/driver/Driver.pri`). It is the shared contract: changing the `fortconf.h` layout, the IOCTL set, or the log record format requires updating both sides and bumping `DRIVER_VERSION` in `src/version/fort_version.h` (checked in `driver/fortdev.c` against the value written by `confbuffer.cpp`).

## Groups and Speed Limits

Programs belong to `Group`s (`conf/group.h`, max 32: enabled, exclusive, an optional Time Period and an optional Rule) by the `app.groups_mask` bit mask, and reference one-direction `SpeedLimit`s (`conf/speedlimit.h`, max 32) by `app.in_limit_id` / `app.out_limit_id`. They replace the old App. Groups, which the conf DB's migration (`ConfMigration`) converts.

## Global rules

The Global Before/After Apps rules are sent to the driver as two synthetic rules (`ConfRuleManager::walkRules()`) with the ids after the max rule id (up to `FORT_CONF_RULE_ID_MAX`): don't reuse the deleted rules' free ids for them.

The IP Addresses, the Network Filters and Rules, the Time Periods and the Hosts Zones: `conf-model.md`.
