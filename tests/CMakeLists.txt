cmake_minimum_required(VERSION 3.16)

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

add_test(
    NAME database_init_smoke_test
    COMMAND database_init_smoke_test
)

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

add_test(
    NAME user_api_smoke_test
    COMMAND user_api_smoke_test
)

add_custom_command(TARGET user_api_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:user_api_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:user_api_smoke_test>/database/schema_v1.sql
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

add_test(
    NAME station_api_smoke_test
    COMMAND station_api_smoke_test
)

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

add_test(
    NAME train_api_smoke_test
    COMMAND train_api_smoke_test
)

add_custom_command(TARGET train_api_smoke_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:train_api_smoke_test>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../database/schema_v1.sql
        $<TARGET_FILE_DIR:train_api_smoke_test>/database/schema_v1.sql
)
