cmake_minimum_required(VERSION 3.22)

project(MultiStart VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)
set(QT_PACKAGE "Qt${QT_VERSION_MAJOR}")

add_subdirectory(third_party/check_auto_update EXCLUDE_FROM_ALL)
# CheckAutoUpdate currently advertises C++20 although its public API and implementation
# use only C++17. Keep MultiStart's Qt 5.15.2/C++17 compatibility contract intact.
set_property(TARGET CheckAutoUpdate PROPERTY COMPILE_FEATURES cxx_std_17)
set_property(TARGET CheckAutoUpdate PROPERTY INTERFACE_COMPILE_FEATURES cxx_std_17)
set_property(TARGET CheckAutoUpdate PROPERTY CXX_STANDARD 17)
set_property(TARGET CheckAutoUpdate PROPERTY CXX_STANDARD_REQUIRED ON)

# Qt 5's QStringList has multiple assignment overloads, so the dependency's bare
# initializer-list assignment is ambiguous with MSVC. Compile a generated compatibility
# copy until the upstream source uses an explicit QStringList initializer.
set(CHECK_AUTO_UPDATE_MANAGER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/check_auto_update/src/UpdateManager.cpp")
file(READ "${CHECK_AUTO_UPDATE_MANAGER_SOURCE}" CHECK_AUTO_UPDATE_MANAGER_CONTENT)
string(REPLACE
    "arguments = {QStringLiteral(\"/SILENT\")"
    "arguments = QStringList{QStringLiteral(\"/SILENT\")"
    CHECK_AUTO_UPDATE_MANAGER_COMPAT_CONTENT
    "${CHECK_AUTO_UPDATE_MANAGER_CONTENT}"
)
if(CHECK_AUTO_UPDATE_MANAGER_COMPAT_CONTENT STREQUAL CHECK_AUTO_UPDATE_MANAGER_CONTENT)
    message(FATAL_ERROR "CheckAutoUpdate Qt 5 compatibility patch no longer matches upstream")
endif()
set(CHECK_AUTO_UPDATE_COMPAT_DIR "${CMAKE_CURRENT_BINARY_DIR}/check_auto_update_compat")
file(MAKE_DIRECTORY "${CHECK_AUTO_UPDATE_COMPAT_DIR}")
set(CHECK_AUTO_UPDATE_MANAGER_COMPAT_SOURCE
    "${CHECK_AUTO_UPDATE_COMPAT_DIR}/UpdateManager.cpp")
file(WRITE "${CHECK_AUTO_UPDATE_MANAGER_COMPAT_SOURCE}"
    "${CHECK_AUTO_UPDATE_MANAGER_COMPAT_CONTENT}")
set_source_files_properties(
    "${CHECK_AUTO_UPDATE_MANAGER_SOURCE}"
    TARGET_DIRECTORY CheckAutoUpdate
    PROPERTIES HEADER_FILE_ONLY TRUE
)
target_sources(CheckAutoUpdate PRIVATE "${CHECK_AUTO_UPDATE_MANAGER_COMPAT_SOURCE}")

get_target_property(QT_QMAKE_EXECUTABLE ${QT_PACKAGE}::qmake IMPORTED_LOCATION)
get_filename_component(QT_RUNTIME_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

set(MULTISTART_SOURCES
    src/main.cpp
    src/AppItem.h
    src/ConfigLoader.h
    src/ConfigLoader.cpp
    src/Launcher.h
    src/Launcher.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    resources/resources.qrc
)

if(WIN32)
    list(APPEND MULTISTART_SOURCES resources/app.rc)
endif()

add_executable(MultiStart WIN32 ${MULTISTART_SOURCES})
target_link_libraries(MultiStart PRIVATE ${QT_PACKAGE}::Widgets CheckAutoUpdate::CheckAutoUpdate)
target_include_directories(MultiStart PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(MultiStart PRIVATE MULTISTART_VERSION="${PROJECT_VERSION}")

set_target_properties(MultiStart PROPERTIES
    OUTPUT_NAME "MultiStart"
    WIN32_EXECUTABLE TRUE
)

add_custom_command(TARGET MultiStart POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:CheckAutoUpdate>
            $<TARGET_FILE_DIR:MultiStart>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/apps.json
            $<TARGET_FILE_DIR:MultiStart>/apps.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts
            $<TARGET_FILE_DIR:MultiStart>/scripts
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:MultiStart>/data
    COMMENT "Copying example configuration and launch targets"
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    get_filename_component(CXX_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(QT_TEST_PATH_MODIFICATIONS "PATH=path_list_prepend:${QT_RUNTIME_DIR}")
    if(IS_DIRECTORY "${CXX_RUNTIME_DIR}")
        list(APPEND QT_TEST_PATH_MODIFICATIONS
            "PATH=path_list_prepend:${CXX_RUNTIME_DIR}"
        )
    endif()

    add_executable(ConfigLoaderTests
        tests/ConfigLoaderTest.cpp
        src/AppItem.h
        src/ConfigLoader.h
        src/ConfigLoader.cpp
    )
    target_link_libraries(ConfigLoaderTests PRIVATE ${QT_PACKAGE}::Core ${QT_PACKAGE}::Test)
    target_include_directories(ConfigLoaderTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME ConfigLoaderTests COMMAND ConfigLoaderTests)
    set_tests_properties(ConfigLoaderTests PROPERTIES
        ENVIRONMENT_MODIFICATION "${QT_TEST_PATH_MODIFICATIONS}"
    )

    add_executable(MainWindowSmokeTests
        tests/MainWindowSmokeTest.cpp
        src/AppItem.h
        src/ConfigLoader.h
        src/ConfigLoader.cpp
        src/Launcher.h
        src/Launcher.cpp
        src/MainWindow.h
        src/MainWindow.cpp
        resources/resources.qrc
    )
    target_link_libraries(MainWindowSmokeTests PRIVATE ${QT_PACKAGE}::Widgets ${QT_PACKAGE}::Test)
    target_include_directories(MainWindowSmokeTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(MainWindowSmokeTests PRIVATE
        MULTISTART_TEST_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/apps.json"
    )
    add_test(NAME MainWindowSmokeTests COMMAND MainWindowSmokeTests)
    set_tests_properties(MainWindowSmokeTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;MULTISTART_SMOKE_SCREENSHOT=${CMAKE_BINARY_DIR}/multistart-smoke.png"
        ENVIRONMENT_MODIFICATION "${QT_TEST_PATH_MODIFICATIONS}"
    )

    add_executable(LauncherTests
        tests/LauncherTest.cpp
        src/AppItem.h
        src/Launcher.h
        src/Launcher.cpp
    )
    target_link_libraries(LauncherTests PRIVATE ${QT_PACKAGE}::Core ${QT_PACKAGE}::Gui ${QT_PACKAGE}::Test)
    target_include_directories(LauncherTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME LauncherTests COMMAND LauncherTests)
    set_tests_properties(LauncherTests PROPERTIES
        ENVIRONMENT_MODIFICATION "${QT_TEST_PATH_MODIFICATIONS}"
    )
endif()

install(TARGETS MultiStart CheckAutoUpdate RUNTIME DESTINATION bin)
install(FILES apps.json DESTINATION bin)
install(DIRECTORY scripts data DESTINATION bin)
install(FILES README.md DESTINATION .)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_RUNTIME_DIR}"
        NO_DEFAULT_PATH
        REQUIRED
    )
    install(CODE "
        set(MULTISTART_INSTALL_BIN \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin\")
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --release
                    --no-translations
                    --dir \"\${MULTISTART_INSTALL_BIN}\"
                    \"\${MULTISTART_INSTALL_BIN}/MultiStart.exe\"
                    \"\${MULTISTART_INSTALL_BIN}/CheckAutoUpdate.dll\"
            RESULT_VARIABLE MULTISTART_DEPLOY_RESULT
        )
        if(NOT MULTISTART_DEPLOY_RESULT EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed with exit code \${MULTISTART_DEPLOY_RESULT}\")
        endif()
    ")
endif()
