from daHEngine import *
from cyclops import *
from omegaToolkit import *

# init stuff
he = HoudiniEngine.createAndInitialize()

he.setLoggingEnabled(False)

# otl examples
# (Otl filename, otl object name, geometry name)

import os
# otl path relative to script location
baseDir = os.path.realpath('../../otl') + "/"

hg = None

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

#asset = createHG(*examples[3]) # multi-geo parts

#asset = createHG("axisA1.otl", "Object/axisA1", "axisA11")
#asset.setPosition(0, 2, -5)
#hg = he.getHG("axisA11")

#asset = createHG(*examples[-4]) # text example
#asset.setPosition(-1, 2, -5)
#hg = he.getHG("text1")

asset = createHG(*examples[-1]) # 3d graph example
asset.setPosition(-1, 2, -5)
hg = he.getHG("axis3D1")

# asset = createHG(*examples[-2]) # cluster example
# asset.setPosition(-1, 2, -5)
# hg = he.getHG("cluster1")

##asset = createHG(*examples[5]) # spaceship
##asset.setPosition(0, 2, -5)
##hg = he.getHG("spaceship1")

#asset = createHG(*examples[8]) # wheel (not totally working)

#asset = createHG(*examples[9]) # curve

#sp = SphereShape.create(.1, 1)
#sp.setPosition(0,2,-5)

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
