request_dependency(cyclops)
include_directories(${OSG_INCLUDES})

#add_library(pointCloud MODULE
#        pointCloud.cpp
#        TextPointsLoader.cpp
#        TextPointsLoader.h
#        BinaryPointsLoader.cpp
#        BinaryPointsLoader.h
#        BinaryPointsReader.cpp
#        BinaryPointsReader.h)

target_link_libraries(daHoudiniEngine omega cyclops)

set_target_properties(daHoudiniEngine PROPERTIES PREFIX "")
if(WIN32)
        set_target_properties(daHoudiniEngine PROPERTIES FOLDER modules SUFFIX ".pyd")
endif()
