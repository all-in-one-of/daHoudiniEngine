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

******************************************************************************/


#ifndef __HOUDINI_ENGINE_H__
#define __HOUDINI_ENGINE_H__

#ifdef WIN32
        #ifndef HE_STATIC
                #ifdef daHengine_EXPORTS
                   #define HE_API    __declspec(dllexport)
                #else
                   #define HE_API    __declspec(dllimport)
                #endif
        #else
                #define HE_API
        #endif
#else
        #define HE_API
#endif


#include <cyclops/cyclops.h>
#ifdef DA_ENABLE_HENGINE
#include "HAPI_CPP.h"
#endif

#define OMEGA_NO_GL_HEADERS
#include <omega.h>
#include <omegaOsg/omegaOsg.h>
#include <omegaToolkit.h>

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

namespace houdiniEngine {
	using namespace std;
	using namespace omega;
	using namespace cyclops;
	using namespace omegaToolkit;
	using namespace omegaToolkit::ui;

#ifdef DA_ENABLE_HENGINE
	#define ENSURE_SUCCESS(session, result) \
	    if ((result) != HAPI_RESULT_SUCCESS) \
	    { \
		ofmsg("failure at %1%:%2%", %__FILE__ %__LINE__); \
		ofmsg("%1% '%2%'", %hapi::Failure::lastErrorMessage(session) %result);\
		exit(1); \
	    }

	#define ENSURE_COOK_SUCCESS(session, result) \
	    if ((result) != HAPI_STATE_READY) \
	    { \
		ofmsg("cook failure at %1%:%2%", %__FILE__ %__LINE__); \
		ofmsg("%1% '%2%'", %hapi::Failure::lastErrorMessage(session) %result);\
		exit(1); \
	    }

	static std::string get_string(HAPI_Session* session, int string_handle);

	class HE_API RefAsset: public hapi::Asset, public ReferenceType
	{
	public:
		RefAsset(int id, HAPI_Session* mySession) : Asset(id, mySession)
		{ ofmsg("Constructing refAsset of id %1%", %id); }
		RefAsset(const hapi::Asset &asset) : hapi::Asset(asset)
		{ ofmsg("Constructing refAsset from asset %1%", %asset.name()); }

		~RefAsset()
		{ ofmsg("Destructing refAsset of id %1%", %id); }

	};

#endif
	//forward references
	class HE_API HoudiniGeometry;

	class BillboardCallback;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	class HE_API HoudiniEngine: public EngineModule
	{
	public:
		HoudiniEngine();
		~HoudiniEngine();

		//! Convenience method to create the module, register and initialize it.
		static HoudiniEngine* createAndInitialize();

		virtual void initialize();

#ifdef DA_ENABLE_HENGINE

		virtual void update(const UpdateContext& context);
		virtual void onMenuItemEvent(MenuItem* mi);
		virtual void onSelectedChanged(SceneNode* source, bool value);

		void process_assets(const hapi::Asset &asset);
		void process_geo_part(
			const hapi::Part &part,
			const int objIndex,
			const int geoIndex,
			const int partIndex,
			HoudiniGeometry* hg
		);
		void process_float_attrib(
		    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
		    const char *attrib_name, vector<Vector3f>& points);

		void wait_for_cook();

		virtual void handleEvent(const Event& evt);

		virtual void commitSharedData(SharedOStream& out);
		virtual void updateSharedData(SharedIStream& in);

		int loadAssetLibraryFromFile(const String& otlFile);
		int getAvailableAssetCount() { return myAssetCount; };
		int instantiateAsset(const String& asset);
		int instantiateAssetById(int asset_id);
		StaticObject* instantiateGeometry(const String& asset);

		HoudiniGeometry* getHG(const String& asset) { return myHoudiniGeometrys[asset]; };

		Menu* getMenu(const String& asset) { return NULL; } // return this asset's parameter menu

        void loadParameters(const String& asset_name);
		void createMenu(const String& asset_name);
		void initializeParameters(const String& asset_name);

		float getFps();

		float getTime();
		void setTime(float time);

		void cook();

		void setLoggingEnabled(const bool toggle);

		void showMappings();

		Container* getContainerForAsset(int n);
		Container* getHoudiniCont() { return houdiniCont; };
		Container* getStagingCont() { return stagingCont; };

	private:
		void createMenuItem(const String& asset_name, ui::Menu* menu, hapi::Parm* parm);
		void createParm(const String& asset_name, Container* cont, hapi::Parm* parm);

		SceneManager* mySceneManager;

		// Scene editor. This will be used to manipulate the object.
		SceneEditorModule* myEditor;

		// Menu stuff.
		MenuManager* myMenuManager;
		MenuItem* myQuitMenuItem;

		// Houdini Engine Stuff
	    int library_id; // need to put into an array for multiple libraries?

	    // indicate when to update geos
		bool updateGeos;

		// this is the model definition, not the instance of it
		// I change the verts, faces, normals, etc in this and StaticObjects
		// in the scene get updated accordingly
		typedef Dictionary<String, Ref<HoudiniGeometry> > HGDictionary;
		typedef Dictionary<String, Ref<RefAsset> > Mapping;

		// geometries
		HGDictionary myHoudiniGeometrys;

		// Materials/textures
		vector <Ref<PixelData> > pds;

		// this is only maintained on the master
		Mapping instancedHEAssetsByName;
		vector<Ref <RefAsset> > instancedHEAssetsById;

		//parameters
		// make it look like this:
		//  Houdini Engine > Container
		// Container:
		// ._________._________.
		// | Asset 1 |_Asset_2_|_________.
		// | ._____._____.               |
		// | | FL1 |_FL2_|______.        |
		// | |                  |        |
		// | | A |22| ----||--  |        |
		// | |                  |        |
		// | | B |43| --||----  |        |
		// | |   |11| ||------  |        |
		// | |                  |        |
		// | | C | Hello World ||        |
		// | |__________________|        |
		// |_____________________________|

		typedef struct MenuObject {
			MenuItem* mi;
			Menu* m;
		} MenuObject;

		typedef Dictionary<String, vector<MenuItem> > Menus;
		typedef Dictionary<String, vector<Container*> > ParmConts;
		ui::Menu* houdiniMenu;
		ui::Container* houdiniCont; // the menu container
		ui::Container* assetChoiceCont; // the container to indicate which asset to show
		ui::Container* stagingCont; // the container to show the contents of the selected folder

		Vector<Container*> assetConts; // keep refs to parameters for this asset
		Dictionary<int, Container* > baseConts; // keep refs to submenus
		Dictionary<int, Container* > folderLists; // keep ref to container for Folder selection
		Dictionary<int, Container* > folderListChoices; // buttons to refer to folder lists above
		Dictionary<int, Container* > folderListContents; // keep refs to folderList container to display child parms

		// the link between widget and parmId
		Dictionary < int, int > widgetIdToParmId; // UI Widget -> HAPI_Parm id

		Menus assetParams;
		ParmConts assetParamConts;
		Dictionary<String, pair < Menu*, vector<MenuObject> > > assetParamsMenus;

		// logging
		bool myLogEnabled;

		// session
		HAPI_Session* session;

		int myAssetCount;
		int currentAsset;
		String currentAssetName;

#endif
	};
};

#endif
