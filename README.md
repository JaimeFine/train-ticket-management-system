# Train Ticket Management System

Qt Widgets + C++17 + SQLite desktop project for train ticket management.

## Branch note

Current active development is on `develop`.

## Build and run

Configure:

```powershell
cmake -S . -B build
```

Build debug:

```powershell
cmake --build build --config Debug --target TrainTicketSystem
```

Build release:

```powershell
cmake --build build --config Release --target TrainTicketSystem
```

Run:

```text
build\Debug\TrainTicketSystem.exe
```

or

```text
build\Release\TrainTicketSystem.exe
```

## Demo data

On initialization, the system now auto-seeds several public demo accounts and basic stations/trains.

Demo accounts:

- `admin` / `admin123`
  Role: administrator
- `seller` / `seller123`
  Role: seller
- `user01` / `user123`
  Role: normal user
- `user02` / `user456`
  Role: normal user

Demo stations:

- `北京南`
- `南京南`
- `上海虹桥`
- `杭州东`

Demo trains:

- `G1001` 北京南 -> 上海虹桥
- `G1002` 上海虹桥 -> 北京南
- `G2001` 南京南 -> 杭州东
- `G2002` 杭州东 -> 南京南

## How to test the main roles

- Log in as `admin` to test employee account management, train/station management, and statistics.
- Log in as `seller` to test seller-side account access.
- Log in as `user01` or `user02` to test normal-user account page and order history.
- Use guest mode to test registration.

## Notes

- The database file is created automatically if it does not exist.
- Demo seed data is inserted with `INSERT OR IGNORE`, so missing demo accounts/trains can be restored on next startup without duplicating rows.
- If you want a completely fresh demo database, delete:

```text
database\train_ticket.db
```

and start the app again.

## Project structure

```txt
train/
├── src/
├── include/
├── resources/
├── docs/
├── tests/
├── database/
├── CMakeLists.txt
└── README.md
```
