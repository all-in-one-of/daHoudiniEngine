Houdini engine module for Omegalib

requires osg, (cyclops ?)

requires houdini engine v1.9+ in houdini 14.201+

daHoudiniEngine.cpp:
this is an example module showing the loading of an otl, display of geometry and modification of parameters
run like this:

./daHoudiniEngine -o /path/to/otl-to-load.otl

daOsgHEngine.cpp:
this is an example module using OpenSceneGraph to display houdini engine geometry.
run like this:

./daOsgHEngine -o /path/to/otl-to-load.otl

switch_asset.otl:
an asset which has multiple geos displayed based on a switch

samples:
from houdini, some of the samples from the documentation

