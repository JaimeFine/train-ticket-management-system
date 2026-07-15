# Train Ticket Management System Presentation Guidelines

## Purpose

This guide is the live-demo script for the current stable system. It is meant to help the team present the finished project in a clear, low-risk order.

Primary reference documents:

- [README.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/README.md)
- [docs/README.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/README.md)
- [docs/legacy/requirements.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/legacy/requirements.md)
- [docs/legacy/TRIP_SCHEMA_V2.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/legacy/TRIP_SCHEMA_V2.md)
- [docs/legacy/UNIFIED_TECHNICAL_SPEC_CN.pdf](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/legacy/UNIFIED_TECHNICAL_SPEC_CN.pdf)
- [docs/legacy/!!!合作规范.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/legacy/!!!合作规范.md)
- [docs/legacy/!!!开发规范.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/legacy/!!!开发规范.md)
- [docs/legacy/worksplit.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/legacy/worksplit.md)
- [docs/legacy/分工2.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/docs/legacy/分工2.md)

## Recommended Demo Length

- Short version: 8-10 minutes
- Full version: 12-18 minutes

## Demo Preparation

Before presenting:

1. Build and open the main application.
2. Confirm the default demo accounts still work:
   - `admin / admin123`
   - `seller / seller123`
   - `user01 / user123`
   - `user02 / user456`
   - guest mode
3. If the database has become messy during testing, delete:
   - `database\train_ticket_v2.db`
   Then reopen the app so demo data is seeded again.
4. If you want to show dynamic pricing more clearly, prepare the live demo executable. Both of these paths have been used in this repo:
   - `.\build\demo\Debug\dynamic_pricing_live_demo.exe`
   - `.\build\tests\Debug\dynamic_pricing_live_demo.exe`
5. Before the presentation starts, sanity-check:
   - ticket query page
   - order query page
   - seller workflow
   - admin train/trip management
   - statistics page

## What To Emphasize First

Open with these points:

- This is a Qt Widgets + C++17 + SQLite desktop system.
- The architecture follows `UI -> Manager -> DatabaseManager -> SQLite`.
- The current runtime model is V2 trip-based ticketing, not the older train-only model.

Suggested line:

`Our system separates reusable train templates from dated trips, so seats, travel date, and pricing can vary per trip instead of being fixed to one train record.`

## Recommended Presentation Order

### 1. Project Overview

Show the login page and explain:

- supported identities: administrator, seller, passenger, guest
- major modules: account management, train/station management, ticket service, route recommendation, statistics
- why the V2 `Trip` model matters for date-based ticketing and dynamic pricing

### 2. Guest Access

Use guest mode and show:

- guest entry
- ticket inquiry access
- route inquiry access
- registration path if needed
- return-to-main behavior

Requirement coverage:

- guest access
- basic inquiry without login
- normal return flow

### 3. Passenger Workflow

Log in as `user01`.

Show:

- ticket query
- search by departure station and arrival station
- optional date filtering
- booking a ticket
- order query
- search by order number
- refund
- change ticket
- personal account / password change if needed

Important talking point:

- a passenger only sees their own orders, even when searching by `订单号`

### 4. Seller Workflow

Log in as `seller`.

Show:

- seller dashboard
- ticket service center
- ticket booking for a passenger
- refund
- change ticket
- search by passenger name
- search by order number
- refreshed order/ticket state after operations

Requirement coverage:

- seller ticket operations
- more flexible order search
- real-time inventory updates

### 5. Administrator Workflow

Log in as `admin`.

Show:

- admin dashboard
- seller account management
- create seller account
- reset seller password
- enable / disable seller account
- view operation logs

Suggested line:

`Admin-only functions are clearly separated from seller and user workflows, which makes the permission model easy to verify during demonstration.`

### 6. Train and Trip Management

Still as admin, show:

- train management dialog
- add a train
- search by train number
- search by station
- edit a train
- suspend and resume a train
- permanent delete rule when allowed
- switch into trip management for a selected train
- generate trips
- edit trip date, seats, and base fare
- show current dynamic price column
- switch to history mode if useful

This is one of the strongest current modules, so it is worth showing cleanly.

### 7. Station Management

Still as admin, show:

- station management dialog
- add station
- view or delete station as allowed

### 8. Statistics and History

Open the statistics area and show:

- total orders
- booked / refunded / changed counts
- popular routes
- monthly passenger flow

Explain:

- this module demonstrates historical reporting, not only live transaction pages

### 9. Dynamic Pricing

Use one of these approaches:

- In-app:
  show trip search results and explain that displayed ticket price changes with trip conditions
- Live demo:
  run `dynamic_pricing_live_demo.exe` and press `立即刷新`

Explain that dynamic pricing now demonstrates:

- different base fares
- different remaining seat levels
- holiday behavior
- near-departure behavior

Talking point:

- base fare comes from the stored trip data, and dynamic price is computed in the ticket business layer

### 10. Route Recommendation

Open the transfer / route dialog and show:

- choose departure station
- choose destination station
- choose optimization strategy
- show the recommended result

Explain:

- this is graph-based route planning
- it supports time-first, transfer-first, and balanced modes

### 11. Exit and Wrap-Up

Finish by showing:

- logout
- guest return path
- normal system exit

This cleanly closes the final requirement category.

## Feature Coverage Checklist

Use this during rehearsal:

- Administrator login
- Seller login
- Passenger login
- Guest access
- Seller account creation
- Password modification
- Enable / disable seller account
- Operation log viewing
- Train add / edit / suspend / resume / delete
- Trip generation and trip editing
- Station management
- Train search by number
- Train search by station
- Ticket search by date
- Ticket search by station pair
- Book ticket
- Refund ticket
- Change ticket
- Personal order query
- Search order by order number
- Search orders by passenger name
- Statistics dashboard
- Popular routes
- Monthly passenger flow
- Dynamic pricing
- Route recommendation
- Exit / return flow

## Suggested Team Speaking Split

If multiple teammates are presenting:

- Presenter 1: system overview, architecture, V2 trip model
- Presenter 2: guest and passenger workflows
- Presenter 3: seller workflow and order operations
- Presenter 4: admin workflow, train/trip/station management, operation logs
- Presenter 5: statistics, dynamic pricing, route recommendation, wrap-up

## Demo Safety Tips

- Avoid creating too many extra records during the live presentation unless CRUD is the point you are showing.
- Prefer seeded demo accounts over ad hoc accounts unless account creation is itself part of the demonstration.
- If the main app data becomes messy, reset `train_ticket_v2.db` before the presentation.
- Keep the dynamic pricing live demo ready as a backup path.
- Rehearse at least one full order lifecycle in advance:
  - search
  - book
  - order query by number
  - refund or change

## Q&A Notes

If asked what the important final-system improvements are, answer with:

- V2 trip-based ticketing
- dynamic pricing
- train and trip management separation
- route recommendation
- statistics and operation-log support
- maintainability and documentation cleanup

If asked about technical discipline, answer with:

- feature-branch integration
- manager-layer separation from UI
- centralized database access in `DatabaseManager`
- smoke-test support plus manual scenario validation
