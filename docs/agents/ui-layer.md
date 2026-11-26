# UI layer

Moved out of `AGENTS.md`: read it when working on this area.

## Forms, models and workers

`form/` uses a window + controller pair per feature area (`programswindow.cpp` / `programscontroller.cpp`, and likewise for rules, zones, groups, services, traffic, connections, opt, home), with `basecontroller.cpp` resolving IoC dependencies. Table content comes from `model/` (`TableSqlModel`/`TableItemModel` subclasses querying SQLite directly). Background work goes through `util/worker/` (`WorkerManager` + `WorkerObject` + `WorkerJob`) — used by `appinfo/`, `hostinfo/`, `task/` and `stat/`.

## Windows

`WindowManager` holds one window per `WindowCode` (`FormPointer`). Its confirmation boxes (`showConfirmBox()`, `showQuestionBox()`, `showChoiceBox()`) are application-modal and call back by the context object passed, the caller: the box is closed with it, without calling back. Other windows are created by their owner with `Qt::WA_DeleteOnClose`: a dialog's child window (e.g. its Program Connections window) stays above it and is closed with it explicitly (the dialog may be only hidden and reused), an independent one (the Programs window's Program Connections windows) closes on `WindowManager::aboutToCloseAllWindows()` (on quit and on the settings' import). `FormPointer` calls a window's `saveWindowState()` on its closing; such a window, if it keeps its state, calls it on its `aboutToClose` itself (cf. `AppAlertConnsWindow`).

## Variants

Make a window's variant a derived class rather than a mode flag inside the window (cf. `ProgramAlertWindow` of `ProgramEditDialog`, `AppConnsWindow` of `ConnectionsWindow`). The base's constructor only builds the UI and connects the controller (a protected one, when the base's public constructor finishes its own setup, as `ConnectionsWindow`'s does); the derived constructor does its own `setupUi()`, then `initialize()` and `setupFormWindow()`: virtual functions aren't dispatched to the derived class in the base's constructor. The variant overrides `windowCode()`, `restoreWindowState()`/`saveWindowState()` and the `retranslateWindowTitle()` slot; the base offers generic helpers for it (e.g. `ConnectionsWindow::hideEditing()`, `headerLayout()`) instead of knowing the variant. Likewise a model's variant overrides the base's protected virtual hooks (cf. `AppConnSearchModel` of `ConnSearchModel`: `isSearching()`, `lastConnsIdMin()`, `sqlSearchWhere()`, `fillSearchVars()`), and the window creating it passes it to its base window's protected constructor. A window's variant creates the variants of its own windows by a virtual factory (cf. `ProgramEditDialog::createConnsWindow()`: the Alert Program window's one creates `AppAlertConnsWindow`).

## Options window

The Options window keeps a working copy of the conf, and only three of its options follow a change saved elsewhere: Filter Enabled, Block Traffic and Filter Mode, which the Filter Off and Auto-Learn timers, the quota, the tray and `-c` change while the window is open. On `ConfManager::confChanged` `OptionsPage::updateTrafficFlags()` takes the saved value of those not edited; an edited one stays and overwrites the saved value on saving. The window saves these flags and the Active Period (a deleted Time Period is cleared in the saved conf; `StatisticsPage` saves what its selector shows) only when edited, by `FirewallConf::FlagGroup`s. The other options stay stale, as accepted: an `IniOptions` key is written only when edited, but saving any other flag writes all of them, and any address edit all the addresses, over those changed by `-c conf set`, `-c addr` or another session's Options window. The IP Addresses' Zones follow a Zone's removal: `MaskSelector::onIdRemoved()` emits `maskChanged()`, which marks the window edited, else saving the addresses would write the removed Zone's bit back.

The Main window: `main-window.md`; the Activity window: `activity.md`; the traffic graphs: `traffic-graph.md`.
