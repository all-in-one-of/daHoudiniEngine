from daHEngine import *
from cyclops import *
from omegaToolkit import *

# init stuff
he = HoudiniEngine.createAndInitialize()

he.setLoggingEnabled(False)

# otl examples
# (Otl filename, otl object name, geometry name)

#baseDir = "/local/omegalib/modules/daHoudiniEngine/otl/"
baseDir = "/da/dev/darren/omegalib/modules/daHoudiniEngine/otl/"

examples = [
        ("switch_asset.otl", "Object/switch_asset", "switch_asset1"),
        ("switch_anim_asset.otl", "Object/switch_anim_asset", "switch_anim_asset1"),
        ("part_asset.otl", "Object/part_asset", "part_asset1"),
        ("multi_geos_parts.otl", "Object/multi_geos_parts", "multi_geos_parts1"),
        ("pos_test.otl", "Object/pos_test", "pos_test1"),
        ("Core/SideFX__spaceship.otl", "SideFX::Object/spaceship", "spaceship1"),
        ("pos_test.otl", 0, "pos_test1"),
        ("texture.otl", "Object/texture", "texture1"),
        ("Additional/WheelAsset.otl", "Object/WheelAsset", "WheelAsset1"),
        ("curve_object.otl", "Object/curve_object", "curve_object1"),
        ("fbxTest.otl", "Object/fbxTest", "fbxTest1"),
        ("multiAsset.otl", "Object/SoftToy", "SoftToy1"),
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
#asset = createHGId(*examples[-2])

asset = createHGId("multiAsset.otl", 0, "softToy1")
he.instantiateAssetById(1)
asset2 = he.instantiateGeometry("boxy1")
#asset = createHG(*examples[-1])
##asset = createHG(*examples[5]) # spaceship
#asset = createHG(*examples[3]) # simple

#asset = createHG(*examples[8]) # wheel (not totally working)
#asset = createHG(*examples[-1]) # curve

#asset = createHG(*examples[-1]) # fbx file


sp = SphereShape.create(.1, 1)

asset.setPosition(-1, 2, -5)
asset2.setPosition(1, 2, -5)

# testing for spaceship
#asset.setEffect('textured -d /da/dev/darren/omegalib/modules/daHoudiniEngine/prp_spaceship_color_1.jpg -e white')

light = Light.create()
light.setPosition(0, 4, -5)

e = None

def onEvent():
	global e
	if getEvent().getType() == EventType.Down:
		e = getEvent()
		print e

setEventFunction(onEvent)