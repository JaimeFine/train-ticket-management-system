# Train Ticket Management System

A Qt Widgets desktop application for train ticket sales, trip scheduling, route search, account management, and statistics, backed by SQLite.

This repository now follows the V2 ticketing model:

- `Train` is the reusable schedule template
- `Trip` is the dated sellable instance
- ticket booking, refund, change, and dynamic pricing operate on `tripId`

## Highlights

- Multi-role access: guest, user, seller, admin
- Ticket search, booking, refund, and change
- Dynamic pricing based on seat pressure, departure timing, and holiday rules
- Train and trip management for administrators
- Route and transfer recommendation
- Order history, operation logs, and statistics dashboards
- Auto-seeded demo data for fast evaluation

## Tech Stack

- C++17
- Qt Widgets
- Qt SQL
- SQLite
- CMake

## Quick Start

Configure:

```powershell
cmake -S . -B build
```

Build:

```powershell
cmake --build build --config Debug --target TrainTicketSystem -j 4
```

Run:

```powershell
.\build\Debug\TrainTicketSystem.exe
```

For Release builds:

```powershell
cmake --build build --config Release --target TrainTicketSystem -j 4
.\build\Release\TrainTicketSystem.exe
```

## Optional Targets

Enable automated smoke tests:

```powershell
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Debug --target dynamic_pricing_smoke_test -j 4
ctest --test-dir build -C Debug --output-on-failure
```

Enable manual demo tools:

```powershell
cmake -S . -B build -DBUILD_DEMOS=ON
cmake --build build --config Debug --target dynamic_pricing_live_demo -j 4
.\build\demo\Debug\dynamic_pricing_live_demo.exe
```

## Demo Accounts

- `admin` / `admin123`
- `seller` / `seller123`
- `user01` / `user123`
- `user02` / `user456`

The database is created automatically on first launch. To reset local demo data, delete `database\train_ticket_v2.db` and start the app again.

## Repository Layout

```text
train/
├── database/    SQLite schema files
├── demo/        Manual demonstration tools
├── docs/        Developer notes, plans, scan reports
├── include/     Public headers and record structs
├── resources/   Qt resources and icons
├── src/         Application implementation
├── tests/       Automated smoke tests
├── CMakeLists.txt
└── README.md
```

## Notes

- The application currently ships with `database/schema_v2.sql`.
- `BUILD_TESTS` and `BUILD_DEMOS` are both optional and default to `OFF`.
- See [`docs/README.md`](docs/README.md) for the internal architecture and maintenance guide.
