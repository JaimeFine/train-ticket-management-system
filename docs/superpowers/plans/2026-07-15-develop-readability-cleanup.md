# Develop Readability Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the `develop` branch easier to read, easier to explain, and cleaner to maintain without changing product behavior.

**Architecture:** Use small, low-risk refactors that preserve existing Qt Widgets behavior while shrinking oversized files, removing zombie artifacts, and restoring cleaner boundaries between UI, business logic, and demo/test code. Add only short, high-value comments at V2 ownership boundaries and complex lookup points.

**Tech Stack:** C++17, Qt Widgets, SQLite, CMake, existing smoke tests and demo tooling

---

## File Structure

### Files to create first

- `src/ui/train_management_dialog_support.h`
  Responsibility: shared helper declarations used only by the train/trip dialog split
- `src/ui/train_management_dialog_support.cpp`
  Responsibility: `StationComboBox` painting and `styleTrainEditDialog()`
- `src/ui/train_management_dialog_train_ops.cpp`
  Responsibility: train CRUD member-function definitions for `TrainManagementDialog`
- `src/ui/train_management_dialog_trip_ops.cpp`
  Responsibility: trip/history/pricing-related member-function definitions for `TrainManagementDialog`
- `demo/CMakeLists.txt`
  Responsibility: non-test demo targets
- `demo/dynamic_pricing_live_demo.cpp`
  Responsibility: manual visual demo for dynamic pricing
- `src/database/database_manager_seed.cpp`
  Responsibility: `DatabaseManager::seedDemoData()` implementation and demo-data helpers

### Files to modify first

- `CMakeLists.txt`
- `include/train_management_dialog.h`
- `include/ticket_manager.h`
- `include/database_manager.h`
- `include/app_style.h`
- `src/ui/train_management_dialog.cpp`
- `src/ticket/ticket_manager.cpp`
- `src/ui/app_style.cpp`
- `src/ui/ticket_service_dialog.cpp`
- `src/ui/transfer_dialog.cpp`
- `src/ui/account_management_dialog.cpp`
- `src/database/database_manager.cpp`
- `tests/CMakeLists.txt`

### Files to remove

- `tests/train_manager_smoke_test.cpp`

### Existing tests to reuse

- `tests/train_api_smoke_test.cpp`
- `tests/ticket_issue9_smoke_test.cpp`
- `tests/ticket_issue10_smoke_test.cpp`
- `tests/dynamic_pricing_smoke_test.cpp`
- `tests/statistics_smoke_test.cpp`

### Documentation to update last

- `docs/maintainability-scan-2026-07-15-develop.md`
- `README.md`

---

### Task 1: Split `TrainManagementDialog` Into Readable Units

**Files:**
- Create: `src/ui/train_management_dialog_support.h`
- Create: `src/ui/train_management_dialog_support.cpp`
- Create: `src/ui/train_management_dialog_train_ops.cpp`
- Create: `src/ui/train_management_dialog_trip_ops.cpp`
- Modify: `src/ui/train_management_dialog.cpp`
- Modify: `include/train_management_dialog.h`
- Modify: `CMakeLists.txt`
- Test: `tests/train_api_smoke_test.cpp`

- [ ] **Step 1: Add the support-file declarations**

Create `src/ui/train_management_dialog_support.h`:

```cpp
#pragma once

#include <QComboBox>

class QDialog;
class QPaintEvent;

class StationComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;

protected:
    void paintEvent(QPaintEvent *event) override;
};

void styleTrainEditDialog(QDialog &dialog);
```

Expected result: the custom-painted combo box and edit-dialog QSS no longer need to live at the top of the 1200+ line dialog file.

- [ ] **Step 2: Move the helper implementations out of `train_management_dialog.cpp`**

Create `src/ui/train_management_dialog_support.cpp`:

```cpp
#include "train_management_dialog_support.h"

#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

void StationComboBox::paintEvent(QPaintEvent *event)
{
    QComboBox::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal arrowAreaWidth = 36.0;

    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0), 8.0, 8.0);
    painter.setClipPath(clipPath);
    painter.fillRect(QRectF(width() - arrowAreaWidth, 0.0, arrowAreaWidth, height()),
                     QColor(QStringLiteral("#eef5f1")));
    painter.setClipping(false);

    painter.setPen(QPen(QColor(QStringLiteral("#d8e0dc")), 1.0));
    painter.drawLine(QPointF(width() - arrowAreaWidth, 1.0),
                     QPointF(width() - arrowAreaWidth, height() - 1.0));

    painter.setPen(Qt::NoPen);
    painter.setBrush(isEnabled() ? QColor(QStringLiteral("#8fa5a0"))
                                 : QColor(QStringLiteral("#bdc8c4")));

    const qreal centerX = width() - arrowAreaWidth / 2.0;
    const qreal centerY = height() / 2.0;
    QPolygonF arrow;
    arrow << QPointF(centerX - 4.5, centerY - 2.5)
          << QPointF(centerX + 4.5, centerY - 2.5)
          << QPointF(centerX, centerY + 4.0);
    painter.drawPolygon(arrow);

    const qreal borderWidth = hasFocus() ? 2.0 : 1.0;
    const qreal borderOffset = borderWidth / 2.0;
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(hasFocus() ? QColor(QStringLiteral("#0f766e"))
                                   : QColor(QStringLiteral("#cbd8d2")),
                        borderWidth));
    painter.drawRoundedRect(QRectF(borderOffset, borderOffset,
                                   width() - borderWidth,
                                   height() - borderWidth),
                            8.0, 8.0);
}

void styleTrainEditDialog(QDialog &dialog)
{
    dialog.setStyleSheet(
        "QDialog { background: #eef2f3; color: #1f2933; font-family: \"Microsoft YaHei UI\", \"Microsoft YaHei\", \"Segoe UI\"; font-size: 14px; }"
        "QLabel { color: #33433d; }"
        "QLineEdit, QComboBox, QSpinBox { color: #1f2933; background-color: #ffffff; border: 1px solid #cbd8d2; border-radius: 6px; padding: 4px 8px; min-height: 28px; }"
        "QLineEdit::placeholder { color: #8b9490; }"
        "QComboBox::drop-down, QSpinBox::up-button, QSpinBox::down-button { border: none; background: #eef5f1; }"
        "QComboBox QAbstractItemView { color: #1f2933; background-color: #ffffff; border: 1px solid #cbd8d2; selection-background-color: #d9f99d; selection-color: #153832; }"
        "QPushButton { background-color: #176b5b; color: white; border: none; border-radius: 8px; padding: 8px 14px; font-weight: 700; }"
        "QPushButton:hover { background-color: #0f5749; }");
}
```

- [ ] **Step 3: Split the member-function definitions by responsibility**

Leave only constructor/setup/search/selection/message helpers in `src/ui/train_management_dialog.cpp`, and move the heavy slots into two files:

```cpp
// src/ui/train_management_dialog_train_ops.cpp
#include "train_management_dialog.h"
#include "database_manager.h"
#include "train_management_dialog_support.h"

void TrainManagementDialog::addTrain() { /* move existing implementation verbatim */ }
void TrainManagementDialog::editTrain() { /* move existing implementation verbatim */ }
void TrainManagementDialog::deleteTrain() { /* move existing implementation verbatim */ }
void TrainManagementDialog::resumeTrain() { /* move existing implementation verbatim */ }
void TrainManagementDialog::deleteTrainPermanently() { /* move existing implementation verbatim */ }
void TrainManagementDialog::updateSeats() { /* keep the current V2-safe message-only behavior */ }
```

```cpp
// src/ui/train_management_dialog_trip_ops.cpp
#include "train_management_dialog.h"
#include "database_manager.h"
#include "ticket_manager.h"
#include "train_management_dialog_support.h"

void TrainManagementDialog::toggleHistoryMode() { /* move existing implementation verbatim */ }
void TrainManagementDialog::loadTripsForTrain(int trainId) { /* move existing implementation verbatim */ }
void TrainManagementDialog::generateTrips() { /* move existing implementation verbatim */ }
void TrainManagementDialog::editTrip() { /* move existing implementation verbatim */ }
void TrainManagementDialog::disableTrip() { /* move existing implementation verbatim */ }
void TrainManagementDialog::goBackToTrainList() { /* move existing implementation verbatim */ }
```

Expected result: the main dialog file becomes a readable coordinator instead of an all-in-one module.

- [ ] **Step 4: Wire the new files into CMake**

Update `CMakeLists.txt`:

```cmake
set(SOURCES
    src/main.cpp
    src/login/login_dialog.cpp
    src/login/login_manager.cpp
    src/ticket/ticket_manager.cpp
    src/train/train_manager.cpp
    src/statistics/statistics_manager.cpp
    src/database/database_manager.cpp
    src/database/database_manager_user.cpp
    src/database/database_manager_station.cpp
    src/database/database_manager_train.cpp
    src/database/database_manager_trip.cpp
    src/database/database_manager_order.cpp
    src/database/database_manager_operation_log.cpp
    src/route_manager.cpp
    src/ui/account_management_dialog.cpp
    src/ui/main_window.cpp
    src/ui/order_history_dialog.cpp
    src/ui/operation_log_dialog.cpp
    src/ui/ticket_service_dialog.cpp
    src/ui/statistics_dialog.cpp
    src/ui/transfer_dialog.cpp
    src/database/database_manager_ticket.cpp
    src/ui/train_management_dialog.cpp
    src/ui/train_management_dialog_support.cpp
    src/ui/train_management_dialog_train_ops.cpp
    src/ui/train_management_dialog_trip_ops.cpp
    src/ui/station_management_dialog.cpp
    src/database/database_manager_statistics.cpp
    src/ui/app_style.cpp
)
```

- [ ] **Step 5: Verify the split without changing behavior**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem train_api_smoke_test -j 4
ctest --test-dir build -C Debug -R "^train_api_smoke_test$" --output-on-failure
```

Expected: compile succeeds and train CRUD behavior stays unchanged.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt include/train_management_dialog.h src/ui/train_management_dialog.cpp src/ui/train_management_dialog_support.h src/ui/train_management_dialog_support.cpp src/ui/train_management_dialog_train_ops.cpp src/ui/train_management_dialog_trip_ops.cpp
git commit -m "refactor: split train management dialog into focused units"
```

---

### Task 2: Move Dynamic-Price Lookup Out Of The Dialog Loop

**Files:**
- Modify: `include/ticket_manager.h`
- Modify: `src/ticket/ticket_manager.cpp`
- Modify: `src/ui/train_management_dialog_trip_ops.cpp`
- Test: `tests/dynamic_pricing_smoke_test.cpp`

- [ ] **Step 1: Add a pricing lookup helper to `TicketManager`**

Update `include/ticket_manager.h`:

```cpp
#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include <QVariantMap>

class DatabaseManager;

class TicketManager {
public:
    explicit TicketManager(DatabaseManager &db);
    int bookTicket(int userId, int tripId, const QString &passengerName);
    int remainingSeats(int tripId) const;
    QVector<QVariantMap> searchTrips(const QString &dep, const QString &arr, const QString &date = "") const;
    QVector<QVariantMap> searchByTrainNumber(const QString &number) const;
    QHash<int, double> dynamicPricesForRouteDate(const QString &dep,
                                                 const QString &arr,
                                                 const QString &travelDate) const;
    bool refundTicket(int orderId);
    bool changeTicket(int orderId, int newTripId);
    // ...
};
```

- [ ] **Step 2: Implement the helper as a thin wrapper around the existing search path**

Add to `src/ticket/ticket_manager.cpp`:

```cpp
QHash<int, double> TicketManager::dynamicPricesForRouteDate(const QString &dep,
                                                            const QString &arr,
                                                            const QString &travelDate) const
{
    QHash<int, double> prices;
    const QVector<QVariantMap> results = searchTrips(dep, arr, travelDate);
    for (const QVariantMap &result : results) {
        prices.insert(result.value(QStringLiteral("tripId")).toInt(),
                      result.value(QStringLiteral("dynamicPrice")).toDouble());
    }
    return prices;
}
```

Expected result: the dialog can ask one question per route/date instead of building its own business lookup loop.

- [ ] **Step 3: Replace the per-row search loop in `loadTripsForTrain()`**

In `src/ui/train_management_dialog_trip_ops.cpp`, replace:

```cpp
double dynamicPrice = 0.0;
QVector<QVariantMap> results = ticketManager.searchTrips(depName, arrName, trip.travelDate);
for (const QVariantMap &result : results) {
    if (result.value("tripId").toInt() == trip.tripId) {
        dynamicPrice = result.value("dynamicPrice").toDouble();
        break;
    }
}
```

with:

```cpp
const QHash<int, double> dynamicPrices =
    ticketManager.dynamicPricesForRouteDate(depName, arrName, trip.travelDate);

const double dynamicPrice = dynamicPrices.value(trip.tripId, 0.0);
```

Add one short annotation above the map construction:

```cpp
// 动态票价仍由 TicketManager 计算；这里仅按 tripId 读取结果，避免 UI 重复遍历查询结果。
```

- [ ] **Step 4: Verify pricing behavior**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem dynamic_pricing_smoke_test ticket_issue9_smoke_test -j 4
ctest --test-dir build -C Debug -R "^(dynamic_pricing_smoke_test|ticket_issue9_smoke_test)$" --output-on-failure
```

Expected: price-related behavior and query behavior remain unchanged.

- [ ] **Step 5: Commit**

```bash
git add include/ticket_manager.h src/ticket/ticket_manager.cpp src/ui/train_management_dialog_trip_ops.cpp
git commit -m "refactor: centralize trip dynamic-price lookup"
```

---

### Task 3: Remove Zombie Test Code And Move Manual Demo Code Out Of `tests/`

**Files:**
- Delete: `tests/train_manager_smoke_test.cpp`
- Create: `demo/CMakeLists.txt`
- Create: `demo/dynamic_pricing_live_demo.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Test: configure/build of demo and retained smoke tests

- [ ] **Step 1: Delete the stale train-manager smoke test**

Remove `tests/train_manager_smoke_test.cpp`.

Why this file should be removed:

```text
- it is not registered in tests/CMakeLists.txt
- it still uses Train.remainingSeats and updateRemainingSeats()
- it contains "skipped" sections and old pending-API notes
```

Expected result: fewer misleading artifacts for future readers.

- [ ] **Step 2: Create a dedicated demo subdirectory**

Create `demo/CMakeLists.txt`:

```cmake
add_executable(dynamic_pricing_live_demo
    dynamic_pricing_live_demo.cpp
    ../src/ticket/ticket_manager.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_user.cpp
    ../src/database/database_manager_station.cpp
    ../src/database/database_manager_train.cpp
    ../src/database/database_manager_trip.cpp
    ../src/database/database_manager_order.cpp
    ../src/database/database_manager_ticket.cpp
    ../src/database/database_manager_statistics.cpp
    ../src/database/database_manager_operation_log.cpp
)

target_include_directories(dynamic_pricing_live_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(dynamic_pricing_live_demo PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Widgets
)

add_custom_command(TARGET dynamic_pricing_live_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:dynamic_pricing_live_demo>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v2.sql
        $<TARGET_FILE_DIR:dynamic_pricing_live_demo>/database/schema_v2.sql
)
```

- [ ] **Step 3: Move the live demo source file**

Move:

```text
tests/dynamic_pricing_live_demo.cpp
```

to:

```text
demo/dynamic_pricing_live_demo.cpp
```

Then remove the old target block from `tests/CMakeLists.txt`.

- [ ] **Step 4: Add an opt-in demos switch in the root CMake**

Update `CMakeLists.txt`:

```cmake
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_DEMOS "Build demo tools" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_DEMOS)
    add_subdirectory(demo)
endif()
```

Expected result: `tests/` contains tests, and `demo/` contains demos.

- [ ] **Step 5: Verify both paths**

Run:

```powershell
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_DEMOS=ON
cmake --build build --config Debug --target dynamic_pricing_live_demo dynamic_pricing_smoke_test -j 4
ctest --test-dir build -C Debug -R "^dynamic_pricing_smoke_test$" --output-on-failure
```

Expected: the demo still builds, and the real smoke test still runs from the `tests/` subtree.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt demo/CMakeLists.txt demo/dynamic_pricing_live_demo.cpp tests/CMakeLists.txt
git rm tests/train_manager_smoke_test.cpp tests/dynamic_pricing_live_demo.cpp
git commit -m "chore: remove zombie test and separate demo tooling"
```

---

### Task 4: Fix V1/V2 Terminology Drift And Split Demo Seeding Out Of `database_manager.cpp`

**Files:**
- Modify: `include/database_manager.h`
- Modify: `src/database/database_manager.cpp`
- Create: `src/database/database_manager_seed.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/database_init_smoke_test.cpp`, `tests/statistics_smoke_test.cpp`

- [ ] **Step 1: Correct the public comments that still describe the old model**

Update `include/database_manager.h`:

```cpp
// Main functionality of initialization:
// - open SQLite
// - enable foreign key support
// - create the V2 tables from schema_v2.sql if they do not already exist
bool initialize();
```

```cpp
bool createTables();       // 按 schema_v2.sql 创建 V2 表结构
bool seedDemoData();       // 写入演示数据（V2：Train 模板 + Trip 每日班次）
```

Add one short V2 boundary annotation where it helps new readers:

```cpp
// V2: Train 只保存车次模板；Trip 才保存具体日期、余票和基础票价。
```

- [ ] **Step 2: Move demo seeding into its own implementation file**

Leave connection/setup helpers in `src/database/database_manager.cpp`, and move the full `DatabaseManager::seedDemoData()` body to `src/database/database_manager_seed.cpp`:

```cpp
#include "database_manager.h"

#include <QDateTime>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

bool DatabaseManager::seedDemoData()
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        m_lastError = QStringLiteral("Database connection is not available.");
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    // move the existing implementation here without changing logic
}
```

Expected result: `database_manager.cpp` becomes focused on connection/schema setup, while demo bootstrapping becomes separately readable.

- [ ] **Step 3: Keep only high-value annotations in the setup path**

Trim overly narrative comments in `src/database/database_manager.cpp`, but keep concise comments where they explain non-obvious behavior:

```cpp
// SQLite requires PRAGMA foreign_keys per connection.
```

```cpp
// Remove comment noise like "返回最近一次错误信息" above trivial getters.
```

Do not add large explanatory blocks. Add only short rationale comments where there is hidden behavior or V2 ownership logic.

- [ ] **Step 4: Wire the new file into CMake**

Update `CMakeLists.txt` and any test target source lists that compile `database_manager.cpp` directly:

```cmake
set(SOURCES
    src/database/database_manager.cpp
    src/database/database_manager_seed.cpp
    # ...
)
```

In `tests/CMakeLists.txt`, add `../src/database/database_manager_seed.cpp` to every test target that already compiles `../src/database/database_manager.cpp`.

- [ ] **Step 5: Verify database setup and seeded V2 behavior**

Run:

```powershell
cmake --build build --config Debug --target database_init_smoke_test statistics_smoke_test -j 4
ctest --test-dir build -C Debug -R "^(database_init_smoke_test|statistics_smoke_test)$" --output-on-failure
```

Expected: database setup still works and seeded statistics behavior remains unchanged.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt include/database_manager.h src/database/database_manager.cpp src/database/database_manager_seed.cpp tests/CMakeLists.txt
git commit -m "refactor: clarify V2 database setup and isolate demo seeding"
```

---

### Task 5: Finish The Shared-Style Cleanup Without Rewriting The Visual Design

**Files:**
- Modify: `include/app_style.h`
- Modify: `src/ui/app_style.cpp`
- Modify: `src/ui/ticket_service_dialog.cpp`
- Modify: `src/ui/transfer_dialog.cpp`
- Modify: `src/ui/account_management_dialog.cpp`
- Test: manual UI sanity pass, app target build

- [ ] **Step 1: Add small reusable style helpers instead of one giant QSS migration**

Update `include/app_style.h`:

```cpp
#ifndef APP_STYLE_H
#define APP_STYLE_H

#include <QString>

class QTableWidget;

namespace UiStyle {

QString dialogStyleSheet();
QString statusLabelStyle(bool success);
QString titleCardLabelStyle();
QString filledInfoLabelStyle();
void prepareTable(QTableWidget *table);

}

#endif
```

- [ ] **Step 2: Implement only the actually repeated fragments**

Add to `src/ui/app_style.cpp`:

```cpp
QString statusLabelStyle(bool success)
{
    return success
        ? QStringLiteral("color: #28a745; font-size: 13px;")
        : QStringLiteral("color: #dc3545; font-size: 13px;");
}

QString titleCardLabelStyle()
{
    return QStringLiteral(
        "color: #153832;"
        "font-size: 14px;"
        "font-weight: bold;"
        "background-color: #eef5f1;"
        "border: 1px solid #cbd8d2;"
        "border-radius: 8px;"
        "padding: 10px 16px;");
}
```

Do not try to centralize every last selector in one pass. Only centralize the fragments already duplicated.

- [ ] **Step 3: Replace repeated status-label and info-panel styles**

Update:

- `src/ui/train_management_dialog.cpp`
- `src/ui/transfer_dialog.cpp`
- `src/ui/station_management_dialog.cpp`
- `src/ui/ticket_service_dialog.cpp`

Example replacement:

```cpp
m_messageLabel->setStyleSheet(UiStyle::statusLabelStyle(true));
```

```cpp
m_trainInfoLabel->setStyleSheet(UiStyle::titleCardLabelStyle());
```

Expected result: smaller diffs in future UI tweaks, with no visual redesign.

- [ ] **Step 4: Preserve only local QSS that is truly page-specific**

Keep page-specific blocks such as:

```cpp
setStyleSheet(QStringLiteral(R"QSS(
    /* page-specific layout or tab rules only */
)QSS"));
```

but remove duplicated base fragments already covered by `UiStyle`.

- [ ] **Step 5: Verify build and quick UI launch**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem -j 4
```

Manual check:

- train management opens
- transfer dialog opens
- ticket service dialog opens
- status labels still show green/red states correctly

- [ ] **Step 6: Commit**

```bash
git add include/app_style.h src/ui/app_style.cpp src/ui/ticket_service_dialog.cpp src/ui/transfer_dialog.cpp src/ui/account_management_dialog.cpp src/ui/station_management_dialog.cpp src/ui/train_management_dialog.cpp
git commit -m "refactor: centralize repeated dialog style fragments"
```

---

### Task 6: Split `TicketServiceDialog` Into Tab-Scoped Helpers

**Files:**
- Modify: `include/ticket_service_dialog.h`
- Modify: `src/ui/ticket_service_dialog.cpp`
- Test: `tests/ticket_issue9_smoke_test.cpp`, `tests/ticket_issue10_smoke_test.cpp`

- [ ] **Step 1: Add focused helper declarations**

Update `include/ticket_service_dialog.h`:

```cpp
private:
    void setupSearchTab();
    void setupManageTab();
    void setupQueryTab();

    void buildSearchFilters(QVBoxLayout *layout);
    void buildSearchResults(QVBoxLayout *layout);
    void buildManageActions(QVBoxLayout *layout);
    void buildQueryFilters(QVBoxLayout *layout);
    void buildQueryResults(QVBoxLayout *layout);

    void searchTrips();
    void bookSelectedTrain();
    void refundOrder();
    void changeOrder();
    void runOrderQuery();
    void loadOwnOrders();
```

Expected result: setup functions become short coordinators instead of giant mixed layout/workflow blocks.

- [ ] **Step 2: Extract the table/section builders without changing behavior**

Refactor `setupSearchTab()` into:

```cpp
void TicketServiceDialog::setupSearchTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    buildSearchFilters(layout);
    buildSearchResults(layout);
    m_tabWidget->addTab(tab, QStringLiteral("车票查询"));
}
```

Do the same for manage/query tabs.

- [ ] **Step 3: Keep business methods separate from layout builders**

Do not move business code into the UI-build helpers. Keep methods like these intact as workflow methods:

```cpp
void TicketServiceDialog::searchTrips()
void TicketServiceDialog::bookSelectedTrain()
void TicketServiceDialog::refundOrder()
void TicketServiceDialog::changeOrder()
void TicketServiceDialog::refreshOrderQueryTable(const QVector<QVariantMap> &results)
```

Add one short annotation only where the V2 ownership rule is important:

```cpp
// 查询表第一列保存 tripId；后续订票、退票和改签都必须按 Trip 而不是 Train 定位。
```

- [ ] **Step 4: Reduce the local passenger-dialog QSS duplication**

Replace the inline passenger-dialog stylesheet block with the shared dialog base plus only the extra selectors it needs:

```cpp
passengerDialog.setStyleSheet(
    UiStyle::dialogStyleSheet() +
    QStringLiteral(R"QSS(
        QLabel#messageLabel {
            border-radius: 8px;
            padding: 8px 10px;
        }
    )QSS"));
```

- [ ] **Step 5: Verify ticket workflows**

Run:

```powershell
cmake --build build --config Debug --target TrainTicketSystem ticket_issue9_smoke_test ticket_issue10_smoke_test order_api_smoke_test -j 4
ctest --test-dir build -C Debug -R "^(ticket_issue9_smoke_test|ticket_issue10_smoke_test|order_api_smoke_test)$" --output-on-failure
```

Expected: booking, refund, change-ticket, and order-query behavior all remain unchanged.

- [ ] **Step 6: Commit**

```bash
git add include/ticket_service_dialog.h src/ui/ticket_service_dialog.cpp
git commit -m "refactor: split ticket service dialog into tab-scoped helpers"
```

---

### Task 7: Update The Docs To Match The Cleaner Structure

**Files:**
- Modify: `docs/maintainability-scan-2026-07-15-develop.md`
- Modify: `README.md`

- [ ] **Step 1: Mark completed cleanup items in the maintainability scan**

Update the scan’s “Strongest Findings” and “Priority Cleanup List” to reflect what has been completed and what remains.

- [ ] **Step 2: Add a short repo-structure note to the README**

Add a small section like this:

```md
## Repository Layout

- `src/ui/` Qt dialog and window implementations
- `src/ticket/`, `src/train/`, `src/statistics/` business-layer modules
- `src/database/` SQLite access layer
- `tests/` automated smoke tests
- `demo/` manual showcase tools
```

- [ ] **Step 3: Document the V2 ownership rule once, clearly**

Add a compact note to the README:

```md
## V2 Data Model Note

In the current V2 model, `Train` is a schedule template and `Trip` owns dated seat inventory, base fare, and sellable ticket instances.
```

- [ ] **Step 4: Commit**

```bash
git add docs/maintainability-scan-2026-07-15-develop.md README.md
git commit -m "docs: document readability cleanup and V2 repo structure"
```

---

## Self-Review

### Spec coverage

This plan covers the strongest issues from `docs/maintainability-scan-2026-07-15-develop.md`:

- oversized `train_management_dialog.cpp`
- UI/business coupling in dynamic pricing display
- zombie `train_manager_smoke_test.cpp`
- demo tooling mixed into `tests/`
- V1/V2 terminology drift
- partial style centralization
- oversized `ticket_service_dialog.cpp`

### Placeholder scan

No `TODO`, `TBD`, or “implement later” placeholders were left in the task steps. Where a step moves existing code verbatim, the destination file and exact function list are specified.

### Type consistency

The new helper/API names introduced in this plan are:

- `StationComboBox`
- `styleTrainEditDialog(QDialog &dialog)`
- `TicketManager::dynamicPricesForRouteDate(...)`
- `UiStyle::statusLabelStyle(bool success)`
- `UiStyle::titleCardLabelStyle()`
- `TicketServiceDialog::buildSearchFilters(...)`
- `TicketServiceDialog::buildSearchResults(...)`
- `TicketServiceDialog::buildManageActions(...)`
- `TicketServiceDialog::buildQueryFilters(...)`
- `TicketServiceDialog::buildQueryResults(...)`

Use these exact names if executing the plan as written.

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-07-15-develop-readability-cleanup.md`. Two execution options:

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

Which approach?
