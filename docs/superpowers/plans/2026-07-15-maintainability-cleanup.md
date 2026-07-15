# Maintainability Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reduce the highest-impact maintainability debt identified in `docs/maintainability-scan-2026-07-14.md` without changing product behavior.

**Architecture:** Make small, low-risk refactors around the existing Qt Widgets structure rather than redesigning the app. Prioritize removing dead V2 remnants, deduplicating repeated UI orchestration, and restoring cleaner boundaries between UI and data-access code.

**Tech Stack:** C++17, Qt Widgets, SQLite, CMake, existing smoke tests under `tests/`

---

## File Structure

### Existing files to modify first

- `src/ui/train_management_dialog.cpp`
  Responsibility: train management UI, add/edit/delete/search flows
- `include/train_management_dialog.h`
  Responsibility: train management dialog public/private API
- `src/ui/main_window.cpp`
  Responsibility: top-level workspace window and module entry orchestration
- `include/main_window.h`
  Responsibility: main window private helper declarations
- `src/ui/transfer_dialog.cpp`
  Responsibility: transfer-search UI and result rendering
- `include/transfer_dialog.h`
  Responsibility: transfer dialog private helper declarations
- `src/route_manager.cpp`
  Responsibility: route graph build/query logic
- `include/route_manager.h`
  Responsibility: route manager API

### Existing files to modify later

- `src/ui/ticket_service_dialog.cpp`
  Responsibility: ticket search, order management, order query UI
- `include/ticket_service_dialog.h`
  Responsibility: ticket dialog helper declarations
- `src/login/login_dialog.cpp`
- `src/ui/account_management_dialog.cpp`
- `src/database/database_manager.cpp`

### Existing tests to reuse

- `tests/train_api_smoke_test.cpp`
- `tests/ticket_issue9_smoke_test.cpp`
- `tests/ticket_issue10_smoke_test.cpp`
- `tests/dynamic_pricing_smoke_test.cpp`

### Documentation to update after refactor

- `docs/maintainability-scan-2026-07-14.md`
- `README.md`

---

### Task 1: Remove V2 Zombie UI From Train Management

**Files:**
- Modify: `src/ui/train_management_dialog.cpp`
- Modify: `include/train_management_dialog.h`
- Test: `tests/train_api_smoke_test.cpp`

- [ ] **Step 1: Remove obsolete remaining-seat editing controls from the edit dialog**

Delete the stale V2 leftovers inside `TrainManagementDialog::editTrain()`:

```cpp
// Remove these obsolete controls entirely.
QSpinBox *remainingSeatsSpin = new QSpinBox();
remainingSeatsSpin->setRange(0, 999);
remainingSeatsSpin->setValue(0);
form->addRow("剩余座位:", remainingSeatsSpin);
```

Expected result: the edit dialog only shows fields that still belong to `Train`.

- [ ] **Step 2: Remove dead validation branches that can never execute**

Delete the literal dead branch in `TrainManagementDialog::editTrain()`:

```cpp
if (false) {
    showMessage("剩余座位不能超过总座位", false);
    return;
}
```

Expected result: no fake validation remains for a field that no longer exists.

- [ ] **Step 3: Convert the obsolete seat-update action into an explicit disabled/hidden V2-safe path**

Replace vague transitional messaging with one consistent behavior in `updateSeats()`:

```cpp
void TrainManagementDialog::updateSeats()
{
    showMessage(QStringLiteral("当前版本的座位余量由 Trip 数据维护。"), false);
}
```

Also consider disabling `m_seatBtn` during `setupUI()` if the product no longer supports train-level seat editing.

- [ ] **Step 4: Run targeted verification**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem train_api_smoke_test
ctest --test-dir build -C Debug -R train_api_smoke_test --output-on-failure
```

Expected: build succeeds; no train-management regression from UI cleanup.

- [ ] **Step 5: Commit**

```bash
git add include/train_management_dialog.h src/ui/train_management_dialog.cpp
git commit -m "refactor: remove obsolete train seat editing leftovers"
```

---

### Task 2: Deduplicate Transfer Launch Logic In Main Window

**Files:**
- Modify: `include/main_window.h`
- Modify: `src/ui/main_window.cpp`
- Test: manual launch of each role workspace

- [ ] **Step 1: Add one private helper for transfer entry**

Add a single private helper to `MainWindow`:

```cpp
private:
    void openTransferDialog();
```

Expected result: role-specific blocks no longer duplicate the same guard/build/exec sequence.

- [ ] **Step 2: Move the repeated transfer-launch body into `openTransferDialog()`**

Extract the repeated body currently duplicated across role sections:

```cpp
void MainWindow::openTransferDialog()
{
    if (m_trainManager == nullptr) {
        QMessageBox::warning(this, QStringLiteral("换乘查询"),
                             QStringLiteral("车次服务尚未初始化，请稍后重试。"));
        return;
    }

    RouteManager routeManager(m_trainManager->databaseManager());
    if (!routeManager.buildGraph()) {
        QMessageBox::warning(this, QStringLiteral("换乘查询"),
                             QStringLiteral("无法构建路线图：%1").arg(routeManager.lastError()));
        return;
    }

    TransferDialog dialog(&routeManager, this);
    dialog.exec();
}
```

- [ ] **Step 3: Replace all duplicated role lambdas with the helper**

Each role block should call:

```cpp
[this]() {
    openTransferDialog();
}
```

Expected result: one source of truth for transfer-entry behavior.

- [ ] **Step 4: Remove user-facing debug traces from main window**

Delete temporary logging around train/station dialog launch:

```cpp
qDebug() << "=== showTrainStationMessage called ===";
qDebug() << "m_trainManager is NULL!";
qDebug() << "Creating TrainManagementDialog...";
qDebug() << "Executing TrainManagementDialog...";
qDebug() << "TrainManagementDialog closed.";
```

- [ ] **Step 5: Run targeted verification**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem
```

Manual check:

- admin can open transfer dialog
- seller can open transfer dialog
- user can open transfer dialog
- guest can open transfer dialog

- [ ] **Step 6: Commit**

```bash
git add include/main_window.h src/ui/main_window.cpp
git commit -m "refactor: centralize transfer dialog launch flow"
```

---

### Task 3: Move Station Loading Back Behind A Manager Boundary

**Files:**
- Modify: `include/route_manager.h`
- Modify: `src/route_manager.cpp`
- Modify: `include/transfer_dialog.h`
- Modify: `src/ui/transfer_dialog.cpp`
- Test: manual transfer dialog station list load

- [ ] **Step 1: Add a route-manager accessor for station data**

Add a small API to `RouteManager`:

```cpp
QList<QPair<int, QString>> stationOptions() const;
```

Implementation should read station IDs/names through the existing database manager dependency and return plain data, not UI objects.

- [ ] **Step 2: Remove raw SQL from `TransferDialog::loadStations()`**

Delete direct UI-owned SQL usage:

```cpp
#include <QSqlQuery>
QSqlQuery query(db);
query.exec("SELECT stationId, stationName FROM Station ORDER BY stationName");
```

Replace it with:

```cpp
const auto stations = m_routeManager->stationOptions();
for (const auto &station : stations) {
    m_stationNameMap[station.first] = station.second;
    m_fromCombo->addItem(station.second, station.first);
    m_toCombo->addItem(station.second, station.first);
}
```

- [ ] **Step 3: Keep failure handling inside the dialog, but keep data access outside it**

If `m_routeManager` is null or returns no stations, show one dialog-level error path only:

```cpp
showError(QStringLiteral("站点数据加载失败。"));
```

Expected result: the dialog owns rendering and messaging, while the manager owns data retrieval.

- [ ] **Step 4: Run targeted verification**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem
```

Manual check:

- transfer dialog opens
- departure station list loads
- arrival station list loads
- query still works for at least one route

- [ ] **Step 5: Commit**

```bash
git add include/route_manager.h src/route_manager.cpp include/transfer_dialog.h src/ui/transfer_dialog.cpp
git commit -m "refactor: remove raw station SQL from transfer dialog"
```

---

### Task 4: Start Centralizing Shared Dialog Styling

**Files:**
- Create: `include/ui/dialog_style.h`
- Create: `src/ui/dialog_style.cpp`
- Modify: `src/ui/ticket_service_dialog.cpp`
- Modify: `src/ui/account_management_dialog.cpp`
- Modify: `src/login/login_dialog.cpp`
- Modify: `src/ui/transfer_dialog.cpp`
- Modify: `CMakeLists.txt`
- Test: manual visual sanity check

- [ ] **Step 1: Introduce a minimal shared style helper**

Create:

```cpp
// include/ui/dialog_style.h
#pragma once

#include <QString>

namespace DialogStyle {
QString baseDialogQss();
}
```

```cpp
// src/ui/dialog_style.cpp
#include "ui/dialog_style.h"

QString DialogStyle::baseDialogQss()
{
    return QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
    )QSS");
}
```

- [ ] **Step 2: Replace only the duplicated base dialog shell first**

Update each dialog to compose on top of the shared style:

```cpp
setStyleSheet(DialogStyle::baseDialogQss() + QStringLiteral(R"QSS(
    /* file-specific rules only */
)QSS"));
```

Expected result: do not over-refactor all QSS in one pass; only extract the clearly shared foundation.

- [ ] **Step 3: Build after the new shared source is wired into CMake**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target TrainTicketSystem
```

Expected: new source file links cleanly.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt include/ui/dialog_style.h src/ui/dialog_style.cpp src/ui/ticket_service_dialog.cpp src/ui/account_management_dialog.cpp src/login/login_dialog.cpp src/ui/transfer_dialog.cpp
git commit -m "refactor: extract shared dialog base styling"
```

---

### Task 5: Break Ticket Service Dialog Into Smaller Helpers

**Files:**
- Modify: `include/ticket_service_dialog.h`
- Modify: `src/ui/ticket_service_dialog.cpp`
- Test: `tests/ticket_issue9_smoke_test.cpp`
- Test: `tests/ticket_issue10_smoke_test.cpp`
- Test: `tests/dynamic_pricing_smoke_test.cpp`

- [ ] **Step 1: Add focused private builder helpers**

Add helper declarations:

```cpp
void buildSearchControls(QVBoxLayout *parentLayout);
void buildSearchResults(QVBoxLayout *parentLayout);
void buildManageControls(QVBoxLayout *parentLayout);
void buildQueryControls(QVBoxLayout *parentLayout);
```

Expected result: `setupSearchTab()`, `setupManageTab()`, and `setupQueryTab()` become short coordinators rather than giant construction blocks.

- [ ] **Step 2: Extract one section at a time without changing logic**

First extract search-tab controls only, for example:

```cpp
void TicketServiceDialog::setupSearchTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    buildSearchControls(layout);
    buildSearchResults(layout);
    m_tabWidget->addTab(tab, QStringLiteral("车票查询"));
}
```

Do not rename business methods like `searchTrips()`, `bookSelectedTrain()`, `refundOrder()`, or `changeOrder()` in this pass.

- [ ] **Step 3: Keep signal wiring next to the UI chunk that owns it**

For each extracted helper, move only the related controls and `connect()` calls together. Avoid moving business logic into the UI builders.

- [ ] **Step 4: Run targeted verification**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem ticket_issue9_smoke_test ticket_issue10_smoke_test dynamic_pricing_smoke_test
ctest --test-dir build -C Debug -R "ticket_issue9_smoke_test|ticket_issue10_smoke_test|dynamic_pricing_smoke_test" --output-on-failure
```

Expected: existing ticket and pricing behavior stays unchanged.

- [ ] **Step 5: Commit**

```bash
git add include/ticket_service_dialog.h src/ui/ticket_service_dialog.cpp
git commit -m "refactor: split ticket service dialog setup into focused helpers"
```

---

### Task 6: Low-Risk Noise Cleanup

**Files:**
- Modify: `src/route_manager.cpp`
- Modify: `src/database/database_manager.cpp`
- Modify: `src/login/login_manager.cpp`
- Move: `tests/dynamic_pricing_live_demo.cpp` to `tools/dynamic_pricing_live_demo.cpp` or `demo/dynamic_pricing_live_demo.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: root `CMakeLists.txt` if needed
- Modify: `README.md`

- [ ] **Step 1: Remove stale production debug logs from route manager**

Delete or gate temporary logs such as:

```cpp
qDebug() << "RouteGraph: DatabaseManager is null";
qDebug() << "RouteGraph: 时间格式无效:" << train.departureTime << train.arrivalTime;
qDebug() << "RouteGraph: 构建完成，站点数=" << m_stationNames.size();
```

- [ ] **Step 2: Trim only comments that restate obvious code**

Do not remove meaningful rationale. Only shorten comments like step-by-step narration of straightforward branches.

- [ ] **Step 3: Move the live demo out of `tests/`**

Preferred destination:

```text
demo/dynamic_pricing_live_demo.cpp
```

Then update the relevant CMake target to keep the executable available without implying it is an automated test.

- [ ] **Step 4: Run targeted verification**

Run:

```powershell
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Debug --target TrainTicketSystem dynamic_pricing_live_demo dynamic_pricing_smoke_test
```

Expected: main app, live demo target, and smoke test target still configure and build.

- [ ] **Step 5: Commit**

```bash
git add src/route_manager.cpp src/database/database_manager.cpp src/login/login_manager.cpp tests/CMakeLists.txt CMakeLists.txt README.md demo/dynamic_pricing_live_demo.cpp
git commit -m "chore: reduce debug noise and separate demo tooling"
```

---

## Self-Review

### Spec coverage

This plan covers the highest-value items from `docs/maintainability-scan-2026-07-14.md`:

- V2 zombie code
- duplicated transfer flow
- UI-owned SQL
- duplicated base styling
- oversized ticket dialog
- debug/noise cleanup
- demo/tooling boundary cleanup

### Placeholder scan

No `TODO`, `TBD`, or "implement later" placeholders were left in the task steps.

### Type consistency

The new helper/API names introduced in this plan are:

- `MainWindow::openTransferDialog()`
- `RouteManager::stationOptions() const`
- `DialogStyle::baseDialogQss()`
- `TicketServiceDialog::buildSearchControls(...)`
- `TicketServiceDialog::buildSearchResults(...)`
- `TicketServiceDialog::buildManageControls(...)`
- `TicketServiceDialog::buildQueryControls(...)`

Use these exact names if executing the plan as written.

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-07-15-maintainability-cleanup.md`. Two execution options:

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

Which approach?
