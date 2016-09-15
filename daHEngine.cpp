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
	this file contains init methods for creating Assets in omegalib

******************************************************************************/

#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>
#include <daHEngine.static.cpp>

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
 		PYAPI_METHOD(HoudiniEngine, showMappings)
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
	instance->setPriority(HoudiniEngine::PriorityHigh);
	ModuleServices::addModule(instance);
	instance->doInitialize(Engine::instance());
	return instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniEngine::HoudiniEngine():
	EngineModule("HoudiniEngine"),
	mySceneManager(NULL),
	myLogEnabled(false),
	myAssetCount(0),
	currentAsset(-1),
	currentAssetName("")
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

	// total asset count
	myAssetCount += assetCount;

    return assetCount;

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

	omsg("after instantiate cook");

//     HAPI_AssetInfo asset_info;
//     ENSURE_SUCCESS(session,  HAPI_GetAssetInfo( asset_id, &asset_info ) );

	Ref <RefAsset> myAsset = new RefAsset(asset_id, session);
	instancedHEAssetsByName[asset_name] = myAsset;
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
	instancedHEAssetsByName[asset_name] = myAsset;

	ofmsg("instance asset name: %1%", %(*myAsset.get()).name());

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
	ofmsg("%1% assets available", %myAssetCount);
    HAPI_StringHandle* asset_name_sh = new HAPI_StringHandle[myAssetCount];
    ENSURE_SUCCESS(session,  HAPI_GetAvailableAssets( session, library_id, asset_name_sh, myAssetCount ) );

	for (int i =0; i < myAssetCount; ++i) {
	    std::string asset_name = get_string( session, asset_name_sh[i] );
		ofmsg("asset %1%: %2%", %(i + 1) %asset_name);
	}

	String s = ostr("%1%", %asset);

	if (myHoudiniGeometrys[s] == NULL) {
		ofwarn("No model of %1%.. creating", %asset);

		HoudiniGeometry* hg = HoudiniGeometry::create(s);
		myHoudiniGeometrys[s] = hg;
		if (mySceneManager->getModel(s) == NULL) {
			mySceneManager->addModel(hg);
		}
	}

	StaticObject* so = new StaticObject(mySceneManager, s);

	// TODO: make this general
	if (mySceneManager->getTexture("testing", false) != NULL) {
		so->setEffect("textured -d white -e white");
		so->getMaterial()->setDiffuseTexture("testing");

	}

	return so;
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

			// TODO: expose these in python to allow changeable options
		    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
			cook_options.cookTemplatedGeos = true; // default false


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
	MenuItem* hMenu = houdiniMenu->addContainer();
	hMenu->getContainer()->setLayout(Container::LayoutVertical);
	// create asset selection container and parameter container
	assetCont = Container::create(Container::LayoutHorizontal, hMenu->getContainer());
	assetCont->setName("AssetCont");

	houdiniCont = Container::create(Container::LayoutFree, hMenu->getContainer());

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

