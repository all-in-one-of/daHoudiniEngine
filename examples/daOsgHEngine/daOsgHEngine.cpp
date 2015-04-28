/**************************************************************************************************
 * THE OMEGA LIB PROJECT
 *-------------------------------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:
 *  Alessandro Febretti		febret@gmail.com
 *-------------------------------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-------------------------------------------------------------------------------------------------
 *	cyhello2
 *		A slightly more advanced cyclops application. Loads a mesh and lets the user manipulate it.
 *		This demo introduces the use of the SceneEditor classes and the menu system.
 **************************************************************************************************/
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

// TODO: look at cyclops/ModelGeometry.cpp, use something like this class to make
// houdini engine objects, and then add some parameter manipulation in a menu
// also, see if display lists help or not..

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

class HoudiniGeometry : public ModelGeometry
{
public:
	static HoudiniGeometry* create(const String& name)
	{
		return new HoudiniGeometry(name);
	}

	int addNormal(const Vector3f& v);
	void setNormal(int index, const Vector3f& v);
	Vector3f getNormal(int index);

public:
	HoudiniGeometry(const String& name): ModelGeometry(name) {
	}
private:
	Ref<osg::Vec3Array> myNormals;

};


///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addNormal(const Vector3f& v)
{
	if(myNormals == NULL) {
		myNormals = new osg::Vec3Array();
		myGeometry->setNormalArray(myNormals);
		myGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
	}

	myNormals->push_back(osg::Vec3d(v[0], v[1], v[2]));
// 	myGeometry->dirtyBound();
	return myNormals->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setNormal(int index, const Vector3f& v)
{
	oassert(myNormals->size() > index);
	osg::Vec3f& c = myNormals->at(index);
	c[0] = v[0];
	c[1] = v[1];
	c[2] = v[2];
	myNormals->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Vector3f HoudiniGeometry::getNormal(int index)
{
	oassert(myNormals->size() > index);
	const osg::Vec3f& c = myNormals->at(index);
	return Vector3f(c[0], c[1], c[2]);
}


static std::string get_string(int string_handle);

String sOtl_file;

///////////////////////////////////////////////////////////////////////////////////////////////////
class HelloApplication: public EngineModule, IMenuItemListener, SceneNodeListener
{
public:
	HelloApplication();
	~HelloApplication();

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
    int library_id;
    hapi::Asset* myAsset;
	int asset_id;

	HoudiniGeometry* hg;
	vector<String> geoNames;

};


///////////////////////////////////////////////////////////////////////////////////////////////////
HelloApplication::HelloApplication():
	EngineModule("HelloApplication"),
	mySceneManager(NULL)
{
    try
    {

		const char *otl_file; // = "test_asset.otl";

		ofmsg("file is %1%", %sOtl_file);


		if (sOtl_file == "") {
			owarn ("using test_asset.otl");
			otl_file = "test_asset.otl";
		} else {
			otl_file = sOtl_file.c_str();
		}

	    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();

	    ENSURE_SUCCESS(HAPI_Initialize(
		getenv("$HOME"),
		/*dso_search_path=*/NULL,
	        &cook_options,
		/*use_cooking_thread=*/true,
		/*cooking_thread_max_size=*/-1));

	    if (HAPI_LoadAssetLibraryFromFile(
	            otl_file,
	            &library_id) != HAPI_RESULT_SUCCESS)
	    {
	        cout << "Could not load " << otl_file << endl;
	        exit(1);
	    }

		cout << "HAPI load asset from file " << otl_file << endl;

	    HAPI_StringHandle asset_name_sh;
	    ENSURE_SUCCESS( HAPI_GetAvailableAssets( library_id, &asset_name_sh, 1 ) );
	    std::string asset_name = get_string( asset_name_sh );

	    ENSURE_SUCCESS(HAPI_InstantiateAsset(
	            asset_name.c_str(),
	            /* cook_on_load */ true,
	            &asset_id ));

		wait_for_cook();

	    HAPI_AssetInfo asset_info;

	    ENSURE_SUCCESS( HAPI_GetAssetInfo( asset_id, &asset_info ) );

		myAsset = new hapi::Asset(asset_id); // make this a ref pointer later

    }
    catch (hapi::Failure &failure)
    {
	cout << failure.lastErrorMessage() << endl;
	throw;
    }
}

HelloApplication::~HelloApplication() {

	delete myAsset;

    try
    {
	    ENSURE_SUCCESS(HAPI_Cleanup());
		cout << "done HAPI" << endl;
    }
    catch (hapi::Failure &failure)
    {
		cout << failure.lastErrorMessage() << endl;
		throw;
    }
}

void HelloApplication::process_assets(const hapi::Asset &asset)
{
    vector<hapi::Object> objects = asset.objects();
    for (int object_index=0; object_index < int(objects.size()); ++object_index)
    {
		vector<hapi::Geo> geos = objects[object_index].geos();

		for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
		{
		    vector<hapi::Part> parts = geos[geo_index].parts();

		    for (int part_index=0; part_index < int(parts.size()); ++part_index)
			{

				String s = "geo" + part_index;

				hg = HoudiniGeometry::create(s);
				geoNames.push_back(s);
				process_geo_part(parts[part_index]);
				mySceneManager->addModel(hg);
			}
		}
    }
}

void HelloApplication::process_geo_part(const hapi::Part &part)
{
    cout << "object " << part.geo.object.id << ", geo " << part.geo.id
	 << ", part " << part.id << endl;

    // Print the list of point attribute names.
    cout << "attributes:" << endl;
    vector<std::string> attrib_names = part.attribNames(
	HAPI_ATTROWNER_POINT);
    for (int attrib_index=0; attrib_index < int(attrib_names.size());
	    ++attrib_index)
		cout << "    " << attrib_names[attrib_index] << endl;

    cout << "point positions:" << endl;

	vector<Vector3f> points;
	vector<Vector3f> normals;

	bool has_normals = false;

	// TODO: better process normals, on HAPI_ATTROWNER_VERTEX, or PRIMITIVE

    process_float_attrib(part, HAPI_ATTROWNER_POINT, "P", points);

    for (int attrib_index=0; attrib_index < int(attrib_names.size());
	    ++attrib_index) {
		if (attrib_names[attrib_index] == "N") {
		    cout << "point normals:" << endl;
			has_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "N", normals);
		}
	}

    cout << "Number of faces: " << part.info().faceCount << endl;
    int * face_counts = new int[ part.info().faceCount ];
    ENSURE_SUCCESS( HAPI_GetFaceCounts(
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		face_counts,
		0,
		part.info().faceCount
	) );

    int * vertex_list = new int[ part.info().vertexCount ];
    ENSURE_SUCCESS( HAPI_GetVertexList(
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		vertex_list, 0, part.info().vertexCount ) );
    cout << "Vertex Indices into Points array:\n";
    int curr_index = 0;

// 	hg->setVertexListSize(pointCount);

	int prev_faceCount = face_counts[0];
    int prev_faceCountIndex = 0;

	// TODO: get primitive set working for different triangles..
	// TODO: get normals working from Vertex set.

    for( int ii=0; ii < part.info().faceCount; ii++ )
    {
        for( int jj=0; jj < face_counts[ii]; jj++ )
        {

// 			hg->setVertex(curr_index,points[vertex_list[ curr_index ]]);
			hg->addVertex(points[vertex_list[ curr_index ]]);
			if (has_normals) {
				hg->addNormal(-normals[vertex_list[ curr_index ]]);
	            cout << "normal :" << curr_index << ", belonging to face: " << ii <<", index: "
	                << vertex_list[ curr_index ] << " of normals array, being:" <<
	                hg->getNormal(curr_index) << "\n";
			}
			hg->addColor(Color(0.8f, 0.8f, 0.8f, 1.0));

            cout << "vertex :" << curr_index << ", belonging to face: " << ii <<", index: "
                << vertex_list[ curr_index ] << " of points array, being:" <<
                points[vertex_list[ curr_index ]] << "\n";
            curr_index++;
        }

		if (face_counts[ii] != prev_faceCount) {
			osg::PrimitiveSet::Mode myType;

			if (face_counts[ii] == 3) {
				myType = osg::PrimitiveSet::TRIANGLES;
			} else if (face_counts[ii] == 4) {
				myType = osg::PrimitiveSet::QUADS;
			} else if (face_counts[ii] > 4) {
				cout << "face count is " << face_counts[ii] << endl;
				myType = osg::PrimitiveSet::TRIANGLE_FAN;
			}

			cout << "making primitive set for " << face_counts[ii] << ", from " <<
				prev_faceCountIndex << " to " <<
				1 + curr_index << endl;

			hg->addPrimitiveOsg(myType, prev_faceCountIndex, 1 + curr_index);
			prev_faceCountIndex = curr_index + 1;
			prev_faceCount = face_counts[ii];
		}
    }

	osg::PrimitiveSet::Mode myType;

	if (prev_faceCount == 3) {
		myType = osg::PrimitiveSet::TRIANGLES;
	} else if (prev_faceCount == 4) {
		myType = osg::PrimitiveSet::QUADS;
	} else if (prev_faceCount > 4) {
		cout << "face count is " << prev_faceCount << endl;
		myType = osg::PrimitiveSet::TRIANGLE_FAN;
	}

    hg->addPrimitiveOsg(myType, prev_faceCountIndex, 1 + curr_index);

    delete[] face_counts;
    delete[] vertex_list;

}


void HelloApplication::process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name, vector<Vector3f>& points)
{
    // Get the attribute values.
    HAPI_AttributeInfo attrib_info = part.attribInfo(attrib_owner, attrib_name);
    float *attrib_data = part.getNewFloatAttribData(attrib_info, attrib_name);

	points.clear();
	points.resize(attrib_info.count);
    for (int elem_index=0; elem_index < attrib_info.count; ++elem_index)
    {

		Vector3f v;
		for (int tuple_index=0; tuple_index < attrib_info.tupleSize;
			++tuple_index)
		{

			v[tuple_index] = attrib_data[elem_index * attrib_info.tupleSize + tuple_index ];

		    cout << attrib_data[
			    elem_index * attrib_info.tupleSize + tuple_index]
			<< " ";
		}
		cout << endl;
		points[elem_index] = v;
    }

    delete [] attrib_data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::initialize()
{
	// Create and initialize the cyclops scene manager.
	mySceneManager = SceneManager::createAndInitialize();

	// Load some basic model from the omegalib meshes folder.
	// Force the size of the object to be 0.8 meters
	//ModelInfo torusModel("simpleModel", "demos/showcase/data/walker/walker.fbx", 2.0f);
	ModelInfo* torusModel = new ModelInfo("simpleModel", "cyclops/test/torus.fbx", 2.0f);
	mySceneManager->loadModel(torusModel);

	// Create a new object using the loaded model (referenced using its name, 'simpleModel')
	myObject = new StaticObject(mySceneManager, "simpleModel");
	myObject->setName("object");
	myObject->setEffect("colored -d #303030 -g 1.0 -s 20.0 -v envmap");
	// Add a selection listener to the object. HelloApplication::onSelectedChanged will be
	// called whenever the object selection state changes.
	myObject->pitch(-90 * Math::DegToRad);
	myObject->setPosition(0, 0, -2);
	myObject->addListener(this);

	// Create the scene editor and add our loaded object to it.
	myEditor = SceneEditorModule::createAndInitialize();
	myEditor->addNode(myObject);
	myEditor->setEnabled(false);

// 	// Create a plane for reference.
// 	PlaneShape* plane = new PlaneShape(mySceneManager, 100, 100, Vector2f(50, 50));
// 	plane->setName("ground");
// 	//plane->setEffect("textured -d cyclops/test/checker2.jpg -s 20 -g 1 -v envmap");
// 	plane->setEffect("textured -d cubemaps/gradient1/negy.png");
// 	plane->pitch(-90 * Math::DegToRad);
// 	plane->setPosition(0, 0, -2);

	// Setup a light for the scene.
	Light* light = new Light(mySceneManager);
	light->setEnabled(true);
	light->setPosition(Vector3f(0, 5, -2));
	light->setColor(Color(1.0f, 1.0f, 1.0f));
	light->setAmbient(Color(0.1f, 0.1f, 0.1f));
	//light->setSoftShadowWidth(0.01f);

	// Setup a light for the scene.
	Light* light2 = new Light(mySceneManager);
	light2->setEnabled(true);
	light2->setPosition(Vector3f(5, 0, -2));
	light2->setColor(Color(0.0f, 1.0f, 0.0f));

	// Setup a light for the scene.
	Light* light3 = new Light(mySceneManager);
	light3->setEnabled(true);
	light3->setPosition(Vector3f(0, 0, 3));
	light3->setColor(Color(0.0f, 0.0f, 1.0f));

	// Create and initialize the menu manager
	myMenuManager = MenuManager::createAndInitialize();

	// Create the root menu
	ui::Menu* menu = myMenuManager->createMenu("menu");
	myMenuManager->setMainMenu(menu);

	sn = SceneNode::create("myOtl");

    process_assets(*myAsset);

	for (int i = 0; i < geoNames.size(); ++i) {
		StaticObject* hgGeoInstance = new StaticObject(mySceneManager, geoNames[i]);
// 		sn->addChild(hgGeoInstance);
	}
	myEditor->addNode(sn);

	// Add the 'manipulate object' menu item. This item will be a checkbox. We attach this class as
	// a listener for the menu item: When the item is activated, the menu system will call
	// HelloApplication::onMenuItemEvent
	myEditorMenuItem = menu->addItem(MenuItem::Checkbox);
	myEditorMenuItem->setText("Manipulate object");
	myEditorMenuItem->setChecked(false);
	myEditorMenuItem->setListener(this);

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
void HelloApplication::update(const UpdateContext& context)
{
	// If we are not in editing mode, Animate our loaded object.
	if(!myEditor->isEnabled())
	{
		myObject->setPosition(0, 2 + Math::sin(context.time) * 0.5f + 0.5f, -2);
		myObject->pitch(context.dt);
		myObject->yaw(context.dt);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::onMenuItemEvent(MenuItem* mi)
{
	// is this event generated by the 'manipulate object' menu item?
	if(mi == myEditorMenuItem)
	{
		myEditor->setEnabled(myEditorMenuItem->isChecked());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::onSelectedChanged(SceneNode* source, bool value)
{
	if(source == myObject)
	{
		if(myObject->isSelected())
		{
			// Color the object yellow when selected.
			myObject->setEffect("colored -d #ffff30 -g 1.0 -s 20.0");
		}
		else
		{
			// Color the object gray when not selected.
			myObject->setEffect("colored -d #303030 -g 1.0 -s 20.0");
		}
	}
}

void HelloApplication::wait_for_cook()
{
    int status;
    do
    {
		osleep(100); // sleeping..
        HAPI_GetStatus(HAPI_STATUS_COOK_STATE, &status);
    }
    while ( status > HAPI_STATE_MAX_READY_STATE );
    ENSURE_COOK_SUCCESS( status );
}

static std::string get_string(int string_handle)
{
    // A string handle of 0 means an invalid string handle -- similar to
    // a null pointer.  Since we can't return NULL, though, return an empty
    // string.
    if (string_handle == 0)
	return "";

    int buffer_length;
    ENSURE_SUCCESS(HAPI_GetStringBufLength(string_handle, &buffer_length));

    char * buf = new char[ buffer_length ];

    ENSURE_SUCCESS(HAPI_GetString(string_handle, buf, buffer_length));

    std::string result( buf );
    delete[] buf;
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Application entry point
int main(int argc, char** argv)
{
	Application<HelloApplication> app("daOsgHEngine");
    oargs().newNamedString('o', "otl", "otl", "The otl to load", sOtl_file);
	return omain(app, argc, argv);
}

