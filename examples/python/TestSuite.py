from daHEngine import *
from cyclops import *
from omegaToolkit import *

# init stuff
he = HoudiniEngine.createAndInitialize()
he.setLoggingEnabled(False)

import os
# otl path relative to script location
baseDir = os.path.realpath('../../otl') + "/"

hg = None

ui = UiModule.createAndInitialize()
wf = ui.getWidgetFactory()
uiroot = ui.getUi()

# otl examples
# (Otl filename, otl object name, geometry name)
examples = [
	# simple test of an asset param
        ("switch_asset.otl", "Object/switch_asset", "switch_asset1"),
        ("switch_anim_asset.otl", "Object/switch_anim_asset", "switch_anim_asset1"),
        ("part_asset.otl", "Object/part_asset", "part_asset1"),
	# test multiple parts and params
        ("multi_geos_parts.otl", "Object/multi_geos_parts", "multi_geos_parts1"),
        ("pos_test.otl", "Object/pos_test", "pos_test1"),
        # spaceship, has textures and python scripts
        ("Core/SideFX__spaceship.otl", "SideFX::Object/spaceship", "spaceship1"),
        ("pos_test.otl", 0, "pos_test1"),
        ("texture.otl", "Object/texture", "texture1"),
        # has custom ui elements
        ("Additional/WheelAsset.otl", "Object/WheelAsset", "WheelAsset1"),
        # Points
        ("points.otl", "Object/points1", "points11"),
        # Text
        ("text.otl", "Object/text", "text1"),
        # curves
        ("curve_object.otl", "Object/curve_object", "curve_object1"),
        # cluster otl example
        ("cluster.otl", "Object/cluster", "cluster1"),
        # multiple assets in library
        ("axisA1.otl", "Object/axis3D", "axis3D1")
]

def createHG(otl, assetName, geoName):
        global baseDir
        print "loading", baseDir + otl
        he.loadAssetLibraryFromFile(baseDir + otl)
        print "instantiating", assetName
        he.instantiateAsset(assetName)
        print "creating geometry", geoName
        return he.instantiateGeometry(geoName)

def createHGId(otl, assetIndex, geoName):
        global baseDir
        he.loadAssetLibraryFromFile(baseDir + otl)
        he.instantiateAssetById(assetIndex)
        return he.instantiateGeometry(geoName)

# Here's some ideal methods to use

asset = createHG(*examples[-2]) # cluster example
asset.setPosition(-1, 2, -5)
hg = he.getHG("cluster1")

heAsset = he.getAsset("cluster1")

myParms = he.getAsset("cluster1").getParameters()

for parm in myParms:
        print parm.getName(), parm.getType(), parm.getValue()

heAsset.setStrimgParameterValue("filename", "/da/proj/HSDH/HSDH_example.csv")

light = Light.create()
light.setPosition(0, 4, 0)

e = None

# my houdini geometry counts
if hg != None:
	print hg.getObjectCount()
	for i in range(hg.getObjectCount()):
		print "",hg.getGeodeCount(i)
		for j in range(hg.getGeodeCount(i)):
			print " ", hg.getDrawableCount(j, i)

def onEvent():
	global e
	if getEvent().getType() == EventType.Down:
		e = getEvent()
		print e

print "loaded everything, running.."
# he.loadParameters("cluster1")
# he.setStringParameterValue("cluster1", 37, 0, "/da/proj/dpiWater/data/clustering.csv")

