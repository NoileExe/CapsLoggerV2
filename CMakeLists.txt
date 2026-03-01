cmake_minimum_required(VERSION 3.16)

project(CapsLoggerV2
	VERSION 1.0
	LANGUAGES CXX
)

# Требуем C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


option(STATIC_RUNTIME "Использовать статическую библиотеку времени выполнения (CRT)" OFF)

if(MSVC)
	if(STATIC_RUNTIME)
		# Современный способ (CMake ≥3.15)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	else()
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	endif()
elseif(MINGW)
	# Для MinGW (GCC) можно принудительно статически прилинковать libstdc++ и libgcc
	if(STATIC_RUNTIME)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
	endif()
elseif(UNIX AND NOT APPLE)
	# Для Linux с GCC/Clang (обычно и так статика, но можно добавить -static-libstdc++ при желании)
	if(STATIC_RUNTIME)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
	endif()
endif()

if(NOT DEFINED IWYU_PATH)
	find_program(IWYU_PATH include-what-you-use)
endif()

if(IWYU_PATH)
	message(STATUS "IWYU enabled: ${IWYU_PATH}")

	if(QT_VERSION EQUAL 6)
		set(QT_IMAP_FILE "${CMAKE_SOURCE_DIR}/qt6.imp")
	else()
		set(QT_IMAP_FILE "${CMAKE_SOURCE_DIR}/qt5.imp")
	endif()

	set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
		"${IWYU_PATH}"
		"-Xiwyu;--mapping_file=${QT_IMAP_FILE}"
		"-Xiwyu;--no_fwd_decls"
		"-Xiwyu;--verbose=3"
	)
else()
	message(STATUS "IWYU not found - skipping")
endif()

# ========================================== НАСТРОЙКА QT ==========================================
# Явное указание версии через -DQT_VERSION=5 или -DQT_VERSION=6
if(DEFINED QT_VERSION)
	message(STATUS "Using explicitly specified Qt version: ${QT_VERSION}")
	if(QT_VERSION EQUAL 6)
		find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)
		if(UNIX AND NOT APPLE)
			find_package(Qt6 REQUIRED COMPONENTS DBus)
		endif()
	elseif(QT_VERSION EQUAL 5)
		find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)
		if(UNIX AND NOT APPLE)
			find_package(Qt5 REQUIRED COMPONENTS DBus X11Extras)
		endif()
	else()
		message(FATAL_ERROR "Unsupported Qt version: ${QT_VERSION}. Use 5 or 6.")
	endif()
endif()

if(NOT DEFINED QT_VERSION)
	# Автоопределение: предпочитаем Qt6, fallback на Qt5
	find_package(Qt6 COMPONENTS Core Gui Widgets Network LinguistTools QUIET)
	if(NOT Qt6_FOUND)
		find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network LinguistTools)
		if(UNIX AND NOT APPLE)
			find_package(Qt5 REQUIRED COMPONENTS DBus X11Extras)
		endif()
		set(QT_VERSION 5)
	else()
		if(UNIX AND NOT APPLE)
			find_package(Qt6 REQUIRED COMPONENTS DBus)
		endif()
		set(QT_VERSION 6)
	endif()
endif()

# Включаем автоматическую обработку .ui, .qrc, .ts
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
# ========================================== НАСТРОЙКА QT ==========================================

# ========================================= ИСХОДНЫЕ ФАЙЛЫ =========================================
set(SRC_MAIN
	sources/main.cpp
)

set(SRC_GUI
	sources/gui/KeysLoggerTray.cpp
	sources/gui/TrayMessagePopup.cpp
	sources/gui/SettingsDefines.cpp
	sources/gui/SettingsWidget.cpp
)

set(HEADERS_GUI
	sources/gui/KeysLoggerTray.h
	sources/gui/TrayMessagePopup.h
	sources/gui/SettingsDefines.h
	sources/gui/SettingsWidget.h
)

set(SRC_PLATFORM
	sources/platform/KeysLoggerProvider.cpp
)

set(HEADERS_PLATFORM
	sources/platform/KeysLoggerProvider.h
)

set(UI_FILES
	sources/gui/SettingsWidget.ui
)

set(RESOURCES
	resources.qrc
)
# ========================================= ИСХОДНЫЕ ФАЙЛЫ =========================================

# ==================================== ПЛАТФОРМОЗАВИСИМЫЕ ФАЙЛЫ ====================================
if(WIN32)
	list(APPEND SRC_PLATFORM sources/platform/WinKeysLogger.cpp)
	list(APPEND HEADERS_PLATFORM sources/platform/WinKeysLogger.h)
	list(APPEND PLATFORM_LIBS user32)
	set(WINDOWS_RC resources/appicon.rc)
elseif(UNIX AND NOT APPLE)
	list(APPEND SRC_PLATFORM
		sources/platform/X11KeysLogger.cpp
		sources/platform/WaylandKeysLogger.cpp
	)
	list(APPEND HEADERS_PLATFORM
		sources/platform/X11KeysLogger.h
		sources/platform/WaylandKeysLogger.h
	)

	# Поиск библиотек X11 через pkg-config или напрямую
	find_package(PkgConfig QUIET)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(X11 REQUIRED x11 xkbfile xtst)
		list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})
		include_directories(${X11_INCLUDE_DIRS})
	else()
		find_package(X11 REQUIRED)
		list(APPEND PLATFORM_LIBS X11)
	endif()
endif()
# ==================================== ПЛАТФОРМОЗАВИСИМЫЕ ФАЙЛЫ ====================================

# ======================================== СБОРКА ПРИЛОЖЕНИЯ =======================================
if(QT_VERSION EQUAL 6)
    qt6_wrap_ui(UI_HEADERS ${UI_FILES})
else()
    qt5_wrap_ui(UI_HEADERS ${UI_FILES})
endif()

add_executable(${PROJECT_NAME}
	${SRC_MAIN}
	${SRC_GUI}
	${SRC_PLATFORM}
    ${UI_HEADERS}
	${HEADERS_GUI}
	${HEADERS_PLATFORM}
	${RESOURCES}
	${WINDOWS_RC}		# Windows .rc файл (если есть)
)

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sources
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Подключаем библиотеки Qt
if(QT_VERSION EQUAL 6)
	target_link_libraries(${PROJECT_NAME} PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Network
		${PLATFORM_LIBS}
	)

	if(UNIX AND NOT APPLE)
		target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::DBus)
	endif()
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
		Qt5::Network
		${PLATFORM_LIBS}
	)

	if(UNIX AND NOT APPLE)
		target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::DBus Qt5::X11Extras)
	endif()
endif()
# ======================================== СБОРКА ПРИЛОЖЕНИЯ =======================================
