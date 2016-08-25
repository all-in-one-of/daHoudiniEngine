request_dependency(cyclops)

#directory for HAPI, reads the $HFS variable
set(HOUDINI_BASE $ENV{HFS} CACHE PATH "Set to Houdini base directory (/opt/hfs.x.y.z)")

# if the base exists, compile against HEngine
if (HOUDINI_BASE)
	set(DA_ENABLE_HENGINE true CACHE BOOL "Enables compiling against Houdini Engine")
else()
	message(WARNING "Could not find Houdini base directory. Set this manually and enable 'DA_ENABLE_HENGINE'")
	set(DA_ENABLE_HENGINE false CACHE BOOL "Enables compiling against Houdini Engine")
endif()

set(MODULE_NAME daHEngine)

if (DA_ENABLE_HENGINE)
	include_directories(${HOUDINI_BASE}/toolkit/include)
	include_directories(.)
	include_directories(${OSG_INCLUDES})
	link_directories(${HOUDINI_BASE}/dsolib)

else()
	include_directories(.)
	include_directories(${OSG_INCLUDES})
endif()

link_directories(./)

if (DA_ENABLE_HENGINE)
	add_subdirectory(examples/daOsgHEngine)
	add_subdirectory(examples/daHoudiniEngine)
endif()

add_library(${MODULE_NAME} MODULE
	houdiniGeometry.cpp
	daHEngine.cpp
	loaderTools.cpp
	daPly/ReaderWriterPLY.cpp
    daPly/vertexData.cpp
    daPly/plyfile.cpp
)

if (DA_ENABLE_HENGINE)
	target_link_libraries(${MODULE_NAME}
		omega
		omegaToolkit
		omegaOsg
		HAPIL
	)
else()
	target_link_libraries(${MODULE_NAME}
		omega
		omegaToolkit
		omegaOsg
	)
endif()

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
