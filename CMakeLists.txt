cmake_minimum_required(VERSION 3.25)

project(ICEMET_SERVER)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FMT REQUIRED fmt)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
pkg_check_modules(LIBMARIADB REQUIRED libmariadb)
pkg_check_modules(OPENCV REQUIRED opencv4)
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	${FMT_INCLUDE_DIRS}
	${LIBARCHIVE_INCLUDE_DIRS}
	${OPENCV_INCLUDE_DIRS}
	${LIBMARIADB_INCLUDE_DIRS}
	${YAMLCPP_INCLUDE_DIRS}
)

set(LIBICEMET_SRC
	icemet/database.cpp
	icemet/file.cpp
	icemet/hologram.cpp
	icemet/icemet.cpp
	icemet/img.cpp
	icemet/math.cpp
	icemet/pkg.cpp
	icemet/util/log.cpp
	icemet/util/time.cpp
	icemet/util/version.cpp
)
set(ICEMET_SERVER_SRC
	server/analysis.cpp
	server/config.cpp
	server/main.cpp
	server/preproc.cpp
	server/reader.cpp
	server/recon.cpp
	server/saver.cpp
	server/server.cpp
	server/stats.cpp
	server/watcher.cpp
	server/worker.cpp
)

execute_process(
	COMMAND git rev-parse --short HEAD
	OUTPUT_VARIABLE ICEMET_SERVER_COMMIT
	OUTPUT_STRIP_TRAILING_WHITESPACE
	COMMAND_ERROR_IS_FATAL ANY
)
message("-- Commit ${ICEMET_SERVER_COMMIT}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -pipe -Wall -Wextra -Wfatal-errors -DICEMET_SERVER_COMMIT=\\\"${ICEMET_SERVER_COMMIT}\\\"")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og -ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")

set(LIBICEMET_NAME icemet)
set(ICEMET_SERVER_NAME icemet-server)
add_library(${LIBICEMET_NAME} SHARED ${LIBICEMET_SRC})
add_executable(${ICEMET_SERVER_NAME} ${ICEMET_SERVER_SRC})

target_link_libraries(${LIBICEMET_NAME}
	${FMT_LIBRARIES}
	${LIBARCHIVE_LIBRARIES}
	${LIBMARIADB_LIBRARIES}
	${OPENCV_LIBRARIES}
	${YAMLCPP_LIBRARIES}
)
target_link_libraries(${ICEMET_SERVER_NAME}
	${LIBICEMET_NAME}
)

execute_process(
	COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/create-opencl-headers.py ${CMAKE_SOURCE_DIR}/opencl ./opencl
	COMMAND_ERROR_IS_FATAL ANY
)
