#ifndef __HOUDINI_ENGINE_H__
#define __HOUDINI_ENGINE_H__


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

	//forward references
	class HoudiniGeometry;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	class HoudiniEngine: public EngineModule, IMenuItemListener, SceneNodeListener
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
		void process_geo_part(const hapi::Part &part);
		void process_float_attrib(
		    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
		    const char *attrib_name, vector<Vector3f>& points);

		void wait_for_cook();

		virtual void handleEvent(const Event& evt);
		virtual void commitSharedData(SharedOStream& out);
		virtual void updateSharedData(SharedIStream& in);

		int loadAssetLibraryFromFile(const String& otlFile);
		int instantiateAsset(const String& asset);
		void instantiateGeometry(const String& asset, const int geoNum, const int partNum);

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
	    hapi::Asset* myAsset;
		int asset_id;

		// geometries
		HoudiniGeometry* hg;
	// 	ModelGeometry* hg;
		vector<String> geoNames;

		//parameters
		vector<MenuItem> params;

		int mySwitch;

	};
};

#endif
