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

namespace houdiniEngine {
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

		inline int getNormalCount() { return (myNormals == NULL) ? 0 : myNormals->size(); }
		inline int getVertexCount() { return myVertices->size(); }
		inline int getColorCount() { return (myColors == NULL) ? 0 : myColors->size(); }

		inline int getPrimitiveSetCount() { return myGeometry->getPrimitiveSetList().size(); }

		void dirty();

	public:
		HoudiniGeometry(const String& name): ModelGeometry(name) {
	// 		osg::VertexBufferObject* vboP = myGeometry->getOrCreateVertexBufferObject();
	// 		vboP->setUsage(GL_DYNAMIC_DRAW);
		}
	private:
		Ref<osg::Vec3Array> myNormals;

	};
};

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
 		PYAPI_METHOD(HoudiniEngine, hasHG)
 		PYAPI_METHOD(HoudiniEngine, getHGInfo)
 		PYAPI_REF_GETTER(HoudiniEngine, instantiateGeometry)
		;
}
#endif

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

///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniEngine* HoudiniEngine::createAndInitialize()
{
	// Initialize and register the HoudiniEngine module.
	HoudiniEngine* instance = new HoudiniEngine();
	ModuleServices::addModule(instance);
	instance->doInitialize(Engine::instance());
	return instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HoudiniEngine::HoudiniEngine():
	EngineModule("HoudiniEngine"),
	mySceneManager(NULL)
{
}

HoudiniEngine::~HoudiniEngine()
{
	if (SystemManager::instance()->isMaster())
	{
	    try
	    {
		    ENSURE_SUCCESS(HAPI_Cleanup());
			olog(Verbose, "done HAPI");
	    }
	    catch (hapi::Failure &failure)
	    {
			ofwarn("Houdini Failure on cleanup %1%", %failure.lastErrorMessage());
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
            otlFile.c_str(),
            &library_id);
    if (hr != HAPI_RESULT_SUCCESS)
    {
        ofwarn("Could not load %1%", %otlFile);
        ofwarn("Result is %1%", %hr);
		return assetCount;
    }

    ENSURE_SUCCESS( HAPI_GetAvailableAssetCount( library_id, &assetCount ) );

	ofmsg("%1% assets available", %assetCount);

    return assetCount;

}

void HoudiniEngine::createMenu(const String& asset_name)
{
	// load only the params in the DA Folder
	// first find the index of the folder, then iterate through the list of params
	// from there onwards
	const char* daFolderName = "daFolder_0";

	int daFolderIndex = 0;
	if (myAsset.get()->parmMap().count(daFolderName) > 0) {
		HAPI_ParmId daFolderId = myAsset.get()->parmMap()[daFolderName].info().id;
	// 	cout << "folder id: " << daFolderId << endl;
		while (daFolderIndex < myAsset.get()->parms().size()) {
			if (myAsset.get()->parmMap()[daFolderName].info().id == myAsset.get()->parms()[daFolderIndex].info().id) {
				break;
			}
			daFolderIndex ++;
		}
	}

	for (int i = daFolderIndex; i < myAsset.get()->parms().size(); i++) {
		hapi::Parm* parm = &myAsset.get()->parms()[i];

// 		cout << parm->name() << "Parm type: " << parm->info().type << endl;

		if (parm->info().invisible) continue;

		MenuItem* mi = NULL;

		Menu* menu = MenuManager::instance()->getMainMenu();

		mi = menu->addItem(MenuItem::Label);
		mi->setText(parm->label());

// 		if (parm->info().size == 1) {
// 			if (parm->info().type == HAPI_PARMTYPE_INT) {
// 				int val = parm->getIntValue(0);
// 				mi->setText(parm->label() + ": " + ostr("%1%", %val));
// 				mi->setUserTag("label." + parm->name());
// 				MenuItem* miLabel = mi;
// 				mi = menu->addSlider(parm->info().max + 1, "");
// 				mi->getSlider()->setValue(val);
// 				mi->setUserData(miLabel);
// 			} else if (parm->info().type == HAPI_PARMTYPE_FLOAT) {
// 				float val = parm->getFloatValue(0);
// 				mi->setText(parm->label() + ": " + ostr("%1%", %val));
// 				mi->setUserTag("label." + parm->name());
// 				MenuItem* miLabel = mi;
// 				mi = menu->addSlider(100 * (parm->info().max), "");
// 				mi->getSlider()->setValue(int(val) * 100);
// 				mi->setUserData(miLabel);
// 			}
// 		}
// 		mi->setUserTag(parm->name());
// 		mi->setListener(this);

		// TODO: generalise this so the following works:
		// check for uiMin/uiMax, then use sliders
		// use text boxes for vectors and non min/max things
		// use submenus/containers for choices
		// use the joinNext variable for displaying items
		// use checkbox for HAPI_PARMTYPE_TOGGLE
		// use text box for string
		// do multiparms
		if (parm->info().size == 1) {
			if (parm->info().type == HAPI_PARMTYPE_INT) {
				int val = parm->getIntValue(0);
				mi->setText(parm->label() + ": " + ostr("%1%", %val));
				mi->setUserTag("label." + parm->name());
				MenuItem* miLabel = mi;
				mi = menu->addSlider(parm->info().max + 1, "");
				mi->getSlider()->setValue(val);
				mi->setUserData(miLabel);
			} else if (parm->info().type == HAPI_PARMTYPE_FLOAT) {
				float val = parm->getFloatValue(0);
				mi->setText(parm->label() + ": " + ostr("%1%", %val));
				mi->setUserTag("label." + parm->name());
				MenuItem* miLabel = mi;
				mi = menu->addSlider(100 * (parm->info().max), "");
				mi->getSlider()->setValue(int(val) * 100);
				// use userData to store ref to the label, for changing values
				// when updating
				mi->setUserData(miLabel);
			}
			mi->setUserTag(parm->name());
			mi->setListener(this);
		} else {
			if (parm->info().type == HAPI_PARMTYPE_FLOAT) {
				mi->setText(parm->label());
				mi->setUserTag("label." + parm->name());
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
				}
			}
		}
	}
}

bool HoudiniEngine::hasHG(const String &s) {
	return (myHoudiniGeometrys[s] != NULL);
}

void HoudiniEngine::getHGInfo(const String &s) {
	if (myHoudiniGeometrys[s] == NULL) {
		return;
	}

	HoudiniGeometry* hg = myHoudiniGeometrys[s];

	ofmsg("name: %1%", %hg->getName());
	ofmsg("verts: %1%", %hg->getVertexCount());
	ofmsg("normals: %1%", %hg->getNormalCount());
	ofmsg("cols: %1%", %hg->getColorCount());

}

int HoudiniEngine::instantiateAsset(const String& asset_name)
{
// 	// this is code to instantiate an asset
//     HAPI_StringHandle asset_name_sh;
//     ENSURE_SUCCESS( HAPI_GetAvailableAssets( library_id, &asset_name_sh, 1 ) );
//     std::string asset_name = get_string( asset_name_sh );

	if (!SystemManager::instance()->isMaster()) {
		return -1;
	}

	int asset_id = 0;

    ENSURE_SUCCESS(HAPI_InstantiateAsset(
            asset_name.c_str(),
            /* cook_on_load */ true,
            &asset_id ));

	wait_for_cook();

//     HAPI_AssetInfo asset_info;
//
//     ENSURE_SUCCESS( HAPI_GetAssetInfo( asset_id, &asset_info ) );

	myAsset = new RefAsset(asset_id);
	instancedHEAssets[asset_name] = myAsset;
    process_assets(myAsset);

	updateGeos = true;
	return 0;
}

// geometry should be instantiated like this:
// lib/otl node
// |
// o- asset node+
//     |
//     o- object node+
//         |
//         o- geo node+
//             |
//             o- part node+
// StaticObject* HoudiniEngine::instantiateGeometry(const String& asset, const int geoNum, const int partNum)
// HoudiniGeometry doesn't seem to be properly instantiated on all nodes. Why?
// No geometry is drawn..
StaticObject* HoudiniEngine::instantiateGeometry(const String& asset)
{
	int geoNum = 0;
	int partNum = 0;
	String s = ostr("%1% 0 %2% %3%", %asset %geoNum %partNum);

	if (myHoudiniGeometrys[s] == NULL) {
		ofwarn("No model of %1% with geo %2% and part %3%.. creating", %asset %geoNum %partNum);

		HoudiniGeometry* hg = HoudiniGeometry::create(s);
		myHoudiniGeometrys[s] = hg;
		if (mySceneManager->getModel(s) == NULL) {
			mySceneManager->addModel(hg);
		}
	}

	ofmsg("making instance %1%", %s);
	return new StaticObject(mySceneManager, s);
}

// void HoudiniEngine::process_assets(const hapi::Asset &asset)
void HoudiniEngine::process_assets(Ref <RefAsset> &refAsset)
{
	hapi::Asset* asset = refAsset.get();

    vector<hapi::Object> objects = asset->objects();
    for (int object_index=0; object_index < int(objects.size()); ++object_index)
    {
		vector<hapi::Geo> geos = objects[object_index].geos();

		for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
		{
		    vector<hapi::Part> parts = geos[geo_index].parts();

		    for (int part_index=0; part_index < int(parts.size()); ++part_index)
			{

				String s = ostr("%1% %2% %3% %4%",
					%asset->name()
					%object_index
					%geo_index
					%part_index
				);

				HoudiniGeometry* hg;

				if (myHoudiniGeometrys[s] == NULL) {
					ofmsg("making hg: '%1%'", %s);
					hg = HoudiniGeometry::create(s);

					ofmsg("made hg: '%1%'", %s);
					myHoudiniGeometrys[s] = hg;
					ofmsg("assigned hg: '%1%'", %s);

					if (mySceneManager->getModel(s) == NULL) {
						mySceneManager->addModel(hg);
					}
					ofmsg("added to scene: '%1%'", %s);
				}
				process_geo_part(parts[part_index], hg);
			}
		}
    }
}

// void HoudiniEngine::process_geo_part(const hapi::Part &part)
void HoudiniEngine::process_geo_part(const hapi::Part &part, HoudiniGeometry* hg)
{
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
	}

    // Print the list of point attribute names.
    vector<std::string> vertex_attrib_names = part.attribNames(
	HAPI_ATTROWNER_VERTEX);
    for (int attrib_index=0; attrib_index < int(vertex_attrib_names.size());
	    ++attrib_index) {
		if (vertex_attrib_names[attrib_index] == "N") {
			has_vertex_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "N", normals);
		}
	}

    // Print the list of point attribute names.
    vector<std::string> primitive_attrib_names = part.attribNames(
	HAPI_ATTROWNER_PRIM);
    for (int attrib_index=0; attrib_index < int(primitive_attrib_names.size());
	    ++attrib_index) {
		if (primitive_attrib_names[attrib_index] == "Cd") {
			has_primitive_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_PRIM, "Cd", colors);
		}
	}

//     cout << "Number of faces: " << part.info().faceCount << endl;
    int *face_counts = new int[ part.info().faceCount ];
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


void HoudiniEngine::process_float_attrib(
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
void HoudiniEngine::initialize()
{
	enableSharedData();

	if (SystemManager::instance()->isMaster()) {
	    try
	    {
		    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();

		    ENSURE_SUCCESS(HAPI_Initialize(
			getenv("$HOME"),
			/*dso_search_path=*/NULL,
		        &cook_options,
			/*use_cooking_thread=*/true,
			/*cooking_thread_max_size=*/-1));
	    }
	    catch (hapi::Failure &failure)
	    {
			ofwarn("Houdini Failure.. %1%", %failure.lastErrorMessage());
			throw;
	    }
	}

	// Create and initialize the cyclops scene manager.
	mySceneManager = SceneManager::createAndInitialize();

	// Create the scene editor and add our loaded object to it.
	myEditor = SceneEditorModule::createAndInitialize();
// 	myEditor->addNode(myObject);
	myEditor->setEnabled(false);

	// Create and initialize the menu manager
	myMenuManager = MenuManager::createAndInitialize();

	// Create the root menu
	ui::Menu* menu = myMenuManager->createMenu("menu");
	myMenuManager->setMainMenu(menu);

	sn = SceneNode::create("myOtl");
	myEditor->addNode(sn);

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
	bool update = false;

	// is this event generated by the 'manipulate object' menu item?
	if(mi == myEditorMenuItem)
	{
		myEditor->setEnabled(myEditorMenuItem->isChecked());
	}
	cout << "parameter: " << mi->getUserTag() << endl;

	int z = mi->getUserTag().find_last_of(" ");
	cout << "first parameter: " << mi->getUserTag().substr(0, z) << endl;
	cout << "last parameter: " << mi->getUserTag().substr(z + 1) << endl;
// 	if (myAsset->parmMap()[mi->getUserTag()].info().size == 1) {
// 		if (myAsset->parmMap()[mi->getUserTag()].info().type == HAPI_PARMTYPE_INT) {
// 			myAsset->parmMap()[mi->getUserTag()].setIntValue(0, mi->getSlider()->getValue());
//
// 			((MenuItem*)mi->getUserData())->setText(myAsset->parmMap()[mi->getUserTag()].label() + ": " +
// 			ostr("%1%", %mi->getSlider()->getValue()));
//
// 			update = true;
//
// 		} else if (myAsset->parmMap()[mi->getUserTag()].info().type == HAPI_PARMTYPE_FLOAT) {
// 			myAsset->parmMap()[mi->getUserTag()].setFloatValue(0, 0.01 * mi->getSlider()->getValue());
//
// 			((MenuItem*)mi->getUserData())->setText(myAsset->parmMap()[mi->getUserTag()].label() + ": " +
// 			ostr("%1%", %(mi->getSlider()->getValue() * 0.01)));
//
// 			update = true;
//
// 		}
// 	}

	hapi::Parm* parm = &(myAsset.get()->parmMap()[mi->getUserTag().substr(0, z)]);
	int index = atoi(mi->getUserTag().substr(z + 1).c_str());

// 	for (int i = 0; i < parm->info().size; ++i) {
	if (parm->info().type == HAPI_PARMTYPE_INT) {
		parm->setIntValue(index, mi->getSlider()->getValue());

		((MenuItem*)mi->getUserData())->setText(parm->label() + ": " +
		ostr("%1%", %mi->getSlider()->getValue()));

		update = true;

	} else if (parm->info().type == HAPI_PARMTYPE_FLOAT) {
		parm->setFloatValue(index, 0.01 * mi->getSlider()->getValue());

		((MenuItem*)mi->getUserData())->setText(parm->label() + ": " +
		ostr("%1%", %(mi->getSlider()->getValue() * 0.01)));

		update = true;

	}
// 	}

	if (update) {
		myAsset.get()->cook();
		wait_for_cook();
// 		process_assets(*myAsset);
		process_assets(myAsset);
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::onSelectedChanged(SceneNode* source, bool value)
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

void HoudiniEngine::wait_for_cook()
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

static std::string houdiniEngine::get_string(int string_handle)
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
void HoudiniEngine::handleEvent(const Event& evt)
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
// only run on master
// for each part of each geo of each object of each asset:
// send all the verts, faces, normals, colours, etc
void HoudiniEngine::commitSharedData(SharedOStream& out)
{
/*
	// share the heAsset
//     foreach(Mapping::Item asset, instancedHEAssets)
//     {
// 		// asset name
// 		out << asset->name();
// 	    vector<hapi::Object> objects = asset->objects();
//
// 		// obj count
// 		out << int(objects.size());
// 	    for (int object_index=0; object_index < int(objects.size()); ++object_index)
// 	    {
// 			vector<hapi::Geo> geos = objects[object_index].geos();
//
// 			// geo count
// 			out << int(geo.size());
// 			for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
// 			{
// 			    vector<hapi::Part> parts = geos[geo_index].parts();
//
// 				// parts count
// 				out << int(parts.size());
//
// 				hapi::HAPI_AttributeOwner[] owners = {
// 					HAPI_ATTROWNER_VERTEX,
// 					HAPI_ATTROWNER_POINT,
// 					HAPI_ATTROWNER_PRIM,
// 					HAPI_ATTROWNER_DETAIL
// 				};
//
// 			    for (int part_index=0; part_index < int(parts.size()); ++part_index)
// 				{
// 					hapi::HAPI_PartInfo hpi = part.info();
//
// 					for (int i = 0; i < 4; ++i) {
//
// 					    vector<std::string> attrib_names = part.attribNames(
// 						owners[i]);
//
// 						// num of attributes of that owner
// 						out << int(attrib_names.size());
//
// 					    for (int attrib_index=0; attrib_index < int(attrib_names.size());
// 						    ++attrib_index) {
// 							out << attrib_names[attrib_index];
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
*/
	out << updateGeos;

	if (!updateGeos) {
		return;
	}

	updateGeos = false;

	if (SystemManager::instance()->isMaster()) {
		ofmsg("MASTER: sending %1% geos", %myHoudiniGeometrys.size());
	}

	// change this to count the number of changed objs, geos, parts
	out << int(myHoudiniGeometrys.size());

	// no, share the houdiniGeometry stuff, as i've already added them to hg on master
    foreach(HGDictionary::Item hg, myHoudiniGeometrys)
    {
		out << hg->getName();
// 		ofmsg("MASTER: info for %1%", %hg->getName());
// 		getHGInfo(hg->getName());
// 		ofmsg("MASTER: end info for %1%", %hg->getName());
		out << hg->getVertexCount();
		for (int i = 0; i < hg->getVertexCount(); ++i) {
			out << hg->getVertex(i);
		}
		out << hg->getNormalCount();
		for (int i = 0; i < hg->getNormalCount(); ++i) {
			out << hg->getNormal(i);
		}
		out << hg->getColorCount();
		for (int i = 0; i < hg->getColorCount(); ++i) {
			out << hg->getColor(i);
		}
		// faces are done in that primitive set way
		// TODO: simplification: assume all faces are triangles?
		osg::Geometry* geo = hg->getOsgNode()->getDrawable(0)->asGeometry();
		osg::Geometry::PrimitiveSetList psl = geo->getPrimitiveSetList();

		out << int(psl.size());
		for (int i = 0; i < psl.size(); ++i) {

			osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(psl[i].get());

			out << da->getMode();
			out << int(da->getFirst());
			out << int(da->getCount());
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

	int numItems = 0;

	// houdiniGeometry count
	in >> numItems;

	if (!SystemManager::instance()->isMaster()) {
		ofmsg("SLAVE: getting data: %1%", %numItems);
	}

    for(int i = 0; i < numItems; i++)
    {
        String name;
        in >> name;

        HoudiniGeometry* hg = myHoudiniGeometrys[name];
        if(hg == NULL)
        {
// 			hg = HoudiniGeometry::create(name);
// 			myHoudiniGeometrys[name] = hg;
// 			SceneManager::instance()->addModel(hg);
			continue;
// 		} else {
// 			ofmsg("SLAVE: i have the hg: '%1%'", %name);
// 			getHGInfo(name);
// 			ofmsg("SLAVE: end info for '%1%'", %name);
		}

		hg->clear();

		int vertCount = 0;
		in >> vertCount;

		for (int j = 0; j < vertCount; ++j)
		{
			Vector3f v;
			in >> v;
			hg->addVertex(v);
		}

		int normalCount = 0;
		in >> normalCount;

		for (int j = 0; j < normalCount; ++j)
		{
			Vector3f n;
			in >> n;
			hg->addNormal(n);
		}

		int colorCount = 0;
		in >> colorCount;

		for (int j = 0; j < colorCount; ++j)
		{
			Color c;
			in >> c;
			hg->addColor(c);
		}

		// primitive set count
		int psCount = 0;
		in >> psCount;

		for (int j = 0; j < psCount; ++j)
		{
			osg::PrimitiveSet::Mode mode;
			int startIndex;
			int count;

			in >> mode;
			in >> startIndex;
			in >> count;

			hg->addPrimitiveOsg(mode, startIndex, count);
		}

		hg->dirty();

    }

	omsg("SLAVE: all hgs:");
    foreach(HGDictionary::Item hg, myHoudiniGeometrys)
    {
		ofmsg("SLAVE: hg: '%1%'", %hg->getName());
		getHGInfo(hg->getName());
	}
}
