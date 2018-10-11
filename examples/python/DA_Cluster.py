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
hg = None

ui = UiModule.createAndInitialize()
wf = ui.getWidgetFactory()
uiroot = ui.getUi()

# 3d Cluster Example (cluster.otl)
# load Houdini Digital Asset
he.loadAssetLibraryFromFile(baseDir + "cluster.otl")
# load particular asset by name
he.instantiateAsset("Object/cluster")
# create HoudiniAsset
asset = he.instantiateGeometry("cluster1") 
asset.setPosition(-4, -3, -35)

# create a houdiniGeometry (equivalent of ModelGeometry)
# hg = he.getHG("cluster1")

# make a light in the scene
light = Light.create()
light.setPosition(0, 4, 20)

print "loaded everything, running.."

# list all the parameters for this asset
if isMaster():
    myParms = he.getParameters('cluster1')
    for parm in myParms.keys():
        print parm, myParms[parm]

    # examples of getting/setting parms
    he.setParameterValue('cluster1', 'filename', "/local/examples/hsdh/HSDH_example.csv")
    he.setParameterValue('cluster1', 'maxrows', 1)
    he.setParameterValue('cluster1', 'tShowMetaball', 0)

    # set parameters based on menu items
    print he.getParameterChoices('cluster1', 'nVal')
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
