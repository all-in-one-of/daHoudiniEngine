request_dependency(cyclops)

#directory for HAPI
include_directories(/opt/hfs14.0.201.13/toolkit/include)
include_directories(.)
link_directories(/opt/hfs14.0.201.13/dsolib)
link_directories(./)

add_subdirectory(examples/daOsgHEngine)
add_subdirectory(examples/daHoudiniEngine)
