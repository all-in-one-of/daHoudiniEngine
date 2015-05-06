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
	virtual void clear();

	void dirty();

public:
	HoudiniGeometry(const String& name): ModelGeometry(name) {
// 		osg::VertexBufferObject* vboP = myGeometry->getOrCreateVertexBufferObject();
// 		vboP->setUsage(GL_DYNAMIC_DRAW);
	}
private:
	Ref<osg::Vec3Array> myNormals;

};

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::clear()
{
	if (myColors != NULL) myColors->clear();
	if (myVertices != NULL) myVertices->clear();
	if (myNormals != NULL) myNormals->clear();
	myGeometry->removePrimitiveSet(0, myGeometry->getNumPrimitiveSets());
	myGeometry->dirtyBound();
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::dirty()
{
	if (myColors != NULL) myColors->dirty();
	if (myVertices != NULL) myVertices->dirty();
	if (myNormals != NULL) myNormals->dirty();
}

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

	virtual void handleEvent(const Event& evt);
	virtual void commitSharedData(SharedOStream& out);
	virtual void updateSharedData(SharedIStream& in);

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

	// geometries
	HoudiniGeometry* hg;
// 	ModelGeometry* hg;
	vector<String> geoNames;

	//parameters
	vector<MenuItem> params;

	int mySwitch;

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
	        ofwarn("Could not load %1%", %otl_file);
	        exit(1);
	    }

		ofwarn("HAPI load asset from file %1%", %otl_file);

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
		ofwarn("%1%", %failure.lastErrorMessage());
	throw;
    }
}

HelloApplication::~HelloApplication() {

	delete myAsset;
	if (hg != NULL) delete hg;

    try
    {
	    ENSURE_SUCCESS(HAPI_Cleanup());
		olog(Verbose, "done HAPI");
    }
    catch (hapi::Failure &failure)
    {
		ofwarn("%1%", %failure.lastErrorMessage());
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

				if (mySceneManager->getModel(s) == NULL) {
					hg = HoudiniGeometry::create(s);
					geoNames.push_back(s);
					mySceneManager->addModel(hg);
				} else {
					hg->clear();
				}
				process_geo_part(parts[part_index]);

			}
		}
    }
}

void HelloApplication::process_geo_part(const hapi::Part &part)
{
//     cout << "object " << part.geo.object.id << ", geo " << part.geo.id
// 	<< ", part " << part.id << endl;

	vector<Vector3f> points;
	vector<Vector3f> normals;
	vector<Vector3f> colors;

	bool has_point_normals = false;
	bool has_vertex_normals = false;
	bool has_point_colors = false;
	bool has_vertex_colors = false;
	bool has_primitive_colors = false;

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

    // Print the list of point attribute names.
//     cout << "point attributes:" << endl;
    vector<std::string> point_attrib_names = part.attribNames(
	HAPI_ATTROWNER_POINT);
    for (int attrib_index=0; attrib_index < int(point_attrib_names.size());
	    ++attrib_index) {
// 		cout << "    " << point_attrib_names[attrib_index] << endl;
		if (point_attrib_names[attrib_index] == "P") {
// 		    cout << "point positions:" << endl;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "P", points);
		}
		if (point_attrib_names[attrib_index] == "N") {
// 		    cout << "point normals:" << endl;
			has_point_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "N", normals);
		}
		if (point_attrib_names[attrib_index] == "Cd") {
// 		    cout << "point colors:" << endl;
			has_point_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "Cd", colors);
		}
	}

    // Print the list of point attribute names.
//     cout << "vertex attributes:" << endl;
    vector<std::string> vertex_attrib_names = part.attribNames(
	HAPI_ATTROWNER_VERTEX);
    for (int attrib_index=0; attrib_index < int(vertex_attrib_names.size());
	    ++attrib_index) {
// 		cout << "    " << vertex_attrib_names[attrib_index] << endl;
		if (vertex_attrib_names[attrib_index] == "N") {
// 		    cout << "vertex normals:" << endl;
			has_vertex_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "N", normals);
		}
	}

    // Print the list of point attribute names.
//     cout << "primitive attributes:" << endl;
    vector<std::string> primitive_attrib_names = part.attribNames(
	HAPI_ATTROWNER_PRIM);
    for (int attrib_index=0; attrib_index < int(primitive_attrib_names.size());
	    ++attrib_index) {
// 		cout << "    " << primitive_attrib_names[attrib_index] << endl;
		if (primitive_attrib_names[attrib_index] == "Cd") {
// 		    cout << "primitive colors:" << endl;
			has_primitive_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_PRIM, "Cd", colors);
		}
	}

//     cout << "Number of faces: " << part.info().faceCount << endl;
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
//     cout << "Vertex Indices into Points array:\n";
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

			hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex);

			prev_faceCountIndex = curr_index;
			prev_faceCount = face_counts[ii];
		} else if ((ii > 0) && (prev_faceCount > 4)) {
// 			cout << "making primitive set for " << prev_faceCount << ", plus " <<
// 				prev_faceCountIndex << " to " <<
// 				curr_index - prev_faceCountIndex << endl;
			hg->addPrimitiveOsg(osg::PrimitiveSet::TRIANGLE_FAN,
				prev_faceCountIndex,
			    curr_index - prev_faceCountIndex);

			prev_faceCountIndex = curr_index;
			prev_faceCount = face_counts[ii];
		}

// 		cout << "face (" << face_counts[ii] << "): " << ii << " ";
        for( int jj=0; jj < face_counts[ii]; jj++ )
        {

			int myIndex = curr_index + (face_counts[ii] - jj) % face_counts[ii];
// 			cout << "i: " << vertex_list[myIndex] << " ";

			int lastIndex = hg->addVertex(points[vertex_list[ myIndex ]]);

			if (has_point_normals) {
				hg->addNormal(normals[vertex_list[ myIndex ]]);
			} else if (has_vertex_normals) {
				hg->addNormal(normals[myIndex]);
			}
			if(has_point_colors) {
				hg->addColor(Color(
					colors[vertex_list[ myIndex ]][0],
					colors[vertex_list[ myIndex ]][1],
					colors[vertex_list[ myIndex ]][2],
					1.0
				));
			} else if (has_primitive_colors) {
				hg->addColor(Color(
					colors[ii][0],
					colors[ii][1],
					colors[ii][2],
					1.0
				));
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

	hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex);

	hg->dirty();

    delete[] face_counts;
    delete[] vertex_list;

}


void HelloApplication::process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name, vector<Vector3f>& points)
{
	// useHAPI_AttributeInfo::storage for the data type

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

// 		    cout << attrib_data[
// 			    elem_index * attrib_info.tupleSize + tuple_index]
// 			<< " ";
		}
// 		cout << endl;
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
// 	ModelInfo* torusModel = new ModelInfo("simpleModel", "cyclops/test/torus.fbx", 2.0f);
// 	mySceneManager->loadModel(torusModel);

	// Create a new object using the loaded model (referenced using its name, 'simpleModel')
// 	myObject = new StaticObject(mySceneManager, "simpleModel");
// 	myObject->setName("object");
// 	myObject->setEffect("colored -d #303030 -g 1.0 -s 20.0 -v envmap");
	// Add a selection listener to the object. HelloApplication::onSelectedChanged will be
	// called whenever the object selection state changes.
// 	myObject->pitch(-90 * Math::DegToRad);
// 	myObject->setPosition(0, 0, -2);
// 	myObject->addListener(this);

	// Create the scene editor and add our loaded object to it.
	myEditor = SceneEditorModule::createAndInitialize();
// 	myEditor->addNode(myObject);
	myEditor->setEnabled(false);

// 	// Create a plane for reference.
// 	PlaneShape* plane = new PlaneShape(mySceneManager, 100, 100, Vector2f(50, 50));
// 	plane->setName("ground");
// 	//plane->setEffect("textured -d cyclops/test/checker2.jpg -s 20 -g 1 -v envmap");
// 	plane->setEffect("textured -d cubemaps/gradient1/negy.png");
// 	plane->pitch(-90 * Math::DegToRad);
// 	plane->setPosition(0, 0, -2);

	// Setup a light for the scene.
// 	light->setSoftShadowWidth(0.01f);

	// Setup a light for the scene.
// 	Light* light2 = new Light(mySceneManager);
// 	light2->setEnabled(true);
// 	light2->setPosition(Vector3f(5, 0, -2));
// 	light2->setColor(Color(0.0f, 1.0f, 0.0f));

	// Setup a light for the scene.
// 	Light* light3 = new Light(mySceneManager);
// 	light3->setEnabled(true);
// 	light3->setPosition(Vector3f(0, 0, 3));
// 	light3->setColor(Color(0.0f, 0.0f, 1.0f));

	// Create and initialize the menu manager
	myMenuManager = MenuManager::createAndInitialize();

	// Create the root menu
	ui::Menu* menu = myMenuManager->createMenu("menu");
	myMenuManager->setMainMenu(menu);

	sn = SceneNode::create("myOtl");

    process_assets(*myAsset);

	for (int i = 0; i < geoNames.size(); ++i) {
		StaticObject* hgGeoInstance = new StaticObject(mySceneManager, geoNames[i]);
// 		getEngine()->getScene()->addChild(hgGeoInstance);
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

	// load only the params in the DA Folder
	// first find the index of the folder, then iterate through the list of params
	// from there onwards
	const char* daFolderName = "daFolder_0";

	int daFolderIndex = 0;
	if (myAsset->parmMap().count(daFolderName) > 0) {
		HAPI_ParmId daFolderId = myAsset->parmMap()[daFolderName].info().id;
	// 	cout << "folder id: " << daFolderId << endl;
		while (daFolderIndex < myAsset->parms().size()) {
			if (myAsset->parmMap()[daFolderName].info().id == myAsset->parms()[daFolderIndex].info().id) {
				break;
			}
			daFolderIndex ++;
		}
	}

	for (int i = daFolderIndex; i < myAsset->parms().size(); i++) {
// 	for (int i = 0; i < myAsset->parms().size(); i++) {
		hapi::Parm* parm = &myAsset->parms()[i];

// 		cout << parm->name() << "Parm type: " << parm->info().type << endl;

		if (parm->info().invisible) continue;

		MenuItem* mi = NULL;

		mi = menu->addItem(MenuItem::Label);
		mi->setText(parm->label());

		if (parm->info().size == 1) {
			if (parm->info().type == HAPI_PARMTYPE_INT) {
				int val = parm->getIntValue(0);
				mi->setText(parm->label() + ": " + ostr("%1%", %val));
				mi->setUserTag("label." + parm->name());
				MenuItem* miLabel = mi;
	// 			mi = menu->addItem(MenuItem::Slider);
// 				cout << parm->name() << " max: " << parm->info().max + 1 << endl;;
				mi = menu->addSlider(parm->info().max + 1, "");
				mi->getSlider()->setValue(val);
				mi->setUserData(miLabel);
			} else if (parm->info().type == HAPI_PARMTYPE_FLOAT) {
				float val = parm->getFloatValue(0);
				mi->setText(parm->label() + ": " + ostr("%1%", %val));
				mi->setUserTag("label." + parm->name());
				MenuItem* miLabel = mi;
	// 			mi = menu->addItem(MenuItem::Slider);
// 				cout << parm->name() << " max: " << 100 * (parm->info().max) << endl;;
				mi = menu->addSlider(100 * (parm->info().max), "");
				mi->getSlider()->setValue(int(val) * 100);
				mi->setUserData(miLabel);
			}
		}
		mi->setUserTag(parm->name());
		mi->setListener(this);
	}

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
// 	// If we are not in editing mode, Animate our loaded object.
// 	if(!myEditor->isEnabled())
// 	{
// 		myObject->setPosition(0, 2 + Math::sin(context.time) * 0.5f + 0.5f, -2);
// 		myObject->pitch(context.dt);
// 		myObject->yaw(context.dt);
// 	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::onMenuItemEvent(MenuItem* mi)
{
	bool update = false;

	// is this event generated by the 'manipulate object' menu item?
	if(mi == myEditorMenuItem)
	{
		myEditor->setEnabled(myEditorMenuItem->isChecked());
	}
// 	cout << "parameter: " << mi->getUserTag() << endl;
	if (myAsset->parmMap()[mi->getUserTag()].info().size == 1) {
		if (myAsset->parmMap()[mi->getUserTag()].info().type == HAPI_PARMTYPE_INT) {
			myAsset->parmMap()[mi->getUserTag()].setIntValue(0, mi->getSlider()->getValue());

			((MenuItem*)mi->getUserData())->setText(myAsset->parmMap()[mi->getUserTag()].label() + ": " +
			ostr("%1%", %mi->getSlider()->getValue()));

			update = true;

		} else if (myAsset->parmMap()[mi->getUserTag()].info().type == HAPI_PARMTYPE_FLOAT) {
			myAsset->parmMap()[mi->getUserTag()].setFloatValue(0, 0.01 * mi->getSlider()->getValue());

			((MenuItem*)mi->getUserData())->setText(myAsset->parmMap()[mi->getUserTag()].label() + ": " +
			ostr("%1%", %(mi->getSlider()->getValue() * 0.01)));

			update = true;

		}
	}

	if (update) {
		myAsset->cook();
		wait_for_cook();
		process_assets(*myAsset);
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::onSelectedChanged(SceneNode* source, bool value)
{
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
}

void HelloApplication::wait_for_cook()
{
    int status;
    do
    {
		osleep(100); // sleeping..

        int statusBufSize = 0;
        ENSURE_SUCCESS( HAPI_GetStatusStringBufLength(
            HAPI_STATUS_COOK_STATE, HAPI_STATUSVERBOSITY_ERRORS,
            &statusBufSize ) );
        char * statusBuf = NULL;
        if ( statusBufSize > 0 )
        {
            statusBuf = new char[statusBufSize];
            ENSURE_SUCCESS( HAPI_GetStatusString(
                HAPI_STATUS_COOK_STATE, statusBuf ) );
        }
        if ( statusBuf )
        {
            std::string result( statusBuf );
            ofmsg("cooking...:%1%", %result);
            delete[] statusBuf;
        }
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

///////////////////////////////////////////////////////////////////////////////
void HelloApplication::handleEvent(const Event& evt)
{
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
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::commitSharedData(SharedOStream& out)
{
	out << mySwitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::updateSharedData(SharedIStream& in)
{
 	in >> mySwitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Application entry point
int main(int argc, char** argv)
{
	Application<HelloApplication> app("daOsgHEngine");
    oargs().newNamedString('o', "otl", "otl", "The otl to load", sOtl_file);
	return omain(app, argc, argv);
}

