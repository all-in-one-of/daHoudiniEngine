/**********************************************************************************************************************
 * THE OMEGA LIB PROJECT
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright 2010								Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:
 *  Alessandro Febretti							febret@gmail.com
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2010, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------------------------------------------------
 *	ohello2
 *		A slightly more complex version of ohello, implementing event handling and data sharing. ohello2 defines a
 *		new class, HelloServer, that reads mouse events and uses it to rotate the rendered object.
 *		HelloServer also implements SharedObject to synchronize rotation data across all nodes.
 *********************************************************************************************************************/

#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>

using namespace houdiniEngine;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Python wrapper code.
#ifdef OMEGA_USE_PYTHON
#include "omega/PythonInterpreterWrapper.h"
BOOST_PYTHON_MODULE(daHEngine)
{
	// HoudiniEngine
	PYAPI_REF_BASE_CLASS(HoudiniEngine)
		PYAPI_STATIC_REF_GETTER(HoudiniEngine, createAndInitialize)
 		PYAPI_METHOD(HoudiniEngine, loadAssetLibraryFromFile)
 		PYAPI_METHOD(HoudiniEngine, instantiateAsset)
 		PYAPI_METHOD(HoudiniEngine, instantiateAssetById)
 		PYAPI_REF_GETTER(HoudiniEngine, instantiateGeometry)
 		PYAPI_METHOD(HoudiniEngine, getFps)
 		PYAPI_METHOD(HoudiniEngine, getTime)
 		PYAPI_METHOD(HoudiniEngine, setTime)
 		PYAPI_METHOD(HoudiniEngine, cook)
 		PYAPI_METHOD(HoudiniEngine, setLoggingEnabled)
		;
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniEngine* HoudiniEngine::createAndInitialize()
{
	int minHoudiniVersion = 15;
	int minHEngineVersion = 2;
	int houdiniVersionMajor = 0;
	int hEngineVersionMajor = 0;

	ENSURE_SUCCESS(NULL, HAPI_GetEnvInt(HAPI_ENVINT_VERSION_HOUDINI_MAJOR, &houdiniVersionMajor ));
	ENSURE_SUCCESS(NULL, HAPI_GetEnvInt(HAPI_ENVINT_VERSION_HOUDINI_ENGINE_MAJOR, &hEngineVersionMajor ));

	if (hEngineVersionMajor < minHEngineVersion || houdiniVersionMajor < minHoudiniVersion) {
		oerror("Could not find appropriate version of Houdini or Houdini Engine");
		oferror("Houdini Version: %1% (needed %2%), Houdini Engine Version %2% (needed %4%)",
				%houdiniVersionMajor %minHoudiniVersion %hEngineVersionMajor %minHEngineVersion);
		return NULL;
	}

	// Initialize and register the HoudiniEngine module.
	HoudiniEngine* instance = new HoudiniEngine();
	ModuleServices::addModule(instance);
	instance->doInitialize(Engine::instance());
	return instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniEngine::HoudiniEngine():
	EngineModule("HoudiniEngine"),
	mySceneManager(NULL),
	myLogEnabled(false)
{
}

HoudiniEngine::~HoudiniEngine()
{
	if (SystemManager::instance()->isMaster())
	{
	    try
	    {
		    ENSURE_SUCCESS(session, HAPI_Cleanup(session));
			olog(Verbose, "done HAPI");
	    }
	    catch (hapi::Failure &failure)
	    {
			ofwarn("Houdini Failure on cleanup %1%", %failure.lastErrorMessage(session));
			throw;
	    }
	}
}

int HoudiniEngine::loadAssetLibraryFromFile(const String& otlFile)
{
    int assetCount = -1;

	// do this on master only
	if (!SystemManager::instance()->isMaster()) {
		return -1;
	}

    HAPI_Result hr = HAPI_LoadAssetLibraryFromFile(
			session,
            otlFile.c_str(),
			false, /* allow_overwrite */
            &library_id);
    if (hr != HAPI_RESULT_SUCCESS)
    {
        ofwarn("Could not load %1%", %otlFile);
        ofwarn("Result is %1%", %hr);
		return assetCount;
    }

    ENSURE_SUCCESS(session,  HAPI_GetAvailableAssetCount( session, library_id, &assetCount ) );

	ofmsg("%1% assets available", %assetCount);
    HAPI_StringHandle* asset_name_sh = new HAPI_StringHandle[assetCount];
    ENSURE_SUCCESS(session,  HAPI_GetAvailableAssets( session, library_id, asset_name_sh, assetCount ) );

	for (int i =0; i < assetCount; ++i) {
	    std::string asset_name = get_string( session, asset_name_sh[i] );
		ofmsg("asset %1%: %2%", %(i + 1) %asset_name);
	}

	delete[] asset_name_sh;

    return assetCount;

}

// TODO: generalise this so the following works:
// check for uiMin/uiMax, then use sliders
// use text boxes for vectors and non min/max things
// use submenus/containers for choices
// use the joinNext variable for displaying items
// use checkbox for HAPI_PARMTYPE_TOGGLE
// use text box for string
// do multiparms
// do menu layout

// HAPI_PARMTYPE_INT 				= 0,
// HAPI_PARMTYPE_MULTIPARMLIST,		= 1
// HAPI_PARMTYPE_TOGGLE,			= 2
// HAPI_PARMTYPE_BUTTON,			= 3
//
// HAPI_PARMTYPE_FLOAT,				= 4
// HAPI_PARMTYPE_COLOR,				= 5
//
// HAPI_PARMTYPE_STRING,			= 6
// HAPI_PARMTYPE_PATH_FILE,			= 7
// HAPI_PARMTYPE_PATH_FILE_GEO,		= 8
// HAPI_PARMTYPE_PATH_FILE_IMAGE,	= 9
// HAPI_PARMTYPE_PATH_NODE,			= 10
//
// HAPI_PARMTYPE_FOLDERLIST,		= 11
//
// HAPI_PARMTYPE_FOLDER,			= 12
// HAPI_PARMTYPE_LABEL,				= 13
// HAPI_PARMTYPE_SEPARATOR,			= 14
//
// HAPI_PARMTYPE_MAX - total supported parms = 15?

void HoudiniEngine::createMenuItem(const String& asset_name, ui::Menu* menu, hapi::Parm* parm) {
	MenuItem* mi = NULL;

	ofmsg("PARM %1%: %2% s-%3% id-%4%", %parm->label() %parm->info().type %parm->info().size %parm->info().parentId );

	if (parm->info().size == 1) {
		mi = menu->addItem(MenuItem::Label);
		mi->setText(parm->label());

		assetParams[asset_name].push_back(*mi);

		if (parm->info().choiceCount > 0) {
			ofmsg("  choiceindex: %1%", %parm->info().choiceIndex);
			ofmsg("  choiceCount: %1%", %parm->info().choiceCount);
			if (parm->info().type == HAPI_PARMTYPE_STRING ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) {
				std::string val = parm->getStringValue(0);
				mi->setText(parm->label() + ": " + ostr("%1%", %val));
			} else {
				int val = parm->getIntValue(0);
				mi->setText(parm->label() + ": " + ostr("%1%", %val));
			}

			mi->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParams[asset_name].push_back(*mi);

			ui::Menu* choiceMenu = menu->addSubMenu(parm->choices[0].label());
			for (int i = 0; i < parm->choices.size(); ++i) {
				ofmsg("  choice %1%: %2% %3%", %i %parm->choices[i].label() %parm->choices[i].value());
				MenuItem* choiceItem = choiceMenu->addItem(MenuItem::Label);
				choiceItem->setText(parm->choices[i].label());
				choiceItem->setUserData(mi); // reference to the label, for updating values
			}

		} else if (parm->info().type == HAPI_PARMTYPE_INT) {
			int val = parm->getIntValue(0);
			mi->setText(parm->label() + ": " + ostr("%1%", %val));
			mi->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParams[asset_name].push_back(*mi);
			MenuItem* miLabel = mi;
			mi = menu->addSlider(parm->info().max + 1, "");
			mi->getSlider()->setValue(val);
			mi->setUserData(miLabel); // reference to the label, for updating values

		} else if (parm->info().type == HAPI_PARMTYPE_TOGGLE) {
			int val = parm->getIntValue(0);
			mi->setText(parm->label());
			mi->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParams[asset_name].push_back(*mi);
			MenuItem* miLabel = mi;
			mi = menu->addButton("", "");
			mi->getButton()->setCheckable(true);
			mi->getButton()->setChecked(val);
			mi->setUserData(miLabel); // reference to the label, for updating values

		} else if (parm->info().type == HAPI_PARMTYPE_BUTTON) {
			int val = parm->getIntValue(0);
			mi->setText(parm->label());
			mi->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParams[asset_name].push_back(*mi);
			MenuItem* miLabel = mi;
			mi = menu->addButton("", "");
// 					mi->getButton()->setCheckable(true);
			mi->getButton()->setChecked(val);
			mi->setUserData(miLabel); // reference to the label, for updating values

		} else if (parm->info().type == HAPI_PARMTYPE_FLOAT ||
				   parm->info().type == HAPI_PARMTYPE_COLOR) {
			float val = parm->getFloatValue(0);
			mi->setText(parm->label() + ": " + ostr("%1%", %val));
			mi->setUserTag(asset_name);
			assetParams[asset_name].push_back(*mi);
			MenuItem* miLabel = mi;
			mi = menu->addSlider(100 * (parm->info().max), "");
			mi->getSlider()->setValue(int(val) * 100);
			// use userData to store ref to the label, for changing values
			// when updating
			mi->setUserData(miLabel);
		} else if (parm->info().type == HAPI_PARMTYPE_STRING ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) { // TODO: update with TextBox
			std::string val = parm->getStringValue(0);
			mi->setText(parm->label() + ": " + ostr("%1%", %val));
			mi->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParams[asset_name].push_back(*mi);
			MenuItem* miLabel = mi;
			mi = menu->addItem(MenuItem::Label);
			mi->setText(val);
			mi->setUserData(miLabel); // reference to the label, for updating values
		} else if (parm->info().type == HAPI_PARMTYPE_SEPARATOR) { // TODO: don't double up
			mi->setText("");
			mi->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParams[asset_name].push_back(*mi);
			MenuItem* miLabel = mi;
			mi = menu->addItem(MenuItem::Label);
			mi->setText("");
			mi->setUserData(miLabel); // reference to the label, for updating values
		}
		mi->setUserTag(parm->name());
		mi->setListener(this);
		assetParams[asset_name].push_back(*mi);
	} else {
		mi = menu->addItem(MenuItem::Label);
		mi->setText(parm->label());

		assetParams[asset_name].push_back(*mi);

		// use sliders for floats
		if (parm->info().type == HAPI_PARMTYPE_FLOAT ||
			parm->info().type == HAPI_PARMTYPE_COLOR) {
			mi->setText(parm->label());
			mi->setUserTag(asset_name);
			MenuItem* miLabel = mi;
			for (int j = 0; j < parm->info().size; ++j) {
				float val = parm->getFloatValue(j);
				mi = menu->addSlider(100 * (parm->info().max), "");
				mi->getSlider()->setValue(int(val) * 100);
				// use userData to store ref to the label, for changing values
				// when updating
				mi->setUserData(miLabel);
				mi->setUserTag(parm->name() + ostr(" %1%", %j));
				mi->setListener(this);
				miLabel->setText(ostr("%1% %2%", %miLabel->getText() %val));
				assetParams[asset_name].push_back(*mi);
			}
		}
	}

}


// only run on master
// identical-looking menus get created on slaves
void HoudiniEngine::createMenu(const String& asset_name)
{
	// load only the params in the DA Folder
	// first find the index of the folder, then iterate through the list of params
	// from there onwards
	const char* daFolderName = "daFolder_0";

	hapi::Asset* myAsset = instancedHEAssets[asset_name];

	if (myAsset == NULL) {
		ofwarn("No asset of name %1%", %asset_name);
	}

	std::map<std::string, hapi::Parm> parmMap = myAsset->parmMap();
	std::vector<hapi::Parm> parms = myAsset->parms();

	int daFolderIndex = 0;
	if (parmMap.count(daFolderName) > 0) {
		HAPI_ParmId daFolderId = parmMap[daFolderName].info().id;
		while (daFolderIndex < parms.size()) {
			hapi::Parm* parm = &parms[daFolderIndex];
			ofmsg("(before) PARM %1%: %2%", %parm->label() %parm->info().type);
			if (daFolderId == parms[daFolderIndex].info().id) {
				break;
			}
			daFolderIndex ++;
		}
	}

	ui::Menu* menu = houdiniMenu;

	int i = daFolderIndex;

	Dictionary<int, Menu* > subMenus; // keep refs to submenus

	while (i < parms.size()) {
		hapi::Parm* parm = &parms[i];
		ofmsg("PARM %1%: %2% %3% %4% %5%", %parm->label()
										   %parm->info().id
										   %parm->info().type
										   %parm->info().size
										   %parm->info().parentId
		);

		if (parm->info().type == HAPI_PARMTYPE_FOLDERLIST) {
			cout << "doing folderList " << parm->info().id << endl;
			ui::Menu* subMenu = NULL;
			if (parm->info().parentId == -1) {
				subMenu = houdiniMenu;
			} else {
				subMenu = menu->addSubMenu(parm->label() + "-->");
			}

			subMenus[parm->info().id] = subMenu;
		} else if (parm->info().type == HAPI_PARMTYPE_FOLDER) {
			cout << "doing folder " << parm->info().id << endl;
			ui::Menu* subMenu = menu->addSubMenu(parm->label());
			subMenus[parm->info().id] = subMenu;
		} else {
			cout << "doing param " << parm->info().id << endl;
			if (parm->info().parentId == -1) {
				menu = houdiniMenu;
			} else {
				if (subMenus.find(parm->info().parentId) != subMenus.end()) {
					menu = subMenus[parm->info().parentId];
				}
			}
			MenuItem* mi = NULL;

			// skip if invisible, BUG: but this affects submenus too..
			if (parm->info().invisible) {
				i++;
				continue;
			}

			createMenuItem(asset_name, menu, parm);
		}

		i++;
	}

}


// only run on master
// returns id of the asset instance
int HoudiniEngine::instantiateAsset(const String& asset_name)
{
	if (!SystemManager::instance()->isMaster()) {
		return -1;
	}

	int asset_id = -1;

	try {

 	ofmsg("about to instantiate %1%", %asset_name);

    ENSURE_SUCCESS(session, HAPI_InstantiateAsset(
			session,
            asset_name.c_str(),
            /* cook_on_load */ true,
            &asset_id ));

	if (asset_id < 0) {
		ofmsg("unable to instantiate %1%", %asset_name);
		return -1;
	}

 	ofmsg("instantiated %1%, id: %2%", %asset_name %asset_id);

	wait_for_cook();

//     HAPI_AssetInfo asset_info;
//     ENSURE_SUCCESS(session,  HAPI_GetAssetInfo( asset_id, &asset_info ) );

	Ref <RefAsset> myAsset = new RefAsset(asset_id, session);
	instancedHEAssets[asset_name] = myAsset;
    process_assets(*myAsset.get());

	createMenu(asset_name);
	updateGeos = true;

	} catch (hapi::Failure &failure)
	{
		ofwarn("%1%", %failure.lastErrorMessage(session));
		throw;
	}

	return asset_id;
}

int HoudiniEngine::instantiateAssetById(int asset_id)
{
	if (!SystemManager::instance()->isMaster()) {
		return -1;
	}

// 	ofmsg("about to instantiate %1%", %asset_name);

	int assetCount;

    ENSURE_SUCCESS(session,  HAPI_GetAvailableAssetCount( session, library_id, &assetCount ) );

	ofmsg("%1% assets available", %assetCount);
    HAPI_StringHandle* asset_name_sh = new HAPI_StringHandle[assetCount];
    ENSURE_SUCCESS(session,  HAPI_GetAvailableAssets( session, library_id, asset_name_sh, assetCount ) );

	std::string asset_name;

	for (int i =0; i < assetCount; ++i) {
	    asset_name = get_string( session, asset_name_sh[i] );
		ofmsg("asset %1%: %2%", %(i + 1) %asset_name);
	}

	asset_name = get_string( session, asset_name_sh[asset_id]);

    ENSURE_SUCCESS(session, HAPI_InstantiateAsset(
			session,
            asset_name.c_str(),
            /* cook_on_load */ true,
            &asset_id ));

	if (asset_id < 0) {
		ofmsg("unable to instantiate %1%", %asset_name);

		delete [] asset_name_sh;
		return -1;
	}

// 	ofmsg("instantiated %1%, id: %2%", %asset_name %asset_id);

	wait_for_cook();

//     HAPI_AssetInfo asset_info;
//     ENSURE_SUCCESS(session,  HAPI_GetAssetInfo( asset_id, &asset_info ) );

	Ref <RefAsset> myAsset = new RefAsset(asset_id, session);
	instancedHEAssets[asset_name] = myAsset;
    process_assets(*myAsset.get());

	createMenu(asset_name);
	updateGeos = true;

	delete [] asset_name_sh;
	return asset_id;
}

// TODO: geometry should be instantiated like this:
//    asset node+
//     |
//     o- object node+ (staticObject)
//         |
//         o- geo node+ (staticObject or osg nodes)
//             |
//             o- part node+ (drawables as part of geo, no transforms)
StaticObject* HoudiniEngine::instantiateGeometry(const String& asset)
{
	String s = ostr("%1%", %asset);

	if (myHoudiniGeometrys[s] == NULL) {
		ofwarn("No model of %1%.. creating", %asset);

		HoudiniGeometry* hg = HoudiniGeometry::create(s);
		myHoudiniGeometrys[s] = hg;
		if (mySceneManager->getModel(s) == NULL) {
			mySceneManager->addModel(hg);
		}
	}

	return new StaticObject(mySceneManager, s);
}

// put houdini engine asset data into a houdiniGeometry
void HoudiniEngine::process_assets(const hapi::Asset &asset)
{
    vector<hapi::Object> objects = asset.objects();
	ofmsg("%1%: %2% objects", %asset.name() %objects.size());

	String s = ostr("%1%", %asset.name());

	HoudiniGeometry* hg;

	if (myHoudiniGeometrys.count(s) > 0) {
		hg = myHoudiniGeometrys[s];
	} else {
		hg = HoudiniGeometry::create(s);
		myHoudiniGeometrys[s] = hg;

	}

	hg->objectsChanged = asset.info().haveObjectsChanged;
	ofmsg("process_assets: Objects changed:  %1%", %hg->objectsChanged);

	if (hg->getObjectCount() < objects.size()) {
		hg->addObject(objects.size() - hg->getObjectCount());
	}

	if (hg->objectsChanged) {
		for (int object_index=0; object_index < int(objects.size()); ++object_index)
	    {
			HAPI_ObjectInfo objInfo = objects[object_index].info();

			// TODO check for instancing, then do things differently
			if (objInfo.isInstancer > 0) {
				ofmsg("instance path: %1%: %2%", %objects[object_index].name() %objects[object_index].objectInstancePath());
				ofmsg("%1%: %2%", %objects[object_index].id %objInfo.objectToInstanceId);
			}

			vector<hapi::Geo> geos = objects[object_index].geos();
			ofmsg("%1%: %2%: %3% geos", %objects[object_index].name() %object_index %geos.size());

			if (hg->getGeodeCount(object_index) < geos.size()) {
				hg->addGeode(geos.size() - hg->getGeodeCount(object_index), object_index);
			}

			hg->setGeosChanged(objInfo.haveGeosChanged, object_index);
			ofmsg("process_assets: Object Geos %1% changed: %2%", %object_index %hg->getGeosChanged(object_index));

			if (hg->getGeosChanged(object_index)) {

				for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
				{
				    vector<hapi::Part> parts = geos[geo_index].parts();
					ofmsg("process_assets: Geo %1%:%2% %3% parts", %object_index %geo_index %parts.size());

					if (hg->getDrawableCount(geo_index, object_index) < parts.size()) {
						hg->addDrawable(parts.size() - hg->getDrawableCount(geo_index, object_index), geo_index, object_index);
					}

					hg->setGeoChanged(geos[geo_index].info().hasGeoChanged, geo_index, object_index);
					ofmsg("process_assets: Geo %1%:%2% changed:  %3%", %object_index %geo_index %hg->getGeosChanged(object_index));

					hg->clear(geo_index, object_index);
					if (hg->getGeoChanged(geo_index, object_index)) {
					    for (int part_index=0; part_index < int(parts.size()); ++part_index)
						{
							ofmsg("processing %1% %2%", %s %parts[part_index].name());
							process_geo_part(parts[part_index], object_index, geo_index, part_index, hg);
						}
					}
				}
			}

 			hg->setTransformChanged(objInfo.hasTransformChanged, object_index);
			ofmsg("process_assets: Object Transform %1% changed:  %2%", %object_index %hg->getTransformChanged(object_index));
		}

		HAPI_Transform* objTransforms = new HAPI_Transform[objects.size()];
		// NB: this resets all ObjectInfo::hasTransformChanged flags to false
		ENSURE_SUCCESS(session, HAPI_GetObjectTransforms( session, asset.id, HAPI_TRS, objTransforms, 0, objects.size()));

		for (int object_index=0; object_index < int(objects.size()); ++object_index)
	    {
			if (hg->getTransformChanged(object_index)) {
				hg->getOsgNode()->asGroup()->getChild(object_index)->asTransform()->
					asPositionAttitudeTransform()->setPosition(osg::Vec3d(
						objTransforms[object_index].position[0],
						objTransforms[object_index].position[1],
						objTransforms[object_index].position[2]
					)
				);

				hg->getOsgNode()->asGroup()->getChild(object_index)->asTransform()->
					asPositionAttitudeTransform()->setScale(osg::Vec3d(
						objTransforms[object_index].scale[0],
						objTransforms[object_index].scale[1],
						objTransforms[object_index].scale[2]
					)
				);
			}
	    }
	    delete[] objTransforms;
	}

	if (mySceneManager->getModel(s) == NULL) {
		mySceneManager->addModel(hg);
	}
}

// TODO: incrementally update the geometry?
// send a new version, and still have the old version?
void HoudiniEngine::process_geo_part(const hapi::Part &part, const int objIndex, const int geoIndex, const int partIndex, HoudiniGeometry* hg)
{
	vector<Vector3f> points;
	vector<Vector3f> normals;
	vector<Vector3f> colors;

	// texture coordinates
	vector<Vector3f> uvs;

	bool has_point_normals = false;
	bool has_vertex_normals = false;
	bool has_point_colors = false;
	bool has_vertex_colors = false;
	bool has_primitive_colors = false;

	bool has_point_uvs = false;
	bool has_vertex_uvs = false;

// 	ofmsg("clearing %1%", %hg->getName());
// 	hg->clear();

	//  attrib owners:
	// 	HAPI_ATTROWNER_VERTEX
	// 	HAPI_ATTROWNER_POINT
	// 	HAPI_ATTROWNER_PRIM
	// 	HAPI_ATTROWNER_DETAIL
	// 	HAPI_ATTROWNER_MAX

	// attributes:
	// 	HAPI_ATTRIB_COLOR 		"Cd"
	// 	HAPI_ATTRIB_NORMAL		"N"
	// 	HAPI_ATTRIB_POSITION	"P" // usually on point
	// 	HAPI_ATTRIB_TANGENT		"tangentu"
	// 	HAPI_ATTRIB_TANGENT2	"tangentv"
	// 	HAPI_ATTRIB_UV			"uv"
	// 	HAPI_ATTRIB_UV2			"uv2"

	// TODO: improve this..
    vector<std::string> point_attrib_names = part.attribNames(
	HAPI_ATTROWNER_POINT);
    for (int attrib_index=0; attrib_index < int(point_attrib_names.size());
	    ++attrib_index) {

// 		ofmsg("has %1%", %point_attrib_names[attrib_index]);

		if (point_attrib_names[attrib_index] == "P") {
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "P", points);
		}
		if (point_attrib_names[attrib_index] == "N") {
			has_point_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "N", normals);
		}
		if (point_attrib_names[attrib_index] == "Cd") {
			has_point_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "Cd", colors);
		}
		if (point_attrib_names[attrib_index] == "uv") {
			has_point_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "uv", uvs);
		}
	}

    vector<std::string> vertex_attrib_names = part.attribNames(
	HAPI_ATTROWNER_VERTEX);
    for (int attrib_index=0; attrib_index < int(vertex_attrib_names.size());
	    ++attrib_index) {

// 		ofmsg("has %1%", %vertex_attrib_names[attrib_index]);

		if (vertex_attrib_names[attrib_index] == "N") {
			has_vertex_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "N", normals);
		}
		if (vertex_attrib_names[attrib_index] == "uv") {
			has_vertex_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "uv", uvs);
		}
	}

    vector<std::string> primitive_attrib_names = part.attribNames(
	HAPI_ATTROWNER_PRIM);
    for (int attrib_index=0; attrib_index < int(primitive_attrib_names.size());
	    ++attrib_index) {

// 		ofmsg("has %1%", %primitive_attrib_names[attrib_index]);

		if (primitive_attrib_names[attrib_index] == "Cd") {
			has_primitive_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_PRIM, "Cd", colors);
		}
	}

	if (part.info().faceCount == 0) {
		// this is to do with instancing
 		ofmsg ("No faces, points count? %1%", %points.size());
		hg->dirty();
// 		return;
	}

	// TODO: render curves
// 	if (part.info().isCurve) {
//  		ofmsg ("This part is a curve: %1%", %part.name());
// 		return;
// 	}

    int *face_counts = new int[ part.info().faceCount ];
    ENSURE_SUCCESS(session,  HAPI_GetFaceCounts(
		session,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		face_counts,
		0,
		part.info().faceCount
	) );

	// Material handling (WIP)

	bool all_same = true;
	HAPI_MaterialId* mat_ids = new HAPI_MaterialId[ part.info().faceCount ];

    ENSURE_SUCCESS(session,  HAPI_GetMaterialIdsOnFaces(
		session,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		&all_same /* are_all_the_same*/,
		mat_ids, 0, part.info().faceCount ));

	HAPI_MaterialInfo mat_info;

	ENSURE_SUCCESS(session,  HAPI_GetMaterialInfo (
		session,
		part.geo.object.asset.id,
		mat_ids[0],
		&mat_info));

	if (mat_info.exists) {
		ofmsg("Material info for %1%: matId: %2% assetId: %3% nodeId: %4%",
			  %hg->getName() %mat_info.id %mat_info.assetId %mat_info.nodeId
		);

		HAPI_NodeInfo node_info;
		ENSURE_SUCCESS(session,  HAPI_GetNodeInfo(
			session,
			mat_info.nodeId, &node_info ));
		ofmsg("Node info for %1%: id: %2% assetId: %3% internal path: %4%",
			  %get_string( session, node_info.nameSH) %node_info.id %node_info.assetId
			  %get_string( session, node_info.internalNodePathSH)
		);

		HAPI_ParmInfo* parm_infos = new HAPI_ParmInfo[node_info.parmCount];
		ENSURE_SUCCESS(session,  HAPI_GetParameters(
			session,
			mat_info.nodeId,
			parm_infos,
			0 /* start */,
			node_info.parmCount));

		int mapIndex = -1;

		for (int i =0; i < node_info.parmCount; ++i) {
			ofmsg("%1% %2% (%3%) id = %4%",
				  %i
				  %get_string( session, parm_infos[i].labelSH)
				  %get_string( session, parm_infos[i].nameSH)
				  %parm_infos[i].id);
			if (parm_infos[i].stringValuesIndex >= 0) {
				int sh = -1;
				ENSURE_SUCCESS(session,  HAPI_GetParmStringValues(
					session,
					mat_info.nodeId, true,
					&sh,
					parm_infos[i].stringValuesIndex, 1)
				);
				ofmsg("%1%  %2%: %3%", %i %parm_infos[i].id
					%get_string(session, sh));

				if (sh != -1 && (get_string(session, parm_infos[i].nameSH) == "baseColorMap")) {
// 				if (sh != -1 && (get_string(session, parm_infos[i].nameSH) == "map")) {
					mapIndex = i;
				}
			}
		}

		for (int i = 0; i < node_info.parmCount; ++i) {
			ofmsg("index %1%: %2% '%3%'",
				  %i
				  %parm_infos[i].id
				  %get_string(session, parm_infos[i].nameSH)
			);
		}

// 		ofmsg("the texture path: assetId=%1% matInfo.id=%2% mapIndex=%3% parm_id=%4% string=%5%",
// 			  %mat_info.assetId
// 			  %mat_info.id
// 			  %mapIndex
// 			  %parm_infos[mapIndex].id
// 			  %get_string(session, parm_infos[mapIndex].nameSH)
// 		);

		// NOTE this works if the image is a png
 		ENSURE_SUCCESS(session,  HAPI_RenderTextureToImage(
			session,
			mat_info.assetId,
 			mat_info.id,
			parm_infos[mapIndex].id /* parmIndex for "map" */));


		// render using mantra
//  		ENSURE_SUCCESS(session,  HAPI_RenderMaterialToImage(
//  			mat_info.assetId,
//  			mat_info.id,
// 			HAPI_SHADER_MANTRA));
// // 			HAPI_SHADER_OPENGL));

		HAPI_ImageInfo image_info;
		ENSURE_SUCCESS(session,  HAPI_GetImageInfo(
			session,
			mat_info.assetId,
			mat_info.id,
			&image_info));

		ofmsg("width %1% height: %2% format: %3% dataFormat: %4% packing %5%",
			  %image_info.xRes
			  %image_info.yRes
			  %get_string(session, image_info.imageFileFormatNameSH)
			  %image_info.dataFormat
			  %image_info.packing
		);
		// ---------

		HAPI_StringHandle imageSH;

		ENSURE_SUCCESS(session,  HAPI_GetImagePlanes(
			session,
			mat_info.assetId,
			mat_info.id,
			&imageSH,
			1
		));

		int imgBufSize = -1;

		//TODO: get the image extraction working correctly..
		// needed to convert from PNG/JPG/etc to RGBA.. use decode() from omegalib

		// get image planes into a buffer (default is png.. change to RGBA?)
		ENSURE_SUCCESS(session,  HAPI_ExtractImageToMemory(
			session,
			mat_info.assetId,
			mat_info.id,
			HAPI_PNG_FORMAT_NAME,
// 			NULL /* HAPI_DEFAULT_IMAGE_FORMAT_NAME */,
			"C A", /* image planes */
			&imgBufSize
		));

		char *myBuffer = new char[imgBufSize];

		// put into a buffer
		ENSURE_SUCCESS(session,  HAPI_GetImageMemoryBuffer(
			session,
			mat_info.assetId,
			mat_info.id,
			myBuffer /* tried (char *)pd->map() */,
			imgBufSize
		));

		// load into a pixelData bufferObject
		// this works!
		Ref<PixelData> refPd = ImageUtils::decode((void *) myBuffer, image_info.xRes * image_info.xRes * 4, "");

		// TODO: set this in the object's material(s)!
		osg::Image* myImg = OsgModule::pixelDataToOsg(refPd, true); //transferBufferOwnership = true

		// TODO: image is correct, and UVs also seem correct (bind per vertex regardless?)
		// so apply iamge to texture to the object, probably do through houdiniGeometry? refer to
		osg::Texture2D* tex = new osg::Texture2D;
		tex->setImage(myImg);

		// should have something else here.. this doesn't seem to be working..
		// TODO: put textures in HoudiniGeometry, use default shader to show everything properly
// 		hg->getOsgNode(geoIndex, objIndex)->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);

		// just trying this out.. it works??
// 		MenuItem* myNewImage = myMenuManager->getMainMenu()->addItem(MenuItem::Image);
// 		myNewImage->setImage(refPd);

// 		ofmsg("my image width %1% height: %2%, size %3%, bufSize %4%",
// 			%refPd->getWidth() %refPd->getHeight() %refPd->getSize() %imgBufSize
// 		);
	} else {
		ofmsg("No material for %1%", %hg->getName());
	}

	// end materials

    int * vertex_list = new int[ part.info().vertexCount ];
    ENSURE_SUCCESS(session,  HAPI_GetVertexList(
		session,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		vertex_list, 0, part.info().vertexCount ) );
    int curr_index = 0;

	int prev_faceCount = face_counts[0];
    int prev_faceCountIndex = 0;

	// TODO: get primitive set working for different triangles..

	// primitives with sides > 4 can't use drawArrays, as it thinks all points are part
	// of the triangle_fan. should use drawMultipleArrays, but osg doesn't have it?
	// instead, make a primitive set for each primitive > 4 facecount.
	// it has something better: drawArrayLengths(osgPrimitiveType(TRIANGLE_FAN), start index, length)
	// may use next iteration over this

	osg::PrimitiveSet::Mode myType;

	// objects with primitives of different side count > 4 don't get rendered well. get around this
	// by triangulating meshes on houdini engine side.

    for( int ii=0; ii < part.info().faceCount; ii++ )
    {

		// add primitive group if face count is different from previous
		if (face_counts[ii] != prev_faceCount) {

			if (prev_faceCount == 3) {
				myType = osg::PrimitiveSet::TRIANGLES;
			} else if (prev_faceCount == 4) {
				myType = osg::PrimitiveSet::QUADS;
			}

// 			cout << "making primitive set for " << prev_faceCount << ", from " <<
// 				prev_faceCountIndex << " plus " <<
// 				curr_index - prev_faceCountIndex << endl;

			hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

			prev_faceCountIndex = curr_index;
			prev_faceCount = face_counts[ii];
		} else if ((ii > 0) && (prev_faceCount > 4)) {
// 			cout << "making primitive set for " << prev_faceCount << ", plus " <<
// 				prev_faceCountIndex << " to " <<
// 				curr_index - prev_faceCountIndex << endl;
			hg->addPrimitiveOsg(osg::PrimitiveSet::TRIANGLE_FAN,
				prev_faceCountIndex,
			    curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

			prev_faceCountIndex = curr_index;
			prev_faceCount = face_counts[ii];
		}

// 		cout << "face (" << face_counts[ii] << "): " << ii << " ";
        for( int jj=0; jj < face_counts[ii]; jj++ )
        {

			int myIndex = curr_index + (face_counts[ii] - jj) % face_counts[ii];
// 			cout << "i: " << vertex_list[myIndex] << " ";

			int lastIndex = hg->addVertex(points[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);

			if (has_point_normals) {
				hg->addNormal(normals[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
			} else if (has_vertex_normals) {
				hg->addNormal(normals[myIndex], partIndex, geoIndex, objIndex);
			}
			if(has_point_colors) {
				hg->addColor(Color(
					colors[vertex_list[ myIndex ]][0],
					colors[vertex_list[ myIndex ]][1],
					colors[vertex_list[ myIndex ]][2],
					1.0
				), partIndex, geoIndex, objIndex);
			} else if (has_primitive_colors) {
				hg->addColor(Color(
					colors[ii][0],
					colors[ii][1],
					colors[ii][2],
					1.0
				), partIndex, geoIndex, objIndex);
			}
			if (has_point_uvs) {
				hg->addUV(uvs[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
// 				cout << "(p)uvs: " << uvs[vertex_list[ myIndex ]][0] << ", " << uvs[vertex_list[ myIndex ]][1] << endl;
			} else if (has_vertex_uvs) {
// 				hg->addUV(uvs[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
				hg->addUV(uvs[myIndex], partIndex, geoIndex, objIndex);
// 				cout << "(v)uvs: " << uvs[myIndex][0] << ", " << uvs[myIndex][1] << endl;
			}

//             cout << "v:" << myIndex << ", i: "
// 				<< hg->getVertex(lastIndex) << endl; //" ";
        }

        curr_index += face_counts[ii];

    }

	if (prev_faceCount == 3) {
		myType = osg::PrimitiveSet::TRIANGLES;
	} else if (prev_faceCount == 4) {
		myType = osg::PrimitiveSet::QUADS;
	}
	if (prev_faceCount > 4) {
// 		cout << "face count is " << prev_faceCount << endl;
		myType = osg::PrimitiveSet::TRIANGLE_FAN;
	}

// 	cout << "making final primitive set for " << prev_faceCount << ", from " <<
// 		prev_faceCountIndex << " plus " <<
// 		curr_index - prev_faceCountIndex << endl;

	hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

	hg->dirty();

    delete[] face_counts;
    delete[] vertex_list;

}


void HoudiniEngine::process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name, vector<Vector3f>& points)
{
	// useHAPI_AttributeInfo::storage for the data type

    // Get the attribute values.
    HAPI_AttributeInfo attrib_info = part.attribInfo(attrib_owner, attrib_name);
    float *attrib_data = part.getNewFloatAttribData(attrib_info, attrib_name);

// 	cout << attrib_name << " (" << attrib_info.tupleSize << ")" << endl;

	points.clear();
	points.resize(attrib_info.count);
    for (int elem_index=0; elem_index < attrib_info.count; ++elem_index)
    {

		Vector3f v;
		for (int tuple_index=0; tuple_index < attrib_info.tupleSize;
			++tuple_index)
		{
			v[tuple_index] = attrib_data[elem_index * attrib_info.tupleSize + tuple_index ];
		}
		points[elem_index] = v;
// 		cout << elem_index << ": " << v << endl;
    }

    delete [] attrib_data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::initialize()
{
	enableSharedData();

	if (SystemManager::instance()->isMaster()) {
	    try
	    {
			// create sessions
			session = &mySession;
// 			session = NULL; // NULL means use in-process session

			if (session != NULL) {
				HAPI_StartThriftSocketServer( true, 7788, 5000, NULL);
				HAPI_CreateThriftSocketSession(session, "localhost", 7788);
			}

		    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();

			ENSURE_SUCCESS(session, HAPI_Initialize(
// 				/* session */ NULL,
				session,
				&cook_options,
				/*use_cooking_thread=*/true,
				/*cooking_thread_max_size=*/-1,
				/*otl search path*/ getenv("$HOME"),
				/*dso_search_path=*/ NULL,
				/*image_dso_search_path=*/ NULL,
				/*audio_dso_search_path=*/ NULL
			));
	    }
	    catch (hapi::Failure &failure)
	    {
			ofwarn("Houdini Failure.. %1%", %failure.lastErrorMessage(session));
			throw;
	    }
	}

	// Create and initialize the cyclops scene manager.
	mySceneManager = SceneManager::createAndInitialize();

	// Create the scene editor and add our loaded object to it.
// 	myEditor = SceneEditorModule::createAndInitialize();
// 	myEditor->addNode(myObject);
// 	myEditor->setEnabled(false);

	// Create and initialize the menu manager
	myMenuManager = MenuManager::createAndInitialize();

	// Create the root menu
	ui::Menu* menu = myMenuManager->createMenu("menu");
	myMenuManager->setMainMenu(menu);

	// Create the houdini engine menu
	houdiniMenu = menu->addSubMenu("Houdini Engine");
	MenuItem* myLabel = houdiniMenu->addItem(MenuItem::Label);
	myLabel->setText("Houdini Engine Parameters");

// 	sn = SceneNode::create("myOtl");
// 	myEditor->addNode(sn);

	// Add the 'quit' menu item. Menu items can either run callbacks when activated, or they can
	// run script commands. In this case, we make the menu item call the 'oexit' script command.
	// oexit is the standard omegalib script command used to terminate the application.
	// Using scripting with menu items can be extremely powerful and flexible, and limits the amount
	// of callback code that needs to be written to handle ui events.
	myQuitMenuItem = menu->addItem(MenuItem::Button);
	myQuitMenuItem->setText("Quit");
	myQuitMenuItem->setCommand("oexit()");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::update(const UpdateContext& context)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::onMenuItemEvent(MenuItem* mi)
{

// 	ofmsg("%1%: pressed a menu item", %SystemManager::instance()->getHostname());

	if (!SystemManager::instance()->isMaster())
	{
		return;
	}

	// TODO: change this to suit running on the master only, but callable from slave..
	bool update = false;


	// user tag is of form '<parmName> <parmIndex>'
	// eg: 'size 2.1' for a size vector
	int z = mi->getUserTag().find_last_of(" "); // index of space

	// TODO: do a null check here.. may break
	String asset_name = ((MenuItem*)mi->getUserData())->getUserTag();
	hapi::Asset* myAsset = instancedHEAssets[asset_name];

	std::map<std::string, hapi::Parm> parmMap = myAsset->parmMap();

	hapi::Parm* parm = &(parmMap[mi->getUserTag().substr(0, z)]);
	int index = atoi(mi->getUserTag().substr(z + 1).c_str());

	if (myAsset == NULL) {
		ofwarn("No instanced asset %1%", %asset_name);
		return;
	}

	if (parm->info().type == HAPI_PARMTYPE_INT) {
		parm->setIntValue(index, mi->getSlider()->getValue());

		((MenuItem*)mi->getUserData())->setText(parm->label() + ": " +
		ostr("%1%", %mi->getSlider()->getValue()));

		update = true;

	} else if (parm->info().type == HAPI_PARMTYPE_TOGGLE) {
		parm->setIntValue(index, mi->getButton()->isChecked());

		((MenuItem*)mi->getUserData())->setText(parm->label());

		update = true;

	} else if (parm->info().type == HAPI_PARMTYPE_FLOAT) {
		parm->setFloatValue(index, 0.01 * mi->getSlider()->getValue());

		((MenuItem*)mi->getUserData())->setText(parm->label() + ": " +
		ostr("%1%", %(mi->getSlider()->getValue() * 0.01)));

		update = true;

	}

	if (update) {

		// TODO: do incremental checks for what i need to update:
		// HAPI_ObjectInfo::hasTransformChanged
		// HAPI_ObjectInfo::haveGeosChanged
		// HAPI_GeoInfo::hasGeoChanged
		// HAPI_GeoInfo::hasMaterialChanged


		myAsset->cook();
		wait_for_cook();

		process_assets(*myAsset);

		updateGeos = true;
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::onSelectedChanged(SceneNode* source, bool value)
{
	/*
// 	if(source == myObject)
// 	{
// 		if(myObject->isSelected())
// 		{
// 			// Color the object yellow when selected.
// 			myObject->setEffect("colored -d #ffff30 -g 1.0 -s 20.0");
// 		}
// 		else
// 		{
// 			// Color the object gray when not selected.
// 			myObject->setEffect("colored -d #303030 -g 1.0 -s 20.0");
// 		}
// 	}
	*/
}

void HoudiniEngine::setLoggingEnabled(const bool toggle) {
	myLogEnabled = toggle;
}

// TODO: make an async version of for omegalib
void HoudiniEngine::wait_for_cook()
{
    int status;
    do
    {
 		osleep(50); // sleeping..

		if (myLogEnabled) {
	        int statusBufSize = 0;
	        ENSURE_SUCCESS(session,  HAPI_GetStatusStringBufLength(
				session,
/* 	            HAPI_STATUS_COOK_STATE, HAPI_STATUSVERBOSITY_MESSAGES, */
	            HAPI_STATUS_COOK_STATE, HAPI_STATUSVERBOSITY_ERRORS,
	            &statusBufSize ) );
	        char * statusBuf = NULL;
	        if ( statusBufSize > 0 )
	        {
	            statusBuf = new char[statusBufSize];
	            ENSURE_SUCCESS(session,  HAPI_GetStatusString(
					session,
	                HAPI_STATUS_COOK_STATE, statusBuf, statusBufSize ) );
	        }
	        if ( statusBuf )
	        {
	            std::string result( statusBuf );
	            ofmsg("cooking...:%1%", %result);
	            delete[] statusBuf;
	        }
		}
        HAPI_GetStatus(session, HAPI_STATUS_COOK_STATE, &status);
    }
    while ( status > HAPI_STATE_MAX_READY_STATE );
    ENSURE_COOK_SUCCESS( session, status );
}

static std::string houdiniEngine::get_string(HAPI_Session* session, int string_handle)
{
    // A string handle of 0 means an invalid string handle -- similar to
    // a null pointer.  Since we can't return NULL, though, return an empty
    // string.
    if (string_handle == 0)
	return "";

    int buffer_length;
    ENSURE_SUCCESS(session, HAPI_GetStringBufLength(session, string_handle, &buffer_length));

    char * buf = new char[ buffer_length ];

    ENSURE_SUCCESS(session, HAPI_GetString(session, string_handle, buf, buffer_length));

    std::string result( buf );
    delete[] buf;
    return result;
}

// TODO: expose to python to write my own event handler
///////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::handleEvent(const Event& evt)
{
/*
// 	bool update = false;
//
// 	// switch between different objects
// 	if (evt.isKeyDown('0')) {
// 		mySwitch = (mySwitch + 1) % myAsset->parmMap()["switchCount"].getIntValue(0); // number of switches.. how get a var
// // 		cout << ">>> old switch value: " << myAsset->parmMap()["switch1_input"].getIntValue(0) <<
// // 		"/" << myAsset->parmMap()["switchCount"].getIntValue(0) << endl;
// 		myAsset->parmMap()["switch1_input"].setIntValue(0, mySwitch);
// 		update = true;
// 	}
//
// 	if (evt.isKeyDown('t')) {
// 		myAsset->parmMap()["switch_subdivide"].setIntValue(
// 			0,
// 			(myAsset->parmMap()["switch_subdivide"].getIntValue(0) + 1) % 2
// 		);
// 		update = true;
// 	}
//
// 	// cook only if a change
// 	if ((myAsset != NULL) && update) {
// 		myAsset->cook();
// 		wait_for_cook();
// 		process_assets(*myAsset);
// // 		cout << ">>> new switch value: " << myAsset->parmMap()["switch1_input"].getIntValue(0) << endl;
// 	}
//
*/
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// only run on master
// for each part of each geo of each object of each asset:
// send all the verts, faces, normals, colours, etc
void HoudiniEngine::commitSharedData(SharedOStream& out)
{

	out << updateGeos; // may not be necessary to send this..

	// continue only if there is something to send
	if (!updateGeos) {
		return;
	}

	updateGeos = false;

//  	ofmsg("MASTER: sending %1% assets", %myHoudiniGeometrys.size());

	// TODO change this to count the number of changed objs, geos, parts
	out << int(myHoudiniGeometrys.size());

    foreach(HGDictionary::Item hg, myHoudiniGeometrys)
    {
		bool haveObjectsChanged = hg->objectsChanged;
// 		ofmsg("MASTER: Objects changed:  %1%", %haveObjectsChanged);
		out << haveObjectsChanged;

		if (!haveObjectsChanged) {
			continue;
		}

// 		ofmsg("MASTER: Name %1%", %hg->getName());
		out << hg->getName();
		out << hg->getObjectCount();

		// objects
		for (int obj = 0; obj < hg->getObjectCount(); ++obj) {
			bool hasTransformChanged = hg->getTransformChanged(obj);
// 			ofmsg("MASTER: Transforms changed:  %1%", %hasTransformChanged);

			out << hasTransformChanged;

			if (hasTransformChanged) {
				osg::Vec3d pos = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getPosition();
				osg::Quat rot = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getAttitude();
				osg::Vec3d scale = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getScale();

				out << pos[0] << pos[1] << pos[2];
				out << rot.x() << rot.y() << rot.z() << rot.w();
				out << scale[0] << scale[1] << scale[2];
			}

			bool haveGeosChanged = hg->getGeosChanged(obj);
// 			ofmsg("MASTER: Geos changed:  %1%", %haveGeosChanged);
			out << haveGeosChanged;

			if (!haveGeosChanged) {
				continue;
			}

// 			ofmsg("Geodes: %1%", %hg->getGeodeCount(obj));
			out << hg->getGeodeCount(obj);

			// geoms
			for (int g = 0; g < hg->getGeodeCount(obj); ++g) {
				bool hasGeoChanged = hg->getGeoChanged(g, obj);
// 				ofmsg("MASTER: Geo changed:  %1%", %hasGeoChanged);
				out << hasGeoChanged;

				if (!hasGeoChanged) {
					continue;
				}

// 				ofmsg("Drawables: %1%", %hg->getDrawableCount(g, obj));
				out << hg->getDrawableCount(g, obj);

				// parts
				for (int d = 0; d < hg->getDrawableCount(g, obj); ++d) {
// 					ofmsg("Vertex count: %1%", %hg->getVertexCount(d, g, obj));
					out << hg->getVertexCount(d, g, obj);
					for (int i = 0; i < hg->getVertexCount(d, g, obj); ++i) {
						out << hg->getVertex(i, d, g, obj);
					}
// 					ofmsg("Normal count: %1%", %hg->getNormalCount(d, g, obj));
					out << hg->getNormalCount(d, g, obj);
					for (int i = 0; i < hg->getNormalCount(d, g, obj); ++i) {
						out << hg->getNormal(i, d, g, obj);
					}
// 					ofmsg("Color count: %1%", %hg->getColorCount(d, g, obj));
					out << hg->getColorCount(d, g, obj);
					for (int i = 0; i < hg->getColorCount(d, g, obj); ++i) {
						out << hg->getColor(i, d, g, obj);
					}
					// faces are done in that primitive set way
					// TODO: simplification: assume all faces are triangles?
					osg::Geometry* geo = hg->getOsgNode()->asGroup()->getChild(obj)->asGroup()->getChild(g)->asGeode()->getDrawable(d)->asGeometry();
					osg::Geometry::PrimitiveSetList psl = geo->getPrimitiveSetList();

					out << int(psl.size());
					for (int i = 0; i < psl.size(); ++i) {
						osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(psl[i].get());

						out << da->getMode() << int(da->getFirst()) << int(da->getCount());
					}
				}
			}
		}
	}

	// distribute the parameter list
	// int parmCount = assetParams.size();
	// disable parm distribution until menu is improved (WIP)
	int parmCount = 0;
	out << parmCount;

    foreach(Menus::Item mis, assetParams) {
		out << mis.first;
		out << int(mis.second.size());
		for (int i = 0; i < mis.second.size(); ++i) {
			MenuItem* mi = &mis.second[i];
			out << mi->getType();
			// MenuItem.Type: { Button, Checkbox, Slider, Label, SubMenu, Image, Container }
			if (mi->getType() == MenuItem::Label) {
				out << mi->getText();
			} else if (mi->getType() == MenuItem::Slider) {
				out << mi->getSlider()->getTicks();
				out << mi->getSlider()->getValue();
			} else if (mi->getType() == MenuItem::Button) { // checkbox is actually a button
				out << mi->isChecked();
			}
		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// only run on slaves!
void HoudiniEngine::updateSharedData(SharedIStream& in)
{
	in >> updateGeos;

	if (!updateGeos) {
		return;
	}

	// houdiniGeometry count
	int numItems = 0;

	in >> numItems;

// 	if (!SystemManager::instance()->isMaster()) {
// 		ofmsg("SLAVE %1%: getting data: %2%", %SystemManager::instance()->getHostname() %numItems);
// 	}

    for(int i = 0; i < numItems; i++) {

		bool haveObjectsChanged;
		in >> haveObjectsChanged;

// 		ofmsg("SLAVE: objs changed: %1%", %haveObjectsChanged);

		if (!haveObjectsChanged) {
// 			continue;
		}

        String name;
        in >> name;

// 		ofmsg("SLAVE: name: '%1%'", %name);

 		int objectCount = 0;
        in >> objectCount;

//  		ofmsg("SLAVE: obj count: '%1%'", %objectCount);

       HoudiniGeometry* hg = myHoudiniGeometrys[name];
        if(hg == NULL) {
//  			ofmsg("SLAVE: no hg: '%1%'", %name);
			hg = HoudiniGeometry::create(name);
			hg->addObject(objectCount);
			myHoudiniGeometrys[name] = hg;
			SceneManager::instance()->addModel(hg);
// 			continue;
// 		} else {
// 			ofmsg("SLAVE: i have the hg: '%1%'", %name);
// 			getHGInfo(name);
// 			ofmsg("SLAVE: end info for '%1%'", %name);
		}

// 		hg->clear();

// 		ofmsg("SLAVE: current obj count: '%1%'", %hg->getObjectCount());

		if (hg->getObjectCount() < objectCount) {
			hg->addObject(objectCount - hg->getObjectCount());
		}

// 		ofmsg("SLAVE: new obj count: '%1%'", %hg->getObjectCount());

 		for (int obj = 0; obj < objectCount; ++obj) {
			bool hasTransformChanged;
			in >> hasTransformChanged;

// 			ofmsg("SLAVE: transforms changed: '%1%'", %hasTransformChanged);

			if (hasTransformChanged) {
				osg::Vec3d pos;
				in >> pos[0] >> pos[1] >> pos[2];

				osg::Vec4d rot;
				osg::Quat qRot;
				in >> rot[0] >> rot[1] >> rot[2] >> rot[3];
				qRot.set(rot);

				osg::Vec3d scale;
				in >> scale[0] >> scale[1] >> scale[2];

				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setPosition(pos);
				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setAttitude(qRot);
				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setScale(scale);
			}

			bool haveGeosChanged;
			in >> haveGeosChanged;

// 			ofmsg("SLAVE: geos changed: '%1%'", %haveGeosChanged);

			if (!haveGeosChanged) {
				continue;
			}

			int geodeCount = 0;
	        in >> geodeCount;

//  			ofmsg("SLAVE: geode count: '%1%'", %geodeCount);

			if (hg->getGeodeCount(obj) < geodeCount) {
				hg->addGeode(geodeCount - hg->getGeodeCount(obj), obj);
			}

			for (int g = 0; g < geodeCount; ++g) {

				bool hasGeoChanged;
				in >> hasGeoChanged;

				if (!hasGeoChanged) {
					continue;
				}

				int drawableCount = 0;
		        in >> drawableCount;

//  				ofmsg("SLAVE: drawable count: '%1%'", %drawableCount);

				// add drawables if needed
				if (hg->getDrawableCount(g, obj) < drawableCount) {
					hg->addDrawable(drawableCount - hg->getDrawableCount(g, obj), g, obj);
				}

				hg->clear(g, obj);

				for (int d = 0; d < drawableCount; ++d) {

					int vertCount = 0;
					in >> vertCount;

// 					ofmsg("SLAVE: vertex count: '%1%'", %vertCount);
					for (int j = 0; j < vertCount; ++j) {
						Vector3f v;
						in >> v;
						hg->addVertex(v, d, g, obj);
					}

					int normalCount = 0;
					in >> normalCount;

// 					ofmsg("SLAVE: normal count: '%1%'", %normalCount);
					for (int j = 0; j < normalCount; ++j) {
						Vector3f n;
						in >> n;
						hg->addNormal(n, d, g, obj);
					}

					int colorCount = 0;
					in >> colorCount;

// 					ofmsg("SLAVE: color count: '%1%'", %colorCount);
					for (int j = 0; j < colorCount; ++j) {
						Color c;
						in >> c;
						hg->addColor(c, d, g, obj);
					}

					// primitive set count
					int psCount = 0;
					in >> psCount;

// 					ofmsg("SLAVE: primitive set count: '%1%'", %psCount);

					for (int j = 0; j < psCount; ++j) {
						osg::PrimitiveSet::Mode mode;
						int startIndex, count;
						in >> mode >> startIndex >> count;
						hg->addPrimitiveOsg(mode, startIndex, count, d, g, obj);
					}
				}
			}
		}

		hg->dirty();
    }

	// read in menu parms
	int parmCount = 0;
	in >> parmCount;

//  parameter list
// 	ofmsg("SLAVE: currently have %1% asset parameter lists", %assetParams.size());
//     foreach(Menus::Item mis, assetParams)
// 	{
// 		ofmsg("SLAVE: name %1%", %mis.first);
// 		ofmsg("SLAVE: number %1%", %mis.second.size());
// 		for (int i = 0; i < mis.second.size(); ++i) {
// 			ofmsg("SLAVE: menu item %1%", %mis.second[i].getType());
// 		}
// 	}
// 	ofmsg("SLAVE: received %1% asset parameter lists", %parmCount);

	// using assetparams map
	for (int j = 0; j < parmCount; ++j) {
		String name;
		in >> name;
// 		ofmsg("SLAVE: %1%", %name);
		int items = 0;
		in >> items;
// 		ofmsg("SLAVE: menu items to do: %1%", %items);
		MenuItem* prevMenuItem = NULL;
		for (int k = 0; k < items; ++k) {
			MenuItem::Type t;
			in >> t;
// 			ofmsg("SLAVE: menuItem : %1%", %t);

			MenuItem* mi = NULL;

			if (t == MenuItem::Label) {
				String s;
				in >> s;
// 				ofmsg("SLAVE: label : %1%", %s);

				if (assetParams[name].size() > k) {
					mi = &(assetParams[name][k]);
				} else {
					mi = houdiniMenu->addItem(MenuItem::Label);
					mi->setText(s);
					assetParams[name].push_back(*mi);
				}
				prevMenuItem = mi;
			} else if (t == MenuItem::Slider) {
				int ticks;
				in >> ticks;
				int value;
				in >> value;
// 				ofmsg("SLAVE: slider : %1% out of %2%", %value %ticks);
				MenuItem* slider = NULL;

				if (assetParams[name].size() > k) {
					slider = &(assetParams[name][k]);
				} else {
					slider = houdiniMenu->addSlider(ticks, "");
					assetParams[name].push_back(*slider);
				}
				slider->getSlider()->setValue(value);
			} else if (t == MenuItem::Button) {
				int value;
				in >> value;
// 				ofmsg("SLAVE: slider : %1% out of %2%", %value %ticks);
				MenuItem* button = NULL;

				if (assetParams[name].size() > k) {
					button = &(assetParams[name][k]);
				} else {
					button = houdiniMenu->addButton("", "");
					assetParams[name].push_back(*button);
				}
				button->getButton()->setCheckable(true);
				button->getButton()->setChecked(value);
			}
		}
	}
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
		hapi::Asset* myAsset = asset.second;
		myAsset->cook();
		wait_for_cook();
		process_assets(*myAsset);
		updateGeos = true;
	}
}
