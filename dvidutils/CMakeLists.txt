cmake_minimum_required(VERSION 3.1)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

SET(PROJECT_NAME "dvidutils")
project(dvidutils)

include(CheckCXXCompilerFlag)

#-------------------------------------------------------------------------------------------------------------------
# check for c++ 14 support
#-------------------------------------------------------------------------------------------------------------------

# FIXME c++14 checks are broken???
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)
    #CHECK_CXX_COMPILER_FLAG("-std=c++11" HAS_CPP11_FLAG)
    if (HAS_CPP14_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else()
        message(FATAL_ERROR "Unsupported compiler -- multi-array  requires C++14 support!")
    endif()
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()


#-------------------------------------------------------------------------------------------------------------------
# include include files
#-------------------------------------------------------------------------------------------------------------------
#file(GLOB_RECURSE headers include/*.hxx)
include_directories(src)


# Support deduplication in draco encoding.
add_definitions("-DDRACO_ATTRIBUTE_DEDUPLICATION_SUPPORTED=1")

#-------------------------------------------------------------------------------------------------------------------
# Setup python bindings
#-------------------------------------------------------------------------------------------------------------------
set(CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX} CACHE PATH "")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_PREFIX_PATH}/share/cmake/pybind11")

include(pybind11Tools)
include_directories("${CMAKE_PREFIX_PATH}/include")

link_directories("${CMAKE_PREFIX_PATH}/lib")

# numpy
set(PYBIND11_CPP_STANDARD -std=c++14)
find_package(NumPy REQUIRED)
include_directories(${NUMPY_INCLUDE_DIRS})

# boost
FIND_PACKAGE(Boost REQUIRED COMPONENTS container)
include_directories(AFTER ${Boost_INCLUDE_DIR})

#-------------------------------------------------------------------------------------------------------------------
# Add the package
#-------------------------------------------------------------------------------------------------------------------

set(DVIDUTILS_BUILD_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(DVIDUTILS_PACKAGE "${DVIDUTILS_BUILD_OUTPUT_DIR}/dvidutils")

# Target for the compiled module -- install to <build-dir>/dvidutils/_dvidutils.XXX
pybind11_add_module(_dvidutils src/destripe.cpp src/main.cpp) # Also locates Python, so we can use e.g. PYTHON_SITE_PACKAGES

# Link to draco
# NOTE: By default, draco doesn't build shared libs, but our conda recipe for it does.
#       If we ever switch to conda-forge's package for draco, we'll have to change this to the static libs.
if (APPLE)
	target_link_libraries(_dvidutils PRIVATE libdraco.dylib libdracoenc.dylib libdracodec.dylib)
else()
	target_link_libraries(_dvidutils PRIVATE libdraco.so libdracoenc.so libdracodec.so)
endif()

set_target_properties(_dvidutils PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${DVIDUTILS_PACKAGE}")

# Target to copy the python sources to the build output
add_custom_target(dvidutils
     ALL
     COMMAND mkdir -p "${DVIDUTILS_PACKAGE}"
     COMMAND cp -R "${CMAKE_SOURCE_DIR}/dvidutils/*" "${DVIDUTILS_PACKAGE}/"
)
add_dependencies(dvidutils _dvidutils)


##
## Destripe test utility -- not processed by 'make install'
##
add_executable(destripe_main EXCLUDE_FROM_ALL src/destripe_main.cpp src/destripe.cpp src/pngutils.cpp)
target_link_libraries(destripe_main PRIVATE libpng.dylib)


#
# Tests
#
find_program(PYTEST_EXECUTABLE pytest)
add_custom_target(pytests
     COMMAND "PYTHONPATH=${DVIDUTILS_BUILD_OUTPUT_DIR}" "${PYTEST_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests"
)
add_dependencies(pytests dvidutils)

#
# INSTALL
#
# python sources were already copied into the build output,
# so just copy the entire build output to the install dir
install(DIRECTORY "${DVIDUTILS_PACKAGE}" DESTINATION "${PYTHON_SITE_PACKAGES}")

