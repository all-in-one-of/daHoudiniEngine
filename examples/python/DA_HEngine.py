from daHEngine import *
from cyclops import *
from omegaToolkit import *

# init stuff
he = HoudiniEngine.createAndInitialize()

he.setLoggingEnabled(False)

# otl examples
# (Otl filename, otl object name, geometry name)

baseDir = "/local/omegalib/modules/daHoudiniEngine/otl/"

examples = [
        ("switch_asset.otl", "Object/switch_asset", "switch_asset1"),
        ("switch_anim_asset.otl", "Object/switch_anim_asset", "switch_anim_asset1"),
        ("part_asset.otl", "Object/part_asset", "part_asset1"),
        ("multi_geos_parts.otl", "Object/multi_geos_parts", "multi_geos_parts1"),
        ("pos_test.otl", "Object/pos_test", "pos_test1"),
        ("Core/SideFX__spaceship.otl", "SideFX::Object/spaceship", "spaceship1"),
        ("pos_test.otl", 0, "pos_test1"),
        #("Core/SideFX__spaceship.otl", 0, "spaceship1"),
        ("texture.otl", "Object/texture", "texture1"),
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

#asset = createHG(*examples[3])
#asset = createHGId(*examples[-2])

#asset = createHG(*examples[-1])
asset = createHG(*examples[3])

sp = SphereShape.create(.1, 1)

asset.setPosition(0, 2, -5)

light = Light.create()


