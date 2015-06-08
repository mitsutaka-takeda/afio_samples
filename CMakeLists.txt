###############################################################
#  ACTIVATE c++14
#
#  Uses: ACTIVATE_CPP14()
#        ACTIVATE_CPP14(TARGET) # Default mode PUBLIC
#        ACTIVATE_CPP14(MODE TARGET)
###################################################
macro(ACTIVATE_CPP14)
    set (extra_macro_args ${ARGN})
    # Did we get any optional args?
    list(LENGTH extra_macro_args num_extra_args)
    if (${num_extra_args} EQUAL 0) # No target, just compile flags
        MESSAGE("Activating C++14 flags")
        IF(APPLE)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -stdlib=libc++")
        ELSEIF (WIN32 OR UNIX)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
        ENDIF(APPLE)
    elseif(${num_extra_args} EQUAL 1)
        list(GET extra_macro_args 0 target)
        MESSAGE("Activating C++14 for PUBLIC target '${target}'")
        IF(APPLE)
          target_compile_options(${target} PUBLIC -std=c++1y -stdlib=libc++)
        ELSEIF (WIN32 OR UNIX)
          target_compile_options(${target} PUBLIC -std=c++1y)
        ENDIF(APPLE)    
    elseif (${num_extra_args} EQUAL 2)
        list(GET extra_macro_args 0 mode)
        list(GET extra_macro_args 1 target)
        MESSAGE("Activating C++14 for ${mode} target '${target}'")
        IF(APPLE)
          target_compile_options(${target} ${mode} -std=c++1y -stdlib=libc++)
        ELSEIF (WIN32 OR UNIX)
          target_compile_options(${target} ${mode} -std=c++1y)
        ENDIF(APPLE)      
    endif()
endmacro(ACTIVATE_CPP14)

# Yes, INIT_BIICODE_BLOCKS() is no longer necessary

# ADD_BIICODE_TARGETS()
ADD_BII_TARGETS()

INCLUDE(biicode/cmake/tools) # Include tools.cmake file from cmake block from the "biicode" user
ACTIVATE_CPP14(INTERFACE ${BII_BLOCK_TARGET})

message(STATUS "${CMAKE_CXX_FLAGS}")
REMOVE(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "-std=c++11")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
message(STATUS "${CMAKE_CXX_FLAGS}")
#Include biicode Boost setup
INCLUDE(biicode/boost/setup)

BII_FIND_BOOST(COMPONENTS system thread filesystem REQUIRED)
# BII_FIND_BOOST(COMPONENTS system thread REQUIRED)

TARGET_INCLUDE_DIRECTORIES(${BII_BLOCK_TARGET} INTERFACE ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${BII_BLOCK_TARGET} INTERFACE ${Boost_LIBRARIES})

TARGET_INCLUDE_DIRECTORIES(${BII_BLOCK_TARGET} INTERFACE /home/mtakeda/src/cpp/boost.afio/include/)
TARGET_INCLUDE_DIRECTORIES(${BII_BLOCK_TARGET} INTERFACE /home/mtakeda/src/cpp/boost.afio/include/boost/afio)

FIND_PACKAGE (Threads)
TARGET_LINK_LIBRARIES (${BII_BLOCK_TARGET} INTERFACE ${CMAKE_THREAD_LIBS_INIT})

TARGET_LINK_LIBRARIES (${BII_BLOCK_TARGET} INTERFACE dl)
