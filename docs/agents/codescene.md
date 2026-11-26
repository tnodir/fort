# CodeScene's checks

Moved out of `AGENTS.md`'s Conventions: read it when splitting the code for them.

## The thresholds

As CodeScene measures them:

- "Complex Method": cyclomatic complexity 9+. Every `case`/`default` label, `if`, loop, `continue`, `&&`, `||` and `?:` adds one (`break` doesn't); split long `switch`es into helpers or a function table indexed by the enum (e.g. `fort_conf_rule_filter_check_funcList` in `driver/common/fortconf.c`).
- "Excess Number of Function Arguments": 5+ arguments; group them into a struct (cf. `FORT_CALLOUT_ARG`). The WFP callouts' fixed signatures are exempt.
- "Complex Conditional": 2+ `&&`/`||` in one `if`/`while` condition; move the rest into a named helper or early returns.
- "Bumpy Road": 2+ bumps, a bump being a body (an `if`/`else` branch, a loop's or `switch`'s body) of a function's top-level statement which contains another conditional or loop; move the nested ones into helpers.
- "Large Method": a function of 70+ lines of code (non-empty, non-comment lines, the signature included); split it into helpers.
- "Lines of Code in a Single File": a file of 1000+ lines of code (non-empty, non-comment lines); move a part into a file of its own.
- "Duplicated Function Blocks" / "High Degree of Code Duplication": a pair of a file's functions of 10+ lines of code (non-empty, non-comment lines, the signature included) of the same structure: their token sequences match by ~96%+ with the names and numbers generalized, and they share 40%+ of their identifiers (e.g. a handler per IOCTL checking the input's size and calling its setter, a function per group of filters filling them field by field, a reader per log entry type, the two `switch*ConnsWindow()` of `ProgramEditDialog`). Lists of different calls (e.g. `FwpmFilterDeleteByKey0()` of different GUIDs) share too few identifiers to count. Keep the per-item data in a table and process it by one function (cf. `fortDeviceControlProcess_list`'s `in_len` in `driver/fortdev.c`, the `FORT_PROV_GROUP_ARGS` in `driver/common/fortprov.c`, `g_calloutArgs` in `driver/fortcout.c`; a `static const` table refers to the GUIDs by `const GUID *`), or move the common steps into helpers until each function is under 10 lines (cf. `LogBuffer::appendEntry()`/`takeEntry()`, `DriverCommon::logAppHeaderRead()` returning a struct instead of out-parameters).

## The check

`codescene/cc-check.py` of `dev-tools` (`https://github.com/tnodir/dev-tools`, its `codescene/README.md`; `<dev-tools>` is its clone) reports the C/C++ functions and files exceeding them, as CodeScene measures them (its docstring describes how each one is counted):

```bat
pip install lizard
python <dev-tools>/codescene/cc-check.py --changed --if-braces   :: the files changed vs HEAD
python <dev-tools>/codescene/cc-check.py src/driver/fortdev.c    :: given files
```

- Run it from the repository's root after editing C/C++ and before committing. The options `-C`, `-A`, `-L`, `-B`, `-M`, `-F` set the allowed maxima (by default 8, 4, 1, 1, 69, 999).
- A function's issue disabled by CodeScene's directive in a `//` comment right above it (`// @CodeScene(disable:"Excess Number of Function Arguments")`, `@CodeScene(disable-all)`) isn't listed, only counted in the summary. Add one only where the code can't follow the rule (e.g. the WFP callouts' fixed signatures) and the user agreed.
- It checks whole files, so a changed file's old issues and duplicated pairs are listed too: tell them apart from the change's new ones, and fix only the new ones (the old ones in a commit of their own, when asked).
- `--if-braces` also runs `check-if-braces.py HEAD`: an `if` among the changed lines whose body isn't a simple jump and has no braces (Conventions). Without arguments it checks the lines the branch added (`origin/master...HEAD`). A new `if` among a function's existing braceless ones follows them (e.g. `dataDisplayAction()` in `applistmodeldata.cpp`): its report is expected.
- The scripts check only these rules: still read the new lines against the other Conventions.

## The thresholds' sources

CodeScene's defaults and the duplicates' criteria are in `dev-tools`' `codescene/README.md`. Whether 70 and 1000 themselves are reported is not verified (CodeScene's PR reports show only 78 → 81 over 70). As of 2026-11-02 no function or file of `src/ui`/`src/driver` hits them (the largest: `trayicon.cpp` 920 lines, `IfacePage::onRetranslateUi()` 65; the driver's `wdm/um_fwpsk.h` of the user-mode shims, 1707 lines of declarations, aside). The duplicates' criteria were fitted on 30 pairs of CodeScene's reports for `fortdev.c` @5ee96826e, `fortprov.c` @c85ba5943, `fortcout.c` @5ee96826e, `logbuffer.cpp` @b43f0a485 and `programeditdialog.cpp` @e35818145, which they reproduce exactly (the lines' similarity missed `readEntryProcKill`/`readEntryFlowClosed`).
