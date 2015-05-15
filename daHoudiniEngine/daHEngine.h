#ifndef __HOUDINI_ENGINE_H__
#define __HOUDINI_ENGINE_H__

// #include "omega/osystem.h"
// #include "omega/Application.h"

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

	#define ENSURE_SUCCESS(result) \
	    if ((result) != HAPI_RESULT_SUCCESS) \
	    { \
		ofmsg("failure at %1%:%2%", %__FILE__ %__LINE__); \
		ofmsg("%1%", %hapi::Failure::lastErrorMessage());\
		exit(1); \
	    }

	#define ENSURE_COOK_SUCCESS(result) \
	    if ((result) != HAPI_STATE_READY) \
	    { \
		ofmsg("failure at %1%:%2%", %__FILE__ %__LINE__); \
		ofmsg("%1%", %hapi::Failure::lastErrorMessage());\
		exit(1); \
	    }

	static std::string get_string(int string_handle);

	String sOtl_file;

	class HE_API RefAsset: public hapi::Asset, public ReferenceType
	{
	public:
		RefAsset(int id) : Asset(id)
		{}
		RefAsset(const hapi::Asset &asset) : hapi::Asset(asset)
		{}

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

		void process_assets(Ref <RefAsset> &refAsset);
		void process_geo_part(const hapi::Part &part, HoudiniGeometry* hg);
		void process_float_attrib(
		    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
		    const char *attrib_name, vector<Vector3f>& points);

		void wait_for_cook();

		virtual void handleEvent(const Event& evt);
		virtual void commitSharedData(SharedOStream& out);
		virtual void updateSharedData(SharedIStream& in);

		int loadAssetLibraryFromFile(const String& otlFile);
		int instantiateAsset(const String& asset);
		StaticObject* instantiateGeometry(const String& asset);

		void createMenu(const String& asset_name);

		bool hasHG(const String &s);
		void getHGInfo(const String &s);

	private:
		SceneManager* mySceneManager;
		StaticObject* myObject;

		SceneNode* sn;

		// Scene editor. This will be used to manipulate the object.
		SceneEditorModule* myEditor;

		// Menu stuff.
		MenuManager* myMenuManager;
		MenuItem* myQuitMenuItem;
		MenuItem* myEditorMenuItem;

		// Houdini Engine Stuff
	    int library_id; // need to put into an array for multiple libraries?
	    Ref <RefAsset> myAsset;

		bool updateGeos;

		// this is the model definition, not the instance of it
		// I change the verts, faces, normals, etc in this and StaticObjects
		// in the scene get updated accordingly
		// name in form: 'Asset objectNum geoNum partNum'

		typedef Dictionary<String, Ref<HoudiniGeometry> > HGDictionary;
		typedef Dictionary<String, Ref<RefAsset> > Mapping;

		// geometries
		HGDictionary myHoudiniGeometrys;

		// this is only maintained on the master
		Mapping instancedHEAssets;

		//parameters
		vector<MenuItem> params;

	};
};

#endif
