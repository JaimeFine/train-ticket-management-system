cmake_minimum_required(VERSION 3.16)

set(QT_TEST_RUNTIME_DIR "" CACHE PATH "Qt runtime bin directory used by CTest on Windows")
if(WIN32 AND NOT QT_TEST_RUNTIME_DIR AND EXISTS "C:/msys64/ucrt64/bin")
    set(QT_TEST_RUNTIME_DIR "C:/msys64/ucrt64/bin")
endif()

function(add_qt_smoke_test testTarget)
    # Windows 下 CTest 有时找不到 Qt6*.dll，这里给测试程序补上 Qt 运行库目录。
    if(WIN32 AND QT_TEST_RUNTIME_DIR)
        add_test(
            NAME ${testTarget}
            COMMAND ${CMAKE_COMMAND} -E env
                "PATH=${QT_TEST_RUNTIME_DIR}"
                $<TARGET_FILE:${testTarget}>
        )
    else()
        add_test(
            NAME ${testTarget}
            COMMAND ${testTarget}
        )
    endif()
endfunction()

add_executable(database_init_smoke_test
    database_init_smoke_test.cpp
    ../src/database/database_manager.cpp
)

target_include_directories(database_init_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(database_init_smoke_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(database_init_smoke_test)

add_custom_command(TARGET database_init_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:database_init_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:database_init_smoke_test>/database/schema_v1.sql
)

add_executable(user_api_smoke_test
    user_api_smoke_test.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_user.cpp
)

target_include_directories(user_api_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(user_api_smoke_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(user_api_smoke_test)

add_custom_command(TARGET user_api_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:user_api_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:user_api_smoke_test>/database/schema_v1.sql
)

add_executable(login_role_permission_test
    login_role_permission_test.cpp
    ../src/login/login_manager.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_user.cpp
)

target_include_directories(login_role_permission_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(login_role_permission_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(login_role_permission_test)

add_custom_command(TARGET login_role_permission_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:login_role_permission_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:login_role_permission_test>/database/schema_v1.sql
)

add_executable(login_account_management_test
    login_account_management_test.cpp
    ../src/login/login_manager.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_user.cpp
)

target_include_directories(login_account_management_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(login_account_management_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(login_account_management_test)

add_custom_command(TARGET login_account_management_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:login_account_management_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:login_account_management_test>/database/schema_v1.sql
)

add_executable(ticket_issue9_smoke_test
    ticket_issue9_smoke_test.cpp
    ../src/ticket/ticket_manager.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_user.cpp
    ../src/database/database_manager_station.cpp
    ../src/database/database_manager_train.cpp
    ../src/database/database_manager_order.cpp
    ../src/database/database_manager_ticket.cpp
)

target_include_directories(ticket_issue9_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(ticket_issue9_smoke_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(ticket_issue9_smoke_test)

add_custom_command(TARGET ticket_issue9_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:ticket_issue9_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:ticket_issue9_smoke_test>/database/schema_v1.sql
)

add_executable(station_api_smoke_test
    station_api_smoke_test.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_station.cpp
)

target_include_directories(station_api_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(station_api_smoke_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(station_api_smoke_test)

add_custom_command(TARGET station_api_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:station_api_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:station_api_smoke_test>/database/schema_v1.sql
)

add_executable(train_api_smoke_test
    train_api_smoke_test.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_station.cpp
    ../src/database/database_manager_train.cpp
)

target_include_directories(train_api_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(train_api_smoke_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(train_api_smoke_test)

add_custom_command(TARGET train_api_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:train_api_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:train_api_smoke_test>/database/schema_v1.sql
)

add_executable(order_api_smoke_test
    order_api_smoke_test.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_user.cpp
    ../src/database/database_manager_station.cpp
    ../src/database/database_manager_train.cpp
    ../src/database/database_manager_order.cpp
)

target_include_directories(order_api_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(order_api_smoke_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(order_api_smoke_test)

add_custom_command(TARGET order_api_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:order_api_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:order_api_smoke_test>/database/schema_v1.sql
)

add_executable(ticket_search_smoke_test
    ticket_search_smoke_test.cpp
    ../src/database/database_manager.cpp ../src/database/database_manager_user.cpp
    ../src/database/database_manager_station.cpp ../src/database/database_manager_train.cpp
    ../src/database/database_manager_order.cpp ../src/database/database_manager_ticket.cpp
    ../src/ticket/ticket_manager.cpp
)
target_include_directories(ticket_search_smoke_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(ticket_search_smoke_test PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql)
add_qt_smoke_test(ticket_search_smoke_test)
add_custom_command(TARGET ticket_search_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ticket_search_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql $<TARGET_FILE_DIR:ticket_search_smoke_test>/database/schema_v1.sql)

add_executable(ticket_issue10_smoke_test
    ticket_issue10_smoke_test.cpp
    ../src/database/database_manager.cpp
    ../src/database/database_manager_user.cpp
    ../src/database/database_manager_station.cpp
    ../src/database/database_manager_train.cpp
    ../src/database/database_manager_order.cpp
    ../src/database/database_manager_ticket.cpp
    ../src/ticket/ticket_manager.cpp
)

target_include_directories(ticket_issue10_smoke_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(ticket_issue10_smoke_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

add_qt_smoke_test(ticket_issue10_smoke_test)

add_custom_command(TARGET ticket_issue10_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:ticket_issue10_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:ticket_issue10_smoke_test>/database/schema_v1.sql
)
