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
	}
	else
	{
		ofwarn("HoudiniAsset::HoudiniAsset: could not create static object: model not found - %1%", %modelName);
	}
}
