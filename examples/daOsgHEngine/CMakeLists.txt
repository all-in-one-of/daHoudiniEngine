# included from parent CMakeLists.txt
# request_dependency(cyclops)
#
# #directory for HAPI
# include_directories(/opt/hfs14.0.201.13/toolkit/include)
# link_directories(/opt/hfs14.0.201.13/dsolib)
# link_directories(./)
#

MESSAGE(STATUS ">>>>>>>>>> I've been included!!")


include_directories(${OSG_INCLUDES})

# Source files
SET( srcs
#         daHEngine.cpp
        daOsgHEngine.cpp
        )

#######################################################################################################################
# Headers
SET( headers
# 	enginesimplehostcpp.h
	../../HAPI_CPP.h
        )

#######################################################################################################################
# Setup compile info

add_executable(daOsgHEngine ${srcs} ${headers})
# target_link_libraries(daHoudiniEngine omega cyclops HAPI simple_hapi_host)
target_link_libraries(daOsgHEngine
	omega
	omegaToolkit
	omegaOsg
	cyclops
	HAPI)

set_target_properties(daOsgHEngine PROPERTIES PREFIX "")

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES(daOsgHEngine PROPERTIES COMPILE_FLAGS "-fPIC -m64")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Bsymbolic")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Bsymbolic")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
