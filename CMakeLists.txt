cmake_minimum_required(VERSION 3.9)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")

project(icemet_server)
set(ICEMET_VERSION "1.2.0")

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(MariaDBConnector REQUIRED)
find_package(Yaml-cpp REQUIRED)

# Sources
set(ICEMET_SRC
	src/main.cpp
	
	src/analysis.cpp
	src/preproc.cpp
	src/reader.cpp
	src/recon.cpp
	src/saver.cpp
	src/stats.cpp
	src/watcher.cpp
	src/worker.cpp
	
	src/core/config.cpp
	src/core/database.cpp
	src/core/datetime.cpp
	src/core/file.cpp
	src/core/math.cpp
	
	src/util/log.cpp
	src/util/strfmt.cpp
	src/util/time.cpp
)

# Executables
set(EXECUTABLE_OUTPUT_PATH "bin")
set(ICEMET_SERVER_BIN icemet-server)
add_executable(${ICEMET_SERVER_BIN} ${ICEMET_SRC})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall -Wextra -pipe")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og -ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -lm")
add_compile_definitions(ICEMET_VERSION=\"${ICEMET_VERSION}\")

include_directories(
	src
	${OpenCV_INCLUDE_DIRS}
	${MARIADB_CONNECTOR_INCLUDE_DIR}
	${YAML_CPP_INCLUDE_DIR}
)

target_link_libraries(${ICEMET_SERVER_BIN}
	stdc++fs
	Threads::Threads
	${OpenCV_LIBRARIES}
	${MARIADB_CONNECTOR_LIBRARIES}
	${YAML_CPP_LIBRARIES}
)
