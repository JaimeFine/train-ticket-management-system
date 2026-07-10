cmake_minimum_required(VERSION 3.16)
project(TrainTicketSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Find Qt (supports both Qt5 and Qt6) ──────────────────────────────────────
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

message(STATUS "Using Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")

# ── Source files grouped by module ────────────────────────────────────────────
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
    src/database/database_manager_order.cpp
    src/ui/account_management_dialog.cpp
    src/ui/main_window.cpp
    src/database/database_manager_ticket.cpp
)

set(HEADERS
    include/account_management_dialog.h
    include/login_dialog.h
    include/login_manager.h
    include/ticket_manager.h
    include/train_manager.h
    include/statistics_manager.h
    include/database_manager.h
    include/main_window.h
    include/train_management_dialog.h
)

# ── Resources ────────────────────────────────────────────────────────────────
set(RESOURCES
    resources/resources.qrc
)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    src/ui/train_management_dialog.cpp

)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/database/schema_v1.sql
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/database/schema_v1.sql
)

# ── Tests (optional subdirectory) ────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

