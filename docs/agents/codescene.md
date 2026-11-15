# CodeScene's checks

The thresholds are in `AGENTS.md`'s Conventions. `scripts/codescene/cc-check.py` reports the C/C++ functions and files exceeding them, as CodeScene measures them (its docstring describes how each one is counted):

```bat
pip install lizard
python scripts/codescene/cc-check.py --changed              :: the files changed vs HEAD
python scripts/codescene/cc-check.py src/driver/fortdev.c   :: given files
```

- Run it from the repository's root after editing C/C++ and before committing. The options `-C`, `-A`, `-L`, `-B`, `-M`, `-F` set the allowed maxima (by default 8, 4, 1, 1, 69, 999).
- It checks whole files, so a changed file's old issues and duplicated pairs are listed too: tell them apart from the change's new ones, and fix only the new ones (the old ones in a commit of their own, when asked).
- `--changed` also runs `check-if-braces.py HEAD`: an `if` among the changed lines whose body isn't a simple jump and has no braces (Conventions). Without arguments it checks the lines the branch added (`master...HEAD`). A new `if` among a function's existing braceless ones follows them (e.g. `dataDisplayAction()` in `applistmodeldata.cpp`): its report is expected.
- The scripts check only these rules: still read the new lines against the other Conventions.

## The thresholds' sources

CodeScene's defaults per language are the JSON files of the zip linked from helpcenter.codescene.com "What Are the Default Threshold Values for Different Programming Languages?" (`https://codescene.com/hubfs/Technical%20Support%20Data/default-rules.zip`, `code-health-rules.cpp.json`; its other thresholds match the script's, so the `_warning` ones are reached by the value itself); whether 70 and 1000 themselves are reported is not verified (CodeScene's PR reports show only 78 → 81 over 70). As of 2026-11-02 no function or file of `src/ui`/`src/driver` hits them (the largest: `trayicon.cpp` 920 lines, `IfacePage::onRetranslateUi()` 65; the driver's `wdm/um_fwpsk.h` of the user-mode shims, 1707 lines of declarations, aside). The duplicates' criteria: lizard's `nloc` ≥ 10 (CodeScene's default `function_duplication_min_lines_of_code_for_check`), `difflib`'s ratio of the generalized tokens ≥ 0.96, the identifiers' Jaccard index ≥ 0.4: fitted on 30 pairs of CodeScene's reports for `fortdev.c` @5ee96826e, `fortprov.c` @c85ba5943, `fortcout.c` @5ee96826e, `logbuffer.cpp` @b43f0a485 and `programeditdialog.cpp` @e35818145, which it reproduces exactly (the lines' similarity missed `readEntryProcKill`/`readEntryFlowClosed`).
