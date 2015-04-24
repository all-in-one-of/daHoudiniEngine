# Source files
SET( srcs
        daHoudiniEngine.cpp
        )

#######################################################################################################################
# Headers
SET( headers
	../../HAPI_CPP.h
        )

#######################################################################################################################
# Setup compile info

add_executable(daHoudiniEngine ${srcs} ${headers})
target_link_libraries(daHoudiniEngine
	omega
	cyclops
	HAPI
)
set_target_properties(daHoudiniEngine PROPERTIES PREFIX "")

# # need to add some compiler flags
IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES(daHoudiniEngine PROPERTIES COMPILE_FLAGS "-fPIC -m64")
# 	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldl -lrt -pthread")
# 	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ldl -lrt -pthread")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Bsymbolic")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Bsymbolic")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
