#include "daHoudiniEngine/houdiniAsset.h"

using namespace houdiniEngine;

///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniAsset* HoudiniAsset::create(const String& modelName)
{
	return new HoudiniAsset(SceneManager::instance(), modelName);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniAsset::HoudiniAsset(SceneManager* scene, const String& modelName):
	Entity(scene)
{
	myModel = scene->getModel(modelName);
	if(myModel != NULL && myModel->nodes.size() > 0)
	{
		initialize(myModel->nodes[0]);

		// use getPiece() on listPieces() items to break asset up?
		// set materials based on node names..
	}
	else
	{
		ofwarn("HoudiniAsset::HoudiniAsset: could not create static object: model not found - %1%", %modelName);
	}
}

void HoudiniAsset::getCounts() {
	// for (int i = 0; i < myModel->getObjectCount(); i++) {
	// 	for (int j = 0; j < myModel->getGeoCount(i); j++) {
	// 		for (int k = 0; j < myModel->getPartCount(j, i); j++) {
	// 			ofmsg("%1%:%2%:%3%", %i %j %k);
	// 		}
	// 	}
	// }
}
