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
#include "HAPI_CPP.h"

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

	#define ENSURE_SUCCESS(session, result) \
	    if ((result) != HAPI_RESULT_SUCCESS) \
	    { \
		ofmsg("failure at %1%:%2%", %__FILE__ %__LINE__); \
		ofmsg("%1%", %hapi::Failure::lastErrorMessage(session));\
		exit(1); \
	    }

	#define ENSURE_COOK_SUCCESS(session, result) \
	    if ((result) != HAPI_STATE_READY) \
	    { \
		ofmsg("failure at %1%:%2%", %__FILE__ %__LINE__); \
		ofmsg("%1%", %hapi::Failure::lastErrorMessage(session));\
		exit(1); \
	    }

	static std::string get_string(HAPI_Session* session, int string_handle);

	String sOtl_file;

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

	//forward references
	class HE_API HoudiniGeometry;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	class HE_API HoudiniEngine: public EngineModule, IMenuItemListener, SceneNodeListener
	{
	public:
		HoudiniEngine();
		~HoudiniEngine();

		//! Convenience method to create the module, register and initialize it.
		static HoudiniEngine* createAndInitialize();

		virtual void initialize();
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
		int instantiateAsset(const String& asset);
		int instantiateAssetById(int asset_id);
		StaticObject* instantiateGeometry(const String& asset);

		void createMenu(const String& asset_name);
		void initializeParameters(const String& asset_name);

		float getFps();

		float getTime();
		void setTime(float time);

		void cook();

		void setLoggingEnabled(const bool toggle);

	private:
		void createMenuItem(ui::Menu* menu, const String& asset_name, hapi::Parm* parm, int index);

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
		typedef Dictionary<String, vector<MenuItem> > Menus;

		// geometries
		HGDictionary myHoudiniGeometrys;

		// Materials/textures
		vector <Ref<PixelData> > pds;

		// this is only maintained on the master
		Mapping instancedHEAssets;

		//parameters
		ui::Menu* houdiniMenu;
		Menus assetParams;

		// logging
		bool myLogEnabled;

		// session

		HAPI_Session* session;
		HAPI_Session mySession;

	};
};

#endif
