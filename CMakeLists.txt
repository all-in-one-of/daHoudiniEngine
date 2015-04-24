request_dependency(cyclops)

#directory for HAPI
include_directories(/opt/hfs14.0.201.13/toolkit/include)
include_directories(.)
link_directories(/opt/hfs14.0.201.13/dsolib)
link_directories(./)

# Source files
SET( srcs
#         daHEngine.cpp
        daHoudiniEngine.cpp
        )

#######################################################################################################################
# Headers
SET( headers
# 	enginesimplehostcpp.h
	HAPI_CPP.h
        )

#######################################################################################################################
# Setup compile info

add_subdirectory(examples/daOsgHEngine)

add_executable(daHoudiniEngine ${srcs} ${headers})
# target_link_libraries(daHoudiniEngine omega cyclops HAPI simple_hapi_host)
target_link_libraries(daHoudiniEngine omega cyclops HAPI)
set_target_properties(daHoudiniEngine PROPERTIES PREFIX "")

# MESSAGE(STATUS "cmake system processor: " ${CMAKE_SYSTEM_PROCESSOR})
# MESSAGE(STATUS "python? " ${OMEGA_USE_PYTHON})
# MESSAGE(STATUS "python? " ${PYTHON_INCLUDE_DIRS})
# get_property(the_include_dirs DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
# MESSAGE(STATUS "includes?")
# foreach(dir  ${the_include_dirs})
# 	MESSAGE(STATUS ${dir})
# endforeach()
# string(REPLACE ${PYTHON_INCLUDE_DIRS} "" new_include_dirs ${the_include_dirs})
# set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${new_include_dirs})
# get_property(the_include_dirs DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
# MESSAGE(STATUS "includes now?")
# foreach(dir2  ${the_include_dirs})
# 	MESSAGE(STATUS ${dir2})
# endforeach()
# # list(REMOVE_ITEM ${the_include_dirs} ${PYTHON_INCLUDE_DIRS})
# # set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${the_include_dirs})
# # MESSAGE(STATUS "includes now?")
# # foreach(dir2  ${the_include_dirs})
# # 	MESSAGE(STATUS ${dir2})
# # endforeach()
# # need to add some compiler flags

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES(daHoudiniEngine PROPERTIES COMPILE_FLAGS "-fPIC -m64")
# 	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl -lrt -pthread")
# 	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ldl -lrt -pthread")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Bsymbolic")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Bsymbolic")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )



# request_dependency(cyclops)
# include_directories(${OSG_INCLUDES})
#
# #add_library(pointCloud MODULE
# #        pointCloud.cpp
# #        TextPointsLoader.cpp
# #        TextPointsLoader.h
# #        BinaryPointsLoader.cpp
# #        BinaryPointsLoader.h
# #        BinaryPointsReader.cpp
# #        BinaryPointsReader.h)
#
# target_link_libraries(daHoudiniEngine omega cyclops)
#
# set_target_properties(daHoudiniEngine PROPERTIES PREFIX "")
# if(WIN32)
#         set_target_properties(daHoudiniEngine PROPERTIES FOLDER modules SUFFIX ".pyd")
# endif()
