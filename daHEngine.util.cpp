/******************************************************************************
Houdini Engine Module for Omegalib

Authors:
  Darren Lee             darren.lee@uts.edu.au

Copyright 2015-2016,     Data Arena, University of Technology Sydney
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and authors, and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the Data Arena Project.

-------------------------------------------------------------------------------

daHEngine
	module to display geometry from Houdini Engine in omegalib
	this file contains utility-type functions used in HEngine

******************************************************************************/

#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>

using namespace houdiniEngine;

void HoudiniEngine::setLoggingEnabled(const bool toggle) {
	myLogEnabled = toggle;
}

float HoudiniEngine::getFps()
{
	if (SystemManager::instance()->isMaster()) {
		HAPI_TimelineOptions to;
		HAPI_GetTimelineOptions(session, &to);

		return to.fps;
	}

	return 0.0;
}

// TODO: distribute value to slaves
float HoudiniEngine::getTime()
{
	float myTime = -1.0;

	if (SystemManager::instance()->isMaster()) {
		HAPI_GetTime(session, &myTime);
	}

	return myTime;
}

void HoudiniEngine::setTime(float time)
{
	if (SystemManager::instance()->isMaster()) {
		HAPI_SetTime(session, time);
	}
}

// cook everything
void HoudiniEngine::cook()
{
	if (!SystemManager::instance()->isMaster()) {
		return;
	}

	foreach(Mapping::Item asset, instancedHEAssets) {
        cook_one(asset.second);
	}
}

void HoudiniEngine::cook_one(hapi::Asset* asset) 
{
    if (asset != NULL) {

        ofmsg("cooking %1%..", %asset->name());

        asset->cook();
        wait_for_cook();

        process_assets(*asset);

        updateGeos = true;
    }
}

void HoudiniEngine::showMappings() {
	String asset_name = "SideFX::Object/spaceship";
	hapi::Asset* myAsset = instancedHEAssetsByName[asset_name];

	if (myAsset == NULL) {
		ofwarn("No asset of name %1%", %asset_name);
		return;
	}

// 	typedef Dictionary < int, int >::iterator myIt;
// 
// 	for (myIt item = widgetIdToParmId.begin(); item != widgetIdToParmId.end(); ++item) {
// // 		ofmsg("%1%: %2%", %item->first %item->second);
// // 		Widget* myWidget = Widget::mysWidgets[item->first];
// 		hapi::Parm* parm = &(myAsset->parms()[widgetIdToParmId[item->first]]);
// 		ofmsg("%1%: %2%", %item->first %parm->name());
// 	}
}

Container* HoudiniEngine::getContainerForAsset(int n){
	if (n < assetConts.size()) {
		return assetConts[n];
	}
	return NULL;
}
