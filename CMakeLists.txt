cmake_minimum_required(VERSION 3.7)
project(ThermalPrinter)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CMakeModules/)

# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-Ofast")

set(executable output)
set(SrcDIR ./src)
set(IncludeDIR ./include)

# Setting resource path
add_definitions(-D__RESOURCES="${CMAKE_SOURCE_DIR}/resources/")
message(${CMAKE_SOURCE_DIR}/resources)

set(THREADS_PREFER_PTHREAD_FLAD ON)
find_package(Threads REQUIRED)

if (WIN32)
	include_directories(${executable}
	)
endif (WIN32)

if (UNIX)
	include_directories(${executable}
	)
endif (UNIX)

include_directories(${executable}
	${IncludeDIR}
)

file(GLOB SourceFiles 
	${SrcDIR}/*
)

add_executable(${executable} ${SourceFiles})

set_target_properties(${executable} PROPERTIES
	CXX_STANDARD 20
	CXX_EXTENSIONS ON
)

if (WIN32)
	target_link_libraries(${executable}
		${WinSDK}
	)
endif (WIN32)

if (UNIX)
	target_link_libraries(${executable}
	)
endif (UNIX)

target_link_libraries(${executable}
	${CMAKE_DL_LIBS}
)
