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
        print "Parameter {}: {}".format(parm, myParms[parm])

    csvPath = "/local/examples/hsdh/HSDH_example.csv"

    # examples of getting/setting parms
    print "Setting maxrows and filename.."
    he.setParameterValue('cluster1', 'maxrows', 1)
    he.setParameterValue('cluster1', 'filename', csvPath)

    # set parameters based on menu items
    he.cook()
    #choice lists can be set by index (value) or by Label
    # multiple parameters can be set at the same time from a dict
    he.setParameterValues('cluster1', {
        'nVal': 2, # 'LGA Name'
        'xVal': 9, # 'Total Contracts'
        'yVal': 'Population', # 10
        'zVal': 'SEIFA Index', # 3
        'cVal': 3 # SEIFA
    })
    print "Column choices are", he.getParameterChoices('cluster1', 'nVal')

    # add a parm instance!
    he.insertMultiparmInstance('cluster1', 'colRamp', 2)

    # set colours!
    print "Setting colours.."
    he.setParameterValues('cluster1', {
        'tUsePointCols': 0,
        'colRamp1c': [0.8,0.1,0.1], # Red
        'colRamp2c': [0.1,0.8,0.1], # Green
        'colRamp2pos': 0.5,
        'colRamp3c': [0.1,0.1,0.8], # Blue
        'colRamp3pos': 1.0,
        'maxrows': 25
    })

    # cook all the data (could take some time)
    print "Using all rows"
    # False tells Houdini Engine not to cook on parm set, default is True
    he.setParameterValue('cluster1', 'usemaxrows', 0, False)

    # cook all assets
    print "Cooking.."
    he.cook()

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
