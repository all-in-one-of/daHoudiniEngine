import os

from cyclops import *
from daHEngine import *
from omegaToolkit import *

ui = UiModule.createAndInitialize().getUi()

# init stuff
he = HoudiniEngine.createAndInitialize()

he.setLoggingEnabled(False)

# otl examples
# (Otl filename, otl object name, geometry name)

# otl path relative to script location
baseDir = os.path.realpath('../../otl') + "/"

hg = None

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

# cluster example
asset = createHG("cluster.otl", "Object/cluster", "cluster1")
asset.setPosition(-1, 2, -5)

light = Light.create()
light.setPosition(0, 4, 0)

e = None

# my houdini geometry counts
if hg != None:
    print hg.getObjectCount()
    for i in range(hg.getObjectCount()):
        print "", hg.getGeodeCount(i)
        for j in range(hg.getGeodeCount(i)):
            print " ", hg.getDrawableCount(j, i)

def onEvent():
    global e
    if getEvent().getType() == EventType.Down:
        e = getEvent()
        print e

print "loaded everything, running.."
