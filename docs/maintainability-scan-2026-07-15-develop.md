# Repo Maintainability Scan - 2026-07-15 (`develop`)

## Scope

This pass was intentionally non-functional. The goal was to scan the current `develop` branch for cleanup work that would make the repo:

- easier to read
- easier to review
- easier to extend
- cleaner without changing behavior

No build or behavior verification was performed for this report.

## Executive Verdict

`develop` is in better shape than the older snapshot from 2026-07-14. A few good cleanup steps have already happened:

1. transfer-dialog station loading is now behind `RouteManager`
2. shared UI styling has started to move into `app_style.*`
3. `MainWindow` now has a shared `openTransferDialog()` helper

That said, the repo still has three strong readability hotspots:

1. very large files that mix UI layout, workflow, validation, and business coordination
2. partial cleanup work that stopped halfway, especially around styling and old test artifacts
3. comments and helper names that still describe the old V1 model even though the code is now V2-oriented

This is no longer a "spaghetti emergency", but it is absolutely at the point where a cleanup pass would pay off quickly.

## Strongest Findings

### 1. `train_management_dialog.cpp` has become the biggest readability hotspot in the repo

This file is now the clearest maintenance bottleneck:

- `src/ui/train_management_dialog.cpp` - 1299 lines

It currently combines:

- custom widget painting
- dialog-local style definitions
- train CRUD
- trip CRUD
- trip history filtering
- dynamic price display
- tab navigation
- validation
- status messaging

Concrete examples:

- custom combo-box painting starts at `src/ui/train_management_dialog.cpp:29`
- dialog-specific editor styling starts at `src/ui/train_management_dialog.cpp:79`
- trip loading starts at `src/ui/train_management_dialog.cpp:846`
- UI code directly constructs `TicketManager` at `src/ui/train_management_dialog.cpp:902`
- trip generation dialogs are driven inline at `src/ui/train_management_dialog.cpp:967` and `src/ui/train_management_dialog.cpp:1016`
- tab back-navigation cleanup lives at `src/ui/train_management_dialog.cpp:1273`

None of that is individually wrong. The problem is concentration. Small edits now require scanning through unrelated rendering, validation, data-loading, and navigation code in one place.

### 2. Large orchestrator files are still carrying too many responsibilities

The train/trip dialog is the worst case, but it is not the only one. Current large files include:

- `src/ui/ticket_service_dialog.cpp` - 834 lines
- `src/ui/account_management_dialog.cpp` - 614 lines
- `src/ui/main_window.cpp` - 579 lines
- `src/ticket/ticket_manager.cpp` - 570 lines
- `src/database/database_manager.cpp` - 477 lines
- `src/login/login_manager.cpp` - 469 lines
- `src/route_manager.cpp` - 465 lines

These files tend to mix too many layers of concern at once.

Examples:

- `src/ui/ticket_service_dialog.cpp:271`, `:407`, and `:453` define three tab builders in the same file
- search, booking, and order-query logic all remain in that same file at `src/ui/ticket_service_dialog.cpp:514`, `:573`, and `:782`
- `src/ui/main_window.cpp:349` still uses one large `addModuleCard` lambda plus long role-specific branches starting at `:430`, `:466`, and `:503`

The code works, but the repo is getting harder to hold in one mental model.

### 3. Shared styling exists now, but styling is still only partially centralized

This is improved compared with the older scan, because `app_style.*` now exists. But centralization is incomplete.

Examples:

- shared style layer: `include/app_style.h`, `src/ui/app_style.cpp`
- large inline QSS still exists in `src/ui/ticket_service_dialog.cpp:68`
- `src/ui/main_window.cpp:90` still carries a large window-local stylesheet
- `src/login/login_dialog.cpp:52` still carries its own large stylesheet
- `src/ui/account_management_dialog.cpp:102` still carries another large stylesheet
- `src/ui/transfer_dialog.cpp:35` still styles most of the dialog inline

This creates a halfway state:

- some dialogs use shared style helpers
- some dialogs still own their own visual system
- future visual cleanup will still require touching multiple files

That is cleaner than before, but still noisy.

### 4. `train_management_dialog.cpp` still blurs the UI/business boundary

The clearest current example is dynamic pricing display inside the admin train/trip dialog.

At `src/ui/train_management_dialog.cpp:902`, the dialog creates a `TicketManager` directly so it can compute dynamic prices during table refresh.

This makes the file harder to reason about because the dialog is no longer only:

- showing train/trip state

It is also:

- invoking pricing/business logic
- mapping query results back into rows

That is not a correctness bug by itself, but it makes the dialog much less readable and harder to isolate for future cleanup.

### 5. `tests/train_manager_smoke_test.cpp` looks like zombie code

This is one of the strongest "clean without affecting functionality" targets.

What makes it risky/noisy:

- it is not wired into `tests/CMakeLists.txt`
- it still exercises old `remainingSeats` logic
- it still calls `updateRemainingSeats()`
- it contains explicit skipped sections and pending-API summaries

Concrete examples:

- legacy seat test block starts at `tests/train_manager_smoke_test.cpp:195`
- skipped sections appear at `tests/train_manager_smoke_test.cpp:264`, `:305`, `:315`, and `:328`
- the summary still says pending APIs at `tests/train_manager_smoke_test.cpp:335`

This is exactly the kind of file that confuses teammates:

- is it active?
- is it trusted?
- does it describe current behavior or the old model?

Right now the answer is "not clearly".

### 6. Demo tooling is still mixed into the `tests/` area

`dynamic_pricing_live_demo` is useful, but it reads like test code even though it is actually a manual demonstration tool.

Examples:

- target defined at `tests/CMakeLists.txt:194`
- source file is `tests/dynamic_pricing_live_demo.cpp`

This blurs the meaning of the `tests/` folder:

- some entries are automated smoke tests
- some entries are manual demo apps

That makes the repo less readable for new contributors.

### 7. V1/V2 terminology drift still exists in the database layer comments

The codebase is clearly V2-oriented now, but some comments still describe the old model.

Examples:

- `include/database_manager.h:27` says initialization "creates the Version 1 tables"
- `include/database_manager.h:140` still says `createTables(); // V1 建表`
- meanwhile `src/database/database_manager.cpp:24` and `:29` clearly resolve `schema_v2.sql`

This is a small issue, but it matters because the database layer is a repo entrypoint for new readers. Mismatched comments slow people down immediately.

### 8. `database_manager.cpp` is still carrying too many duties at once

`src/database/database_manager.cpp` is not the worst file by line count, but it still mixes several responsibilities:

- schema path resolution
- SQL parsing
- connection lifecycle
- helper execution utilities
- table creation
- demo data seeding

The clearest "second job" in this file is demo-data bootstrapping:

- `src/database/database_manager.cpp:285` starts `seedDemoData()`

Again, this is practical in a student project. The readability problem is that one core file now serves as both:

- infrastructure setup
- demo content initialization

That is harder to scan than it needs to be.

## Progress Since The Older Scan

A few older maintainability issues are already improved on `develop`:

- transfer station loading no longer appears to run raw SQL directly in the dialog
- `MainWindow` no longer duplicates the full transfer-dialog launch body for every role
- shared styling has started to move into `app_style.*`

So the next cleanup wave should focus less on emergency disentangling and more on decomposition and consistency.

## Extra-Over-Expressive / Noisy Code Notes

The current repo still has a recurring pattern of "helpful but dense" inline explanation and presentation-heavy files.

Examples:

- `src/database/database_manager.cpp` narrates many straightforward mechanics line by line
- `src/ui/transfer_dialog.cpp` still embeds a large amount of widget-specific visual detail inline
- `src/ui/ticket_service_dialog.cpp` has both global dialog QSS and local dialog QSS for the passenger prompt

This is not bad code. It is just noisy code:

- obvious comments dilute high-value comments
- repeated QSS hides business-relevant edits in large visual blocks
- large all-in-one files make reviews heavier than they should be

## Priority Cleanup List

### High Priority

- split `src/ui/train_management_dialog.cpp` by responsibility
- extract dynamic-price lookup out of the train/trip dialog UI layer
- remove or rewrite `tests/train_manager_smoke_test.cpp`
- move `dynamic_pricing_live_demo` out of `tests/`
- fix V1/V2 terminology drift in `include/database_manager.h`

### Medium Priority

- split `src/ui/ticket_service_dialog.cpp` into smaller tab- or workflow-oriented helpers
- continue centralizing shared dialog styles into `app_style.*`
- reduce role-branch bulk in `src/ui/main_window.cpp`
- separate demo seeding concerns from `src/database/database_manager.cpp`

### Low Priority

- trim overly narrative comments in core files
- remove stray typo/empty folders if they are not referenced, such as `scr/ui`
- normalize which dialogs use shared style helpers versus local QSS

## Suggested Follow-Up Order

If the team wants the highest readability return with the least behavior risk, this is the order I would use:

1. split `train_management_dialog.cpp` into train list, trip list, and dialog helper pieces
2. delete or modernize `tests/train_manager_smoke_test.cpp`
3. move `dynamic_pricing_live_demo` into a `demo/` or `tools/` folder
4. clean V1/V2 terminology drift in the database layer comments
5. continue migrating repeated dialog styling into `app_style.*`
6. split `ticket_service_dialog.cpp` after the train/trip dialog is under control

## Bottom Line

The repo is cleaner than it was, but it is still getting thicker around a few very large feature files. The biggest readability risk is no longer one obvious architectural mistake. It is the combination of:

- oversized UI/controller files
- partial cleanup work that stopped midway
- a few lingering legacy test and terminology artifacts

That is good news, because it means most of the remaining cleanup can be done without changing functionality. A focused readability pass on `develop` would likely pay off immediately for review speed, onboarding, and lower-risk feature edits.
