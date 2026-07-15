# Developer README

This document is the technical entry point for maintaining the repository. It focuses on architecture, core classes, data objects, file layout, and safe extension rules.

## Architecture Overview

The project follows a simple desktop layering model:

```text
Qt Dialog / MainWindow
    -> Manager classes
        -> DatabaseManager
            -> SQLite (schema_v2.sql)
```

Responsibilities are intentionally split this way:

- UI classes build widgets, collect input, render results, and show status messages
- Manager classes hold workflow and business logic
- `DatabaseManager` is the only module that directly reads and writes SQLite
- record structs under `include/database/` are lightweight data carriers between layers

## V2 Data Model

The current codebase is built around the V2 `tripId` flow.

- `TrainRecord`: template-level schedule information such as train number, route, timetable, and default seat capacity
- `TripRecord`: a concrete dated trip instance created from a train template; this is the sellable inventory unit
- `OrderRecord`: user order bound to a `tripId`

In practice:

- admins manage train templates and dated trips separately
- users buy tickets for trips, not for abstract trains
- dynamic pricing reads trip state, especially date and remaining seats

The active runtime schema is `database/schema_v2.sql`.

## Top-Level Directory Map

```text
database/
  schema_v1.sql          legacy schema reference
  schema_v2.sql          active schema used by the app and tests

demo/
  dynamic_pricing_live_demo.cpp
  CMakeLists.txt

docs/
  README.md
  maintainability-scan-*.md
  superpowers/plans/*.md

include/
  *.h                    public class headers
  database/*.h           record structs

src/
  database/              SQLite access implementation
  login/                 authentication and account workflows
  statistics/            statistics aggregation logic
  ticket/                booking, refund, change, pricing logic
  train/                 train-template business logic
  ui/                    dialogs and main window

tests/
  *_smoke_test.cpp       automated smoke tests
  CMakeLists.txt
```

## Core Classes

### `DatabaseManager`

File entrypoints:

- `include/database_manager.h`
- `src/database/database_manager*.cpp`

Role:

- initializes SQLite and schema
- seeds demo data
- exposes CRUD and query APIs for users, stations, trains, trips, orders, tickets, statistics, and logs
- owns transaction helpers used by booking-related workflows

Important note:

- this is the only layer that should speak SQL directly

Current implementation split:

- `database_manager.cpp`: initialization, connection, schema execution, shared helpers
- `database_manager_seed.cpp`: demo-data seeding
- `database_manager_user.cpp`, `*_station.cpp`, `*_train.cpp`, `*_trip.cpp`, `*_order.cpp`, `*_ticket.cpp`, `*_statistics.cpp`, `*_operation_log.cpp`: domain-specific queries

### `LoginManager`

File entrypoints:

- `include/login_manager.h`
- `src/login/login_manager.cpp`

Role:

- authentication
- guest login
- user registration
- seller account creation, reset, enable/disable
- self-service password changes
- role-based access checks

### `TrainManager`

File entrypoints:

- `include/train_manager.h`
- `src/train/train_manager.cpp`

Role:

- train-template CRUD and search
- bridge between UI-friendly `Train` objects and database `TrainRecord`

### `TicketManager`

File entrypoints:

- `include/ticket_manager.h`
- `src/ticket/ticket_manager.cpp`

Role:

- ticket search
- booking, refund, and change
- order queries
- dynamic pricing calculation
- operation-log writes related to ticket workflows

This is the main business-logic layer for the V2 `tripId` ticketing flow.

### `StatisticsManager`

File entrypoints:

- `include/statistics_manager.h`
- `src/statistics/statistics_manager.cpp`

Role:

- wraps statistics-oriented database queries into UI-ready summaries

### `RouteManager`

File entrypoints:

- `include/route_manager.h`
- `src/route_manager.cpp`

Role:

- builds an in-memory route graph from database data
- computes shortest-path and transfer recommendations
- supports multiple optimization criteria

### `MainWindow`

File entrypoints:

- `include/main_window.h`
- `src/ui/main_window.cpp`

Role:

- role-based entry hub after login
- opens the correct management and service dialogs for the current user

## Main UI Modules

These are the user-facing dialogs under `src/ui/` and `include/`:

- `login_dialog`: entry login screen
- `ticket_service_dialog`: search, booking, orders, guest/user ticket workflows
- `train_management_dialog`: admin train and trip management
- `station_management_dialog`: station maintenance
- `account_management_dialog`: seller account administration and password operations
- `statistics_dialog`: order and traffic statistics
- `order_history_dialog`: order history browsing
- `operation_log_dialog`: operation-log browsing
- `transfer_dialog`: route and transfer recommendation
- `app_style`: shared dialog/table/button styling helpers

## Train Management Dialog Split

Recent cleanup reduced the size of the train management feature without changing behavior:

- `train_management_dialog.cpp`: dialog setup and overall coordination
- `train_management_dialog_support.*`: shared helpers and support code
- `train_management_dialog_train_ops.cpp`: train-side operations
- `train_management_dialog_trip_ops.cpp`: trip-side operations

If more cleanup is needed later, continue splitting by responsibility rather than moving logic back into one large file.

## Core Record Objects

The `include/database/` folder contains plain data structs used across layers:

- `user_record.h`: user id, username, password hash/plain storage fields used by the current project, role, enable state
- `station_record.h`: station id and station name
- `train_record.h`: train template data, including station ids, timetable, capacity, enable state
- `trip_record.h`: trip id, train id, travel date, seat inventory, base price, enable state
- `order_record.h`: order id, user id, trip id, passenger name, purchase time, status

These structs should stay simple. Avoid putting business behavior into them.

## Build, Test, and Demo Conventions

### Main Application

```powershell
cmake -S . -B build
cmake --build build --config Debug --target TrainTicketSystem -j 4
```

### Automated Smoke Tests

```powershell
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Debug --target dynamic_pricing_smoke_test -j 4
ctest --test-dir build -C Debug --output-on-failure
```

Notes:

- automated tests live under `tests/`
- tests copy `database/schema_v2.sql` beside the built test executable
- `tests/CMakeLists.txt` is for automated verification targets only

### Manual Demonstrations

```powershell
cmake -S . -B build -DBUILD_DEMOS=ON
cmake --build build --config Debug --target dynamic_pricing_live_demo -j 4
```

Notes:

- manual showcase utilities live under `demo/`
- `dynamic_pricing_live_demo` is meant for visual inspection, not pass/fail CI

## Safe Extension Rules

When adding or modifying functionality, prefer these boundaries:

- add SQL in `DatabaseManager` domain files, not in dialogs
- keep dialogs focused on presentation and orchestration
- put workflow rules in managers, not in record structs
- extend `schema_v2.sql` first when a feature needs persistent data
- update both app and smoke-test wiring when adding new source files

## Readability and Cleanup Notes

The repo is functional, but these areas remain maintenance hotspots:

- `ticket_service_dialog.cpp` is still large and mixes multiple workflows
- some dialogs still keep substantial inline styles instead of shared helpers
- `main_window.cpp` still carries large role-based UI branching
- comments must stay aligned with V2 terminology so new contributors do not confuse `trainId` and `tripId`

For a broader cleanup roadmap, see:

- `docs/maintainability-scan-2026-07-15-develop.md`
- `docs/superpowers/plans/2026-07-15-develop-readability-cleanup.md`

## Practical Entry Points

If you are debugging or extending a feature, start here:

- login or role issue: `LoginManager`, `login_dialog`, `MainWindow`
- ticket purchase or dynamic pricing: `TicketManager`, `ticket_service_dialog`, trip queries in `DatabaseManager`
- train/trip CRUD: `TrainManager`, `train_management_dialog`, `database_manager_train.cpp`, `database_manager_trip.cpp`
- route recommendation: `RouteManager`, `transfer_dialog`
- statistics mismatch: `StatisticsManager`, `database_manager_statistics.cpp`

## Maintenance Bottom Line

If we keep UI, manager, and database responsibilities separated, the project remains easy to evolve. The biggest risk is not missing functionality; it is letting workflow logic slide back into very large dialog files or bypassing `DatabaseManager` with ad hoc SQL in UI code.
