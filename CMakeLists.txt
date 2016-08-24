request_dependency(cyclops)

#directory for HAPI
set(HOUDINI_BASE ${HFS} CACHE PATH "Set to Houdini base directory (/opt/hfs.x.y.z)")

include_directories(${HOUDINI_BASE}/toolkit/include)
include_directories(.)
link_directories(${HOUDINI_BASE}/dsolib)
link_directories(./)

include_directories(${OSG_INCLUDES})

add_subdirectory(examples/daOsgHEngine)
add_subdirectory(examples/daHoudiniEngine)


set(MODULE_NAME daHEngine)

add_library(${MODULE_NAME} MODULE
	houdiniGeometry.cpp
	daHEngine.cpp
	loaderTools.cpp
	daPly/ReaderWriterPLY.cpp
    daPly/vertexData.cpp
    daPly/plyfile.cpp
)

target_link_libraries(${MODULE_NAME}
	omega
	omegaToolkit
	omegaOsg
	HAPIL
)

# # need to add some compiler flags
IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES COMPILE_FLAGS "-fPIC -m64")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Bsymbolic")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Bsymbolic")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

#------------------------------------------------------------------------------
# DO NOT MODIFY ANYTHING BELOW AFTER THIS LINE
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "")
if(WIN32)
	set_target_properties(${MODULE_NAME} PROPERTIES FOLDER modules SUFFIX ".pyd")
endif()
