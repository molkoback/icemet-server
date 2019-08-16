cmake_minimum_required(VERSION 3.9)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")

project(icemet_server)
set(ICEMET_VERSION "1.3.0")

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(MariaDBConnector REQUIRED)
find_package(Yaml-cpp REQUIRED)

# Sources
set(ICEMET_SRC
	icemet/worker.cpp
	
	icemet/core/config.cpp
	icemet/core/database.cpp
	icemet/core/file.cpp
	icemet/core/math.cpp
	
	icemet/util/log.cpp
	icemet/util/strfmt.cpp
	icemet/util/time.cpp
)
set(ICEMET_SERVER_SRC
	server/main.cpp
	
	server/analysis.cpp
	server/preproc.cpp
	server/reader.cpp
	server/recon.cpp
	server/saver.cpp
	server/stats.cpp
	server/watcher.cpp
	
	${ICEMET_SRC}
)

# Executables
set(EXECUTABLE_OUTPUT_PATH "bin")
set(ICEMET_SERVER_BIN icemet-server)
add_executable(${ICEMET_SERVER_BIN} ${ICEMET_SERVER_SRC})

# Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall -Wextra -pipe")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og -ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -lm")
add_compile_definitions(ICEMET_VERSION=\"${ICEMET_VERSION}\")

# Includes
include_directories(
	.
	${OpenCV_INCLUDE_DIRS}
	${MARIADB_CONNECTOR_INCLUDE_DIR}
	${YAML_CPP_INCLUDE_DIR}
)

# Libraries
set(LIBS
	stdc++fs
	Threads::Threads
	${OpenCV_LIBRARIES}
	${MARIADB_CONNECTOR_LIBRARIES}
	${YAML_CPP_LIBRARIES}
)
target_link_libraries(${ICEMET_SERVER_BIN} ${LIBS})
