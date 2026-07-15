# Repo Maintainability Scan - 2026-07-14

## Scope

This pass was intentionally non-functional. The goal was to scan the current `main` branch for:

- zombie code
- extra-over-expressive / noisy code
- spaghetti-style structure

No build or behavior verification was performed for this report.

## Executive Verdict

The repo is still very workable, but it is starting to accumulate maintenance drag in three places:

1. large UI files that mix layout, styling, role logic, and workflow orchestration
2. V1/V2 transition leftovers that now read like dead code
3. repeated UI and data-access patterns that will make future changes slower and riskier

This is not a "rewrite now" situation, but it is absolutely at the point where cleanup work would pay off.

## Strongest Findings

### 1. Oversized UI files are carrying too many responsibilities

The biggest maintainability hotspot is the UI layer. Several files are large enough that small feature edits will keep getting slower:

- `src/ui/ticket_service_dialog.cpp` - 729 lines
- `src/ui/train_management_dialog.cpp` - 650 lines
- `src/ui/main_window.cpp` - 553 lines
- `src/ui/account_management_dialog.cpp` - 522 lines
- `src/ui/transfer_dialog.cpp` - 359 lines
- `src/login/login_dialog.cpp` - 398 lines

Common pattern:

- one file owns layout creation
- inline style sheets
- signal wiring
- validation
- status messaging
- workflow branching
- service orchestration

That makes these files hard to scan, hard to review, and easy to break accidentally.

### 2. `train_management_dialog.cpp` contains clear zombie/V2-transition leftovers

This file has the clearest dead-code smell in the repo.

Examples:

- `src/ui/train_management_dialog.cpp:504-530` still creates and displays `remainingSeatsSpin`
- `src/ui/train_management_dialog.cpp:506` even comments that remaining seats were removed from `Train`
- `src/ui/train_management_dialog.cpp:559` uses a literal `if (false)` branch
- `src/ui/train_management_dialog.cpp:689-690` still shows a V2 warning instead of removing the obsolete path

This is classic zombie code: the UI still exposes a concept that the data model no longer wants to own.

### 3. `main_window.cpp` is drifting into orchestration spaghetti

`src/ui/main_window.cpp` is doing more than a main window should.

It currently combines:

- window styling
- role-based dashboard assembly
- repeated module-card definitions
- direct dialog creation
- service availability checks
- duplicated transfer-query launch logic
- debug logging

The biggest repetition is the transfer-query flow, duplicated once per role block:

- `src/ui/main_window.cpp:427-445`
- `src/ui/main_window.cpp:475-493`
- `src/ui/main_window.cpp:524-542`
- `src/ui/main_window.cpp:564-582`

That is manageable today, but it is the kind of duplication that quietly turns small UI edits into copy-paste maintenance.

### 4. Raw SQL is leaking into the UI layer

`src/ui/transfer_dialog.cpp:280-281` directly runs:

- `SELECT stationId, stationName FROM Station ORDER BY stationName`

That is a spaghetti warning sign, not because the query is complicated, but because the dialog is bypassing the normal data-access layer. If station loading rules change, the team now has to remember there is a second path sitting inside a UI file.

This is especially notable because the same dialog already depends on `RouteManager`, so the layering is partially abstracted and partially bypassed.

### 5. Styling is heavily duplicated across dialogs

Large inline style sheets appear in multiple files:

- `src/ui/main_window.cpp`
- `src/ui/ticket_service_dialog.cpp`
- `src/ui/account_management_dialog.cpp`
- `src/login/login_dialog.cpp`
- `src/ui/train_management_dialog.cpp`
- `src/ui/transfer_dialog.cpp`

This is not a bug, but it is a maintainability tax:

- color changes require touching many files
- widget states can drift visually
- style fixes get repeated by hand
- review diffs become very noisy

This is one of the strongest examples of "extra-over-expressive" code in the repo: too much presentation detail is embedded directly inside feature files.

### 6. Debug leftovers are still present in user-facing code

Production-side debug logging is still present in places that users can hit:

- `src/ui/main_window.cpp:296-308`
- `src/route_manager.cpp:82`
- `src/route_manager.cpp:103`
- `src/route_manager.cpp:125`

This is not catastrophic, but it is a cleanup signal. Debug traces inside stable user flows usually mean the file is still carrying temporary scaffolding.

### 7. `database_manager.cpp` is taking on too many roles

`src/database/database_manager.cpp` currently appears to handle:

- database path resolution
- database connection lifecycle
- schema loading/parsing
- table creation
- demo data seeding
- some SQL helper utilities

The heaviest example is the demo-data responsibility:

- `src/database/database_manager.cpp:271` starts `seedDemoData()`

This is practical for a student-sized project, but it does mean one core file now mixes infrastructure setup with demo/business bootstrapping. That is a long-term maintenance smell.

### 8. Manual demo code is mixed into the `tests/` area

`tests/dynamic_pricing_live_demo.cpp` is not really an automated test. It is a manual live dashboard/demo app.

That is fine as a tool, but it blurs the meaning of `tests/`:

- some files are automated smoke tests
- some files are interactive demos

This makes the repo harder to understand for new teammates. A future `tools/`, `sandbox/`, or `demo/` folder would be cleaner.

## Extra-Over-Expressive / Noisy Code Notes

This repo has a recurring style of very narrative comments and very dense inline UI detail.

Examples:

- `src/database/database_manager.cpp` contains many comments that explain obvious mechanics line by line
- `src/login/login_manager.cpp` also uses long explanatory comments for straightforward branches
- `src/ui/transfer_dialog.cpp` uses emoji-rich UI literals such as `🚆`, `⏱`, `🔄`, `⚖️`, `🔍`

None of those are wrong by themselves. The issue is consistency and signal-to-noise:

- obvious comments make real warnings easier to miss
- inline visual detail makes business logic harder to spot
- expressive literals feel ad hoc when only one module uses them

## Priority Cleanup List

### High Priority

- remove V2 zombie UI from `src/ui/train_management_dialog.cpp`
- extract the repeated transfer-query launch flow out of `src/ui/main_window.cpp`
- move station loading in `src/ui/transfer_dialog.cpp` back behind a manager/service method

### Medium Priority

- split the largest dialog files by responsibility, starting with `ticket_service_dialog.cpp`
- centralize shared dialog/QSS styling into a smaller common layer
- trim production `qDebug()` traces that are no longer needed

### Low Priority

- reduce overly narrative comments in core files
- move manual demo programs out of `tests/` into a clearer location

## Suggested Follow-Up Order

If the team wants the highest return with the least churn, this is the order I would use:

1. clean zombie V2 remnants in `train_management_dialog.cpp`
2. deduplicate the transfer-query entry path in `main_window.cpp`
3. remove UI-owned SQL from `transfer_dialog.cpp`
4. extract shared style definitions
5. break up `ticket_service_dialog.cpp` into smaller tab-oriented helpers

## Bottom Line

The repo does not look broken, but it does look like it has started to "thicken" around the UI layer. The biggest risk is not one bad file; it is the combination of:

- large feature-dialog files
- leftover transition code
- repeated inline styling
- a few direct data-access shortcuts

That combination usually slows teams down gradually rather than failing all at once. This is a good moment to do cleanup while the codebase is still understandable.
