from cyclops import *
from daHEngine import *
from omegaToolkit import *

import os
# otl path relative to script location
baseDir = os.path.realpath('../../otl') + "/"

# init stuff
he = HoudiniEngine.createAndInitialize()
he.setLoggingEnabled(False)
hg = None

ui = UiModule.createAndInitialize()
wf = ui.getWidgetFactory()
uiroot = ui.getUi()

def createHG(otl, assetName, geoName):
    global baseDir
    print "loading", baseDir + otl
    he.loadAssetLibraryFromFile(baseDir + otl)
    print "instantiating", assetName
    he.instantiateAsset(assetName)
    print "creating geometry", geoName
    return he.instantiateGeometry(geoName)

# 3d cluster example
asset = createHG("cluster.otl", "Object/cluster", "cluster1") 
asset.setPosition(-4, -3, -35)
hg = he.getHG("cluster1")

# make a light in the scene
light = Light.create()
light.setPosition(0, 4, 20)

print "loaded everything, running.."

# list all the parameters for this asset
myParms = he.getParameters('cluster1')
for parm in myParms.keys():
    print parm, myParms[parm]

# examples of getting/setting parms
he.setParameterValue('cluster1', 'filename', "/local/examples/hsdh/HSDH_example.csv")
he.setParameterValue('cluster1', 'maxrows', 1)

# set parameters based on menu items
print he.getParameterChoices('clister1', 'nVal')
#choice lists can be set by index (value) or by Label
he.setParameterValue('cluster1', 'nVal', 2) # 'LGA Name'
he.setParameterValue('cluster1', 'xVal', 9) # 'Total Contracts'
he.setParameterValue('cluster1', 'yVal', 'Population') # 10
he.setParameterValue('cluster1', 'zVal', 'SEIFA Index') # 3
he.setParameterValue('cluster1', 'cVal', 3) # SEIFA

# add a parm instance!
he.insertMultiparmInstance('cluster1', 'colRamp', 2)

# set colours!
he.setParameterValue('cluster1', 'colRamp1c', [0.8,0.1,0.1]) # Red
he.setParameterValue('cluster1', 'colRamp2c', [0.1,0.8,0.1]) # Green
he.setParameterValue('cluster1', 'colRamp2pos', 0.5)
he.setParameterValue('cluster1', 'colRamp3c', [0.1,0.1,0.8]) # Blue
he.setParameterValue('cluster1', 'colRamp3pos', 1.0)
he.setParameterValue('cluster1', 'maxrows', 25)

# cook all the data (could take some time)
he.setParameterValue('cluster1', 'usemaxrows', 0)
