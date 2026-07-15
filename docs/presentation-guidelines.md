# Train Ticket Management System Presentation Guidelines

## Purpose

This guide is a practical demo script for presenting the project in a live setting. It is based on:

- [requirements.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/requirements.md)
- [TRIP_SCHEMA_V2.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/TRIP_SCHEMA_V2.md)
- [README.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/README.md)
- [!!!合作规范.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/!!!合作规范.md)
- [!!!开发规范.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/!!!开发规范.md)
- [worksplit.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/worksplit.md)
- [分工2.md](/abs/path/C:/Users/13647/OneDrive/Desktop/train/分工2.md)

The goal is to demonstrate all required functionalities clearly, in a stable order, with minimal risk during the presentation.

## Recommended Demo Length

- Short version: 8-10 minutes
- Full version: 12-18 minutes

## Demo Preparation

Before presenting:

1. Build and open the main application.
2. Confirm the seeded demo accounts still work:
   - `admin / admin123`
   - `seller / seller123`
   - `user01 / user123`
   - `lalala / 12345`
   - `seller2 / 12345`
   - guest mode
3. Make sure the seeded demo stations and trains are visible.
4. If the database has become messy during testing, delete:
   - `database\train_ticket_v2.db`
   Then reopen the app to reseed demo data.
5. If you want to show dynamic pricing behavior visually, prepare the live demo target:
   - `.\build\tests\Debug\dynamic_pricing_live_demo.exe`

## What To Emphasize

When introducing the system, highlight these three points first:

- This is a Qt Widgets + C++17 + SQLite desktop system.
- The architecture follows `UI -> Manager -> DatabaseManager -> SQLite`.
- The current ticketing model is V2, based on `Trip`, so the same train can have different seat inventory on different travel dates.

## Recommended Presentation Order

### 1. Project Overview

Show the login page and explain:

- supported identities: administrator, seller, passenger, guest
- major modules: user management, train/station management, ticket service, statistics, route recommendation
- why the V2 `Trip` model matters for date-based ticketing and dynamic pricing

Suggested line:

`Our system separates train templates from dated trips, so ticket inventory and pricing can vary by travel date instead of being tied to one static train record.`

### 2. Guest Access Portal

Log in as guest and show:

- guest landing page
- ticket inquiry access
- route inquiry access
- registration entry
- return-to-login behavior

Required functionality covered:

- guest access portal
- remaining ticket inquiry
- date / departure / arrival based search
- exit / return path

### 3. Passenger Login

Log in as `user01`.

Show:

- ticket query page
- search by travel date
- search by departure / arrival station
- schedule table
- ticket booking flow
- refund flow
- change ticket flow
- personal order inquiry
- password change under personal account

Important detail:

- show that passenger order history is limited to that logged-in user
- mention that actual sold ticket price is stored at order level in V2

### 4. Ticket Seller Login

Log in as `seller`.

Show:

- seller dashboard
- ticket service center
- multi-condition ticket search
- booking for a passenger
- refund
- change ticket
- search by passenger name
- search by order number
- real-time order refresh / remaining seat update

Required functionality covered:

- seller booking operations
- personal order inquiry
- real-time ticket availability refresh
- operation-log related behavior

### 5. Administrator Login

Log in as `admin`.

Show:

- admin dashboard
- employee account management
- create seller account
- reset or change seller password
- enable / disable seller account
- view system operation logs

Suggested line:

`Admin functions are separated from seller and passenger workflows, which keeps role permissions visible and easy to verify during testing.`

### 6. Train and Station Management

Still as admin, show:

- open train management
- add a train
- edit schedule fields
- suspend a train
- restore a train
- permanently delete only when allowed
- search by train number
- search by departure station
- station management dialog

Required functionality covered:

- train schedule management
- station management
- start / stop control
- multi-condition train search
- seat / inventory-related configuration entry

### 7. Ticket Data Statistics

Still as admin, show the statistics center:

- total orders
- booked / refunded / changed counts
- popular routes
- monthly passenger flow

What to explain:

- statistics are computed from order and trip data
- monthly flow now shows real months like `2026-07`
- this area demonstrates historical reporting, not just current ticket search

### 8. Dynamic Pricing

Use one of these two approaches:

- In-app approach:
  Show the same route/date under different seat availability conditions and explain that the displayed price is dynamic.
- Live-demo approach:
  Open `dynamic_pricing_live_demo.exe` and show real-time price updates across scenarios.

Explain that dynamic pricing considers:

- seat scarcity
- near-departure discount scenarios with high remaining seats
- holiday demand
- normal weekday behavior

If asked about implementation:

- base price comes from trip-level pricing support
- dynamic price is calculated in the ticket business layer

### 9. Route Recommendation

Open the transfer / route query dialog and show:

- choose departure station and destination
- choose optimization goal
- query result table

Explain that this covers:

- graph-based route storage
- Dijkstra / shortest-path style transfer recommendation
- time-first / transfer-first / balanced search behavior

### 10. Exit and Closeout

Finish by showing:

- returning from guest mode
- logging out from a real account
- normal exit path

This closes the last requirement item: `Exit System`.

## Feature Coverage Checklist

Use this during rehearsal:

- Administrator login
- Seller login
- Passenger login
- Guest access
- Staff account creation
- Password modification
- Enable / disable seller account
- Operation log viewing
- Train add / edit / suspend / resume / delete
- Station management
- Train search by number
- Train search by departure / arrival station
- Ticket search by date
- Ticket search by station pair
- Book ticket
- Refund ticket
- Change ticket
- Personal order query
- Search order by order number
- Search orders by passenger name
- Historical statistics
- Popular routes
- Monthly passenger flow
- Dynamic pricing
- Route recommendation
- Exit / return flow

## Suggested Team Speaking Split

If multiple teammates are presenting, this order matches the project docs well:

- Presenter 1: project overview, architecture, V2 trip model
- Presenter 2: guest and passenger workflows
- Presenter 3: seller workflows and ticket operations
- Presenter 4: admin workflows, train/station management, logs
- Presenter 5: statistics, dynamic pricing, route recommendation, wrap-up

This aligns reasonably well with the module split described in the root docs.

## Demo Safety Tips

- Avoid creating too many extra trains during the live presentation unless you need to show CRUD.
- Prefer demo accounts over manually created accounts unless account creation is itself being demonstrated.
- If a route query or statistics panel depends on seeded data, verify it before presenting.
- Keep one backup path ready:
  - If the app data gets messy, reset `train_ticket_v2.db`
  - If dynamic pricing is hard to show inside the main app, use the live demo executable

## Q&A Notes

If asked what changed in the newer system design, answer with:

- V2 trip-based inventory
- dated ticket search
- dynamic pricing support
- route recommendation
- broader smoke-test and visual-demo support

If asked about technical discipline, answer with:

- feature branches and PR-based integration
- manager-layer separation from UI
- centralized SQL in `DatabaseManager`
- test coverage plus manual scenario validation
