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
#include <daHoudiniEngine/houdiniParameter.h>
#if DA_ENABLE_HENGINE > 0
	#include <daHEngine.static.cpp>
#endif
#include <daHoudiniEngine/loaderTools.h>


using namespace houdiniEngine;

#ifdef DA_ENABLE_HENGINE
HoudiniEngine* HoudiniEngine::myInstance = NULL;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// Python wrapper code.
#ifdef OMEGA_USE_PYTHON
#include "omega/PythonInterpreterWrapper.h"
BOOST_PYTHON_MODULE(daHEngine)
{
#if DA_ENABLE_HENGINE > 0
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
 		PYAPI_REF_GETTER(HoudiniEngine, getContainerForAsset)
 		PYAPI_REF_GETTER(HoudiniEngine, getHoudiniCont)
 		PYAPI_REF_GETTER(HoudiniEngine, getStagingCont)
 		PYAPI_REF_GETTER(HoudiniEngine, getHG)
        PYAPI_REF_GETTER(HoudiniEngine, loadParameters)
        PYAPI_METHOD(HoudiniEngine, getIntegerParameterValue)
        PYAPI_METHOD(HoudiniEngine, setIntegerParameterValue)
        PYAPI_METHOD(HoudiniEngine, getFloatParameterValue)
        PYAPI_METHOD(HoudiniEngine, setFloatParameterValue)
        PYAPI_METHOD(HoudiniEngine, getStringParameterValue)
        PYAPI_METHOD(HoudiniEngine, setStringParameterValue);

	// HoudiniGeometry
	PYAPI_REF_BASE_CLASS(HoudiniGeometry)
		PYAPI_METHOD(HoudiniGeometry, getObjectCount)
		PYAPI_METHOD(HoudiniGeometry, getGeodeCount)
		PYAPI_METHOD(HoudiniGeometry, getDrawableCount);

    // HoudiniParameter
    PYAPI_REF_BASE_CLASS(HoudiniParameter)
        PYAPI_METHOD(HoudiniParameter, getId)
        PYAPI_METHOD(HoudiniParameter, getParentId)
        PYAPI_METHOD(HoudiniParameter, getType)
        PYAPI_METHOD(HoudiniParameter, getSize)
        PYAPI_METHOD(HoudiniParameter, getName)
        PYAPI_METHOD(HoudiniParameter, getLabel);

    // HoudiniParameterList
    PYAPI_REF_BASE_CLASS(HoudiniParameterList)
        PYAPI_METHOD(HoudiniParameterList, size)
        PYAPI_REF_GETTER(HoudiniParameterList, getParameter);
#endif
	// tools for generic models exported from houdini
	PYAPI_REF_BASE_CLASS(LoaderTools)
		PYAPI_STATIC_METHOD(LoaderTools, createBillboardNodes)
		PYAPI_STATIC_METHOD(LoaderTools, registerDAPlyLoader);
}
#endif






///////////////////////////////////////////////////////////////////////////////////////////////////
#if DA_ENABLE_HENGINE > 0
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
	if (myInstance == NULL) {
		myInstance = new HoudiniEngine();
		myInstance->setPriority(HoudiniEngine::PriorityHigh);
		ModuleServices::addModule(myInstance);
		myInstance->doInitialize(Engine::instance());
	}

	return myInstance;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniEngine::HoudiniEngine():
#if DA_ENABLE_HENGINE > 0

	EngineModule("HoudiniEngine"),
	mySceneManager(NULL),
	myLogEnabled(false),
	myAssetCount(0),
	currentAsset(-1),
	currentAssetName("")
#else
	EngineModule("HoudiniEngine")
#endif
{
}

HoudiniEngine::~HoudiniEngine()
{
#if DA_ENABLE_HENGINE > 0

	if (SystemManager::instance()->isMaster())
	{
	    try
	    {
		    ENSURE_SUCCESS(session, HAPI_Cleanup(session));
			olog(Verbose, "done HAPI");
			if (session != NULL) {
				delete session;
			}
	    }
	    catch (hapi::Failure &failure)
	    {
			ofwarn("Houdini Failure on cleanup %1%", %failure.lastErrorMessage(session));
			throw;
	    }
	}
#endif
}

#if DA_ENABLE_HENGINE > 0


int HoudiniEngine::loadAssetLibraryFromFile(const String& otlFile)
{
    int assetCount = -1;

	// do this on master only
	if (!SystemManager::instance()->isMaster()) {
        omsg("loadAssetLibraryFromFile: Im a slave, not doing this");
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

	ofmsg("loadAssetLibraryFromFile: %1% assets available", %assetCount);
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
        omsg("instantiateAsset: Im a slave, not doing this");
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



	Ref <RefAsset> myAsset = new RefAsset(asset_id, session);
	instancedHEAssets[asset_id] = myAsset;
    process_assets(*myAsset.get());

	createMenu(asset_id);
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
		omsg("instantiateAssetById: not loading, not slave");
		return -1;
	}

	int assetCount;

    ENSURE_SUCCESS(session,  HAPI_GetAvailableAssetCount( session, library_id, &assetCount ) );

	ofmsg("instantiateAssetById: %1% assets available", %assetCount);
    HAPI_StringHandle* asset_name_sh = new HAPI_StringHandle[assetCount];
    ENSURE_SUCCESS(session,  HAPI_GetAvailableAssets( session, library_id, asset_name_sh, assetCount ) );

	std::string asset_name;

	for (int i =0; i < assetCount; ++i) {
	    asset_name = get_string( session, asset_name_sh[i] );
		ofmsg("%3%asset %1%: %2%", %(i + 1) %asset_name %(i == asset_id ? "*" : ""));
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

	wait_for_cook();

	Ref <RefAsset> myAsset = new RefAsset(asset_id, session);
	instancedHEAssets[asset_id] = myAsset;

	assetNameToIds[asset_name] = asset_id;

	// asset_id is now NOT the same as the id defined in the library
	// is this important?
 	ofmsg("Instantiated %1%, id: %2%", %myAsset.get()->name() %asset_id);

    process_assets(*myAsset.get());

	createMenu(asset_id);
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
	ofmsg("instantiateGeometry: %1% assets available", %myAssetCount);

	if (myHoudiniGeometrys[asset] == NULL) {
		ofwarn("No model of %1%.. creating", %asset);

		HoudiniGeometry* hg = HoudiniGeometry::create(asset);
		myHoudiniGeometrys[asset] = hg;
		if (mySceneManager->getModel(asset) == NULL) {
			mySceneManager->addModel(hg);
		}
	}

	StaticObject* so = new StaticObject(mySceneManager, asset);

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
#if DA_ENABLE_HENGINE > 0
	enableSharedData();

	if (SystemManager::instance()->isMaster()) {
	    try
	    {
			// create sessions
			session = new HAPI_Session();

			const char* env_host = std::getenv("DA_HOUDINI_ENGINE_HOST");
			const char* env_port = std::getenv("DA_HOUDINI_ENGINE_PORT");

			if (session != NULL) {

				int port = env_port ? atoi(env_port) : 7788;
                
				if (!env_host) {
					HAPI_StartThriftSocketServer(true, port, 5000, NULL);
					env_host = "localhost";
				}

				HAPI_CreateThriftSocketSession(session, env_host, port);

				cout << "Connected to " << env_host << ":" << port << endl;
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
	assetChoiceCont = Container::create(Container::LayoutHorizontal, hMenu->getContainer());
	Label* label = Label::create(assetChoiceCont);
	label->setText("assetChoiceCont");
	assetChoiceCont->setName("AssetCont");

	houdiniCont = hMenu->getContainer();

	// outside of the HEngine Menu
	stagingCont = Container::create(Container::LayoutFree, UiModule::instance()->getUi());
	stagingCont->setVisible(false);
	label = Label::create(stagingCont);
	label->setText("stagingCont");

	// Add the 'quit' menu item. Menu items can either run callbacks when activated, or they can
	// run script commands. In this case, we make the menu item call the 'oexit' script command.
	// oexit is the standard omegalib script command used to terminate the application.
	// Using scripting with menu items can be extremely powerful and flexible, and limits the amount
	// of callback code that needs to be written to handle ui events.
	myQuitMenuItem = menu->addItem(MenuItem::Button);
	myQuitMenuItem->setText("Quit");
	myQuitMenuItem->setCommand("oexit()");
#endif

}

#endif


