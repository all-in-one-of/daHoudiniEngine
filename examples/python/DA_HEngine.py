from cyclops import *
from daHEngine import *
from omegaToolkit import *

# spaceNav control
import DA_spaceNav

# Change these lines for initial control sensitivities
rotSensitivity = 0.4
transSensitivity = 0.1
# Change these lines for initial position and orientation of the object
initPos = Vector3(0, 0, 0)
#initRot = quaternionFromEulerDeg(30, 30, 30)
initRot = quaternionFromEulerDeg(0, 0, 0)
rotOffset = quaternionFromEulerDeg(0, 0, 0)

cam = getDefaultCamera()

import os
# otl path relative to script location
baseDir = os.path.realpath('../../otl') + "/"

# init stuff
he = HoudiniEngine.createAndInitialize()
he.setLoggingEnabled(False)
co = CookOptions()
co.splitGeosByGroup = True
he.setCookOptions(co)
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
    # materials test
    ("mat_test.otl", "Object/mat_test", "mat_test1"),
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

#asset = createHG("empty.otl", "Object/empty", "empty1") # empty example
#asset.setPosition(-1, 2, -5)
#hg = he.getHG("empty1")

#asset = createHG(*examples[-5]) # points example
#asset.setPosition(-1, 2, -5)
#hg = he.getHG("points1")

#asset = createHG(*examples[-4]) # text example
#asset.setPosition(-1, 2, -5)
#hg = he.getHG("text1")

asset = createHG(*examples[-1]) # 3d graph example
asset.setPosition(-1, 2, -5)
hg = he.getHG("axis3D1")

# asset = createHG(*examples[-1]) # materials
# asset.setPosition(0, 2, -5)
# hg = he.getHG("mat_test1")
# he.setParameterValue('mat_test1', 'keepgroups', 1)
# he.setParameterValue('mat_test1', 'multimat', 1)

# asset = createHG(*examples[-2]) # cluster example
# asset.setPosition(-1, 2, -5)
# hg = he.getHG("cluster1")

#asset = createHG(*examples[5]) # spaceship
#asset.setPosition(0, 2, -5)
#hg = he.getHG("spaceship1")

#asset = createHG(*examples[8]) # wheel (not totally working)

#asset = createHG(*examples[9]) # curve

light = Light.create()
light.setPosition(0, 4, 0)

# my houdini geometry counts
if hg != None:
    print hg.getObjectCount()
    for i in range(hg.getObjectCount()):
        print "",hg.getGeodeCount(i)
        for j in range(hg.getGeodeCount(i)):
            print " ", hg.getDrawableCount(j, i)

def onEvent():
    if getEvent().isKeyUp(ord('x')) or getEvent().isKeyUp(ord('X')):
        he.setStringParameterValue("cluster1", 37, 0, "/local/examples/hsdh/HSDH_example.csv")

print "loaded everything, running.."
setEventFunction(onEvent)

## On run
if __name__ == "__main__":
	DA_spaceNav.obj = cam
	DA_spaceNav.rotSensitivity = rotSensitivity
	DA_spaceNav.transSensitivity = transSensitivity 
	DA_spaceNav.initPos = initPos
	DA_spaceNav.initRot = initRot
	# added for mLab 28Sep15
	DA_spaceNav.pivotOffset = Vector3(0,-2,0)
	DA_spaceNav.rotOffset = rotOffset
	setEventFunction(DA_spaceNav.onEvent)
	setUpdateFunction(DA_spaceNav.onUpdate)
