cmake_minimum_required(VERSION 3.16)

add_executable(dynamic_pricing_live_demo
    dynamic_pricing_live_demo.cpp
    ../src/ticket/ticket_manager.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_seed.cpp
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
