cmake_minimum_required(VERSION 3.9)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMake/)

project(ICEMET_SERVER)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

find_package(Threads REQUIRED)
find_package(LibArchive REQUIRED)
find_package(OpenCV REQUIRED)
find_package(MariaDBConnector REQUIRED)
find_package(Yaml-cpp REQUIRED)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	${LibArchive_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIRS}
	${MARIADB_CONNECTOR_INCLUDE_DIR}
	${YAML_CPP_INCLUDE_DIR}
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
	icemet/util/strfmt.cpp
	icemet/util/time.cpp
	icemet/util/version.cpp
)
set(LIBICEMETSERVER_SRC
	server/analysis.cpp
	server/config.cpp
	server/preproc.cpp
	server/reader.cpp
	server/recon.cpp
	server/saver.cpp
	server/server.cpp
	server/stats.cpp
	server/watcher.cpp
	server/worker.cpp
)
set(ICEMETSERVER_SRC
	server/main.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall -Wextra -pipe")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og -ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

set(LIBICEMET_NAME icemet)
set(LIBICEMETSERVER_NAME icemetserver)
set(ICEMETSERVER_NAME icemet-server)
add_library(${LIBICEMET_NAME} SHARED ${LIBICEMET_SRC})
add_library(${LIBICEMETSERVER_NAME} SHARED ${LIBICEMETSERVER_SRC})
add_executable(${ICEMETSERVER_NAME} ${ICEMETSERVER_SRC})

target_link_libraries(${LIBICEMET_NAME}
	stdc++fs
	Threads::Threads
	${LibArchive_LIBRARIES}
	${MARIADB_CONNECTOR_LIBRARIES}
	${OpenCV_LIBRARIES}
	${YAML_CPP_LIBRARIES}
)
target_link_libraries(${LIBICEMETSERVER_NAME}
	${LIBICEMET_NAME}
)
target_link_libraries(${ICEMETSERVER_NAME}
	${LIBICEMETSERVER_NAME}
)

execute_process(
	COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/create-opencl-headers.py ${CMAKE_SOURCE_DIR}/opencl ./opencl
)
