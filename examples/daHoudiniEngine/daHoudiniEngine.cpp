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
 *********************************************************************************************************************/
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

This is an example executable using the Houdini Engine API. Based on the
cyHello demo from Omegalib.

******************************************************************************/


#include <omega.h>
#include <omegaGl.h>
#include "HAPI_CPP.h"

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

using namespace omega;

#define ENSURE_SUCCESS(result) \
    if ((result) != HAPI_RESULT_SUCCESS) \
    { \
	cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
	cout << hapi::Failure::lastErrorMessage(NULL) << endl;\
	exit(1); \
    }

#define ENSURE_COOK_SUCCESS(result) \
    if ((result) != HAPI_STATE_READY) \
    { \
	cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
	cout << hapi::Failure::lastCookErrorMessage(NULL) << endl;\
	exit(1); \
    }

class HEngineApp;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HEngineRenderPass: public RenderPass
{
public:
	HEngineRenderPass(Renderer* client, HEngineApp* app): RenderPass(client, "HEngineRenderPass"), myApplication(app) {}
	virtual void initialize();
	virtual void render(Renderer* client, const DrawContext& context);

// 	Vector3s myNormals[6];
// 	Vector4i myFaces[6];
// 	Vector3s myVertices[8];
// 	Color myFaceColors[6];

private:
	HEngineApp* myApplication;

};

static std::string get_string(int string_handle);

String sOtl_file;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HEngineApp: public EngineModule
{
public:
	static HEngineApp* createAndInitialize();

	HEngineApp();

	~HEngineApp();

	virtual void initializeRenderer(Renderer* r)
	{
		r->addRenderPass(new HEngineRenderPass(r, this));
	}

	vector<Vector3s> myNormals;
	vector<vector <int> > myFaces;
	vector<Vector3s> myVertices;
	vector<Color> myFaceColors;

	void load_asset_and_print_info(const char *otl_file);
	void print_asset_info(const hapi::Asset &asset);
	void process_geo_part(const hapi::Part &part);

	void wait_for_cook();

	void process_float_attrib(
	    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
	    const char *attrib_name);


	float getYaw() { return myYaw; }
	float getPitch() { return myPitch; }
	float getScale() { return myScale; }

	virtual void handleEvent(const Event& evt);
	virtual void commitSharedData(SharedOStream& out);
	virtual void updateSharedData(SharedIStream& in);

	bool ready;
	bool cooked;

	void cook();

    int library_id;
    hapi::Asset* myAsset;
	int asset_id;
	int faceCount;

private:

	float myYaw;
	float myPitch;
	float myScale;
	float myTx, myTy, myTz;
	int mySwitch;
};

HEngineApp::HEngineApp():
	EngineModule("HEngineApp"),
	myScale(1.0),
	mySwitch(0),
	myTx(0.0),
	myTy(0.0),
	myTz(0.0)

{
	enableSharedData();

	if (!SystemManager::instance()->isMaster()) {
		return;
	}

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
			/* session */ NULL,
			&cook_options,
			/*use_cooking_thread=*/true,
			/*cooking_thread_max_size=*/-1,
			/*otl search path*/ getenv("$HOME"),
			/*dso_search_path=*/ NULL,
			/*image_dso_search_path=*/ NULL,
			/*audio_dso_search_path=*/ NULL
		));

		omsg("HAPI Loaded");

	    if (HAPI_LoadAssetLibraryFromFile(
				NULL,
	            otl_file,
				false, /* allow_overwrite */
	            &library_id) != HAPI_RESULT_SUCCESS)
	    {
	        cout << "Could not load " << otl_file << endl;
	        exit(1);
	    }

		ofmsg("HAPI load asset from file %1%",  %otl_file);

	    HAPI_StringHandle asset_name_sh;
	    ENSURE_SUCCESS( HAPI_GetAvailableAssets( NULL, library_id, &asset_name_sh, 1 ) );
	    std::string asset_name = get_string( asset_name_sh );

	    ENSURE_SUCCESS(HAPI_InstantiateAsset(
				NULL,
	            asset_name.c_str(),
	            /* cook_on_load */ true,
	            &asset_id ));

		wait_for_cook();

	    HAPI_AssetInfo asset_info;

	    ENSURE_SUCCESS( HAPI_GetAssetInfo( NULL, asset_id, &asset_info ) );

		myAsset = new hapi::Asset(asset_id, NULL); // make this a ref pointer later

	    print_asset_info(*myAsset);

    }
    catch (hapi::Failure &failure)
    {
	cout << failure.lastErrorMessage(NULL) << endl;
	throw;
    }
}

HEngineApp::~HEngineApp() {

	if (SystemManager::instance()->isMaster()) {
		if (myAsset != NULL) delete myAsset;
	}

    try
    {
		if (SystemManager::instance()->isMaster()) {
		    ENSURE_SUCCESS(HAPI_Cleanup(NULL));
			cout << "done HAPI" << endl;
		}
    }
    catch (hapi::Failure &failure)
    {
		cout << failure.lastErrorMessage(NULL) << endl;
		throw;
    }
}

HEngineApp* HEngineApp::createAndInitialize()
{
	// Initialize and register the HEngineApp module.
	HEngineApp* instance = new HEngineApp();
	ModuleServices::addModule(instance);
	instance->doInitialize(Engine::instance());
	return instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineRenderPass::initialize()
{
	RenderPass::initialize();

	// Initialize cube normals.
	myApplication->myNormals.resize(6);
	myApplication->myNormals[0] = Vector3s(-1, 0, 0);
	myApplication->myNormals[1] = Vector3s(0, 1, 0);
	myApplication->myNormals[2] = Vector3s(1, 0, 0);
	myApplication->myNormals[3] = Vector3s(0, -1, 0);
	myApplication->myNormals[4] = Vector3s(0, 0, 1);
	myApplication->myNormals[5] = Vector3s(0, 0, -1);

	// Initialize cube face indices.
	myApplication->myFaces.resize(0);

	// Initialize cube face colors.
	myApplication->myFaceColors.resize(6);
	myApplication->myFaceColors[0] = Color::Aqua;
	myApplication->myFaceColors[1] = Color::Orange;
	myApplication->myFaceColors[2] = Color::Olive;
	myApplication->myFaceColors[3] = Color::Navy;
	myApplication->myFaceColors[4] = Color::Red;
	myApplication->myFaceColors[5] = Color::Yellow;


	if (SystemManager::instance()->isMaster()) {
		myApplication->cook();

	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineRenderPass::render(Renderer* client, const DrawContext& context)
{

	if(context.task == DrawContext::SceneDrawTask )
	{
		client->getRenderer()->beginDraw3D(context);

		// Enable depth testing and lighting.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);

		// Setup light.
		glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, Color(1.0, 1.0, 1.0).data());
		glLightfv(GL_LIGHT0, GL_POSITION, Vector3s(0.0f, 0.0f, 1.0f).data());

		// Draw a rotating teapot.
		glTranslatef(0, 2, -2);
		glRotatef(10, 1, 0, 0);
		glRotatef(myApplication->getYaw(), 0, 1, 0);
		glRotatef(myApplication->getPitch(), 1, 0, 0);

		float f = myApplication->getScale();

		float z = 0.5 / (myApplication->myFaces.size() == 0 ? 1 : myApplication->myFaces.size());

		// The object
		for (int i = 0; i < myApplication->myFaces.size(); i++)
		{
			if (myApplication->faceCount == 4) {
				glBegin(GL_QUADS);
			} else if (myApplication->faceCount == 3) {
				glBegin(GL_TRIANGLES);
			} else {
				glBegin(GL_TRIANGLE_FAN);
			}

			glColor3fv(Color(0.5 + (i * z), 0.5 + (i * z), 0.5 + (i * z)).data());

			for (int j = 0; j < myApplication->faceCount; ++j) {
				glVertex3fv(myApplication->myVertices[myApplication->myFaces[i][j]].data());
			}

			glEnd();
		}

		client->getRenderer()->endDraw();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineApp::cook()
{

    // Change a parameter and recompute the geometry.
    hapi::Parm size_parm = myAsset->parmMap()["xform1_scale"];
    hapi::Parm center_parm = myAsset->parmMap()["box1_t"];
    hapi::Parm switch_parm = myAsset->parmMap()["switch1_input"];
    cout << "old parm values: scale " <<
	    size_parm.getFloatValue(0) << ", " <<
	    " center: " <<
	    center_parm.getFloatValue(0) << ", " <<
	    center_parm.getFloatValue(1) << ", " <<
	    center_parm.getFloatValue(2) <<
	    " switch: " <<
	    switch_parm.getIntValue(0) <<
	    endl;

    size_parm.setFloatValue(0, myScale);

    center_parm.setFloatValue(0, myTx);
    center_parm.setFloatValue(1, myTy);
    center_parm.setFloatValue(2, myTz);

	switch_parm.setIntValue(0, mySwitch);

	myAsset->cook();

	wait_for_cook();
	cooked = true;

    cout << "new parm values: " <<
	    size_parm.getFloatValue(0) << ", " <<
	    " center: " <<
	    center_parm.getFloatValue(0) << ", " <<
	    center_parm.getFloatValue(1) << ", " <<
	    center_parm.getFloatValue(2) <<
	    " switch: " <<
	    switch_parm.getIntValue(0) <<
	    endl;


		print_asset_info(*myAsset);

	cout << "my face count:" << faceCount << endl;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineApp::handleEvent(const Event& evt)
{
	if(evt.getServiceType() == Service::Pointer)
	{
		// Normalize the mouse position using the total display resolution,
		// then multiply to get 180 degree rotations
		DisplaySystem* ds = getEngine()->getDisplaySystem();
		Vector2i resolution = ds->getDisplayConfig().getCanvasRect().size();
		myYaw = (evt.getPosition().x() / resolution[0]) * 180;
		myPitch = (evt.getPosition().y() / resolution[1]) * 180;
	}

	bool update = false;


	// switch between different objects
	if (evt.isKeyDown('0')) {
		mySwitch = (mySwitch + 1) % 4; // number of switches.. how get a var
// 		cout << "switch count: " << myAsset->parmMap()["switchCount"].getIntValue(0) << endl;
		update = true;
	}

	// scale
	if (evt.isKeyDown('1')) {
		myScale -= 0.1;
		if (myScale < 0.1) myScale = 0.1;

		update = true;
	}

	if (evt.isKeyDown('2')) {
		myScale += 0.1;
		if (myScale > 2.0) myScale = 2.0;

		update = true;
	}

	// translateX
	if (evt.isKeyDown('3')) {
		myTx -= 0.1;
		if (myTx < -0.5) myTx = -0.5;
		update = true;
	}
	if (evt.isKeyDown('4')) {
		myTx += 0.1;
		if (myTx > 0.5) myTx = 0.5;
		update = true;
	}

	// translateY
	if (evt.isKeyDown('5')) {
		myTy -= 0.1;
		if (myTy < -0.5) myTy = -0.5;
		update = true;
	}
	if (evt.isKeyDown('6')) {
		myTy += 0.1;
		if (myTy > 0.5) myTy = 0.5;
		update = true;
	}

	// translateZ
	if (evt.isKeyDown('7')) {
		myTz -= 0.1;
		if (myTz < -0.5) myTz = -0.5;
		update = true;
	}
	if (evt.isKeyDown('8')) {
		myTz += 0.1;
		if (myTz > 0.5) myTz = 0.5;
		update = true;
	}

	// cook only if a change
	if (update && SystemManager::instance()->isMaster()) {
		cook();
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineApp::commitSharedData(SharedOStream& out)
{

	out << cooked;

	int vertSize = myVertices.size(); // NOTE: myVertices.size() != int
	int faceSize = myFaces.size();

	out <<
		myYaw << myPitch << myScale <<
		myTx << myTy << myTz <<
		mySwitch;

	if (!cooked) {
		return;
	}

	out << vertSize;
		if (myVertices.size() > 0) {
			for (int i = 0; i < myVertices.size(); ++i) {
  				out << myVertices[i][0] << myVertices[i][1] << myVertices[i][2];
			}
		}
		out << faceSize;
		for (int i = 0; i < myFaces.size(); ++i) {
			out << (int) myFaces[i].size();
			out << myFaces[i][0] << myFaces[i][1] << myFaces[i][2];
		}

	cooked = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineApp::updateSharedData(SharedIStream& in)
{
	int vertSize = 0;
	int facesSize = 0;
	int blah = 0;

 	in >> cooked >> myYaw >> myPitch >> myScale >>
		myTx >> myTy >> myTz >>
		mySwitch;

	if (!cooked) {
		return;
	}

	in >> vertSize;
	if (vertSize > 0) {
		myVertices.resize(vertSize);
		for (int i = 0; i < myVertices.size(); ++i) {
			in >> myVertices[i][0] >> myVertices[i][1] >> myVertices[i][2];
		}
	}

	in >> facesSize;
	if (facesSize > 0) {
		myFaces.resize(facesSize);
		for (int i = 0; i < facesSize; ++i) {
			in >> faceCount;
			myFaces[i].resize(faceCount);
			in >> myFaces[i][0] >> myFaces[i][1] >> myFaces[i][2];
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplicationBase entry point
int main(int argc, char** argv)
{
	Application<HEngineApp> app("daHoudiniEngine");
    oargs().newNamedString('o', "otl", "otl", "The otl to load", sOtl_file);
	return omain(app, argc, argv);
}

void HEngineApp::wait_for_cook()
{
    int status;
//     int update_counter = 0;
    do
    {
//         // Alternatively just use a sleep here - but didn't want to
//         // platform dependent #ifdefs for this sample.
//         if ( !( update_counter % 20 ) )
//         {
//             int statusBufSize = 0;
//             ENSURE_SUCCESS( HAPI_GetStatusStringBufLength(
//                 HAPI_STATUS_COOK_STATE, HAPI_STATUSVERBOSITY_ERRORS,
//                 &statusBufSize ) );
//             char * statusBuf = NULL;
//             if ( statusBufSize > 0 )
//             {
//                 statusBuf = new char[statusBufSize];
//                 ENSURE_SUCCESS( HAPI_GetStatusString(
//                     HAPI_STATUS_COOK_STATE, statusBuf ) );
//             }
//             if ( statusBuf )
//             {
//                 std::string result( statusBuf );
//                 cout << "cooking...:" << result << "\n";
//                 delete[] statusBuf;
//             }
//         }
//         update_counter++;
		osleep(100); // sleeping..
        HAPI_GetStatus(NULL, HAPI_STATUS_COOK_STATE, &status);
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
    ENSURE_SUCCESS(HAPI_GetStringBufLength(NULL, string_handle, &buffer_length));

    char * buf = new char[ buffer_length ];

    ENSURE_SUCCESS(HAPI_GetString(NULL, string_handle, buf, buffer_length));

    std::string result( buf );
    delete[] buf;
    return result;
}

void HEngineApp::print_asset_info(const hapi::Asset &asset)
{
    vector<hapi::Object> objects = asset.objects();
    for (int object_index=0; object_index < int(objects.size()); ++object_index)
    {
		vector<hapi::Geo> geos = objects[object_index].geos();

		for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
		{
		    vector<hapi::Part> parts = geos[geo_index].parts();

		    for (int part_index=0; part_index < int(parts.size()); ++part_index)
				process_geo_part(parts[part_index]);
		}
    }
}


void HEngineApp::process_geo_part(const hapi::Part &part)
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
    process_float_attrib(part, HAPI_ATTROWNER_POINT, "P");

    cout << "Number of faces: " << part.info().faceCount << endl;
    int * face_counts = new int[ part.info().faceCount ];
    ENSURE_SUCCESS( HAPI_GetFaceCounts(
		NULL,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		face_counts,
		0,
		part.info().faceCount
	) );
    cout << "Facecounts:";
    for( int ii=0; ii < part.info().faceCount; ii++ )
    {
        cout << face_counts[ii] << ",";
    }
    cout << "\n";

	// begin materials
	int mat_id = -1;
	HAPI_MaterialInfo mat_info;
    ENSURE_SUCCESS( HAPI_GetMaterialIdsOnFaces(
		NULL,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		NULL /* are_all_the_same*/,
		&mat_id, 0, 1));
	ENSURE_SUCCESS( HAPI_GetMaterialInfo (
		NULL,
		part.geo.object.asset.id,
		mat_id,
		&mat_info));

	if (mat_info.exists) {
		ofmsg("Material info for %1%: matId: %2% assetId: %3% nodeId: %4%",
			  %mat_info.id %mat_info.id %mat_info.assetId %mat_info.nodeId
		);

		HAPI_NodeInfo node_info;
		ENSURE_SUCCESS( HAPI_GetNodeInfo(
			NULL,
			mat_info.nodeId, &node_info ));
		ofmsg("Node info for %1%: id: %2% assetId: %3% internal path: %4%",
			  %get_string(node_info.nameSH) %node_info.id %node_info.assetId
			  %get_string(node_info.internalNodePathSH)
		);

		HAPI_ParmInfo* parm_infos = new HAPI_ParmInfo[node_info.parmCount];
		ENSURE_SUCCESS( HAPI_GetParameters(
			NULL,
			mat_info.nodeId,
			parm_infos,
			0 /* start */,
			node_info.parmCount));

		for (int i =0; i < node_info.parmCount; ++i) {
			ofmsg("%3% %1% (%2%) id = %4%", %get_string(parm_infos[i].labelSH) %get_string(parm_infos[i].nameSH)
				%i %parm_infos[i].id);
			if (parm_infos[i].stringValuesIndex >= 0) {
				int sh = -1;
				ENSURE_SUCCESS( HAPI_GetParmStringValues(
					NULL,
					mat_info.nodeId, true,
					&sh,
					parm_infos[i].stringValuesIndex, 1)
				);
				ofmsg("  %1%: %2%", %parm_infos[i].stringValuesIndex
					%get_string(sh));
			}
		}

		ofmsg("%1% %2% %3%", %mat_info.assetId %mat_info.id %parm_infos[7].id);

//  		ENSURE_SUCCESS( HAPI_RenderTextureToImage(
//  			mat_info.assetId,
//  			mat_info.id,
// 			parm_infos[7].id /* parmIndex for "map" */));

		for (int i = 0; i < node_info.parmCount; ++i) {
			for (int j = 0; j < 10; ++j) {
				for (int k = 0; k < 10; ++k) {

					std::string s = get_string(parm_infos[i].nameSH);

					// TODO: check parm string values for each one of above.. looking for the
					// filename..

					ofmsg("name: %1%, %2%", %s %i);

					int result = HAPI_RenderTextureToImage(
						NULL,
						k,
						j /*mat_info.id */,
						i);

					if ( result != HAPI_RESULT_SUCCESS) {
						ofmsg("result = %4% (failed with %1% %2% %3%)", %k %j %i %result);
					} else {
						omsg("render success");
					}
				}
			}
		}

//  		ENSURE_SUCCESS( HAPI_RenderMaterialToImage(
//  			mat_info.assetId,
//  			mat_info.id,
// // 			HAPI_SHADER_MANTRA));
// 			HAPI_SHADER_OPENGL));

		HAPI_ImageInfo image_info;
		ENSURE_SUCCESS( HAPI_GetImageInfo(
			NULL,
			node_info.assetId,
			mat_info.id,
			&image_info));

		ofmsg("width %1% height: %2% format: %3%",
			  %image_info.xRes
			  %image_info.yRes
			  %get_string(image_info.imageFileFormatNameSH));

	} else {
		omsg("No material for this asset");
	}

	// end materials


    int * vertex_list = new int[ part.info().vertexCount ];
    ENSURE_SUCCESS( HAPI_GetVertexList(
		NULL,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		vertex_list, 0, part.info().vertexCount ) );
    cout << "Vertex Indices into Points array:\n";
    int curr_index = 0;

	myFaces.resize(part.info().faceCount);

    for( int ii=0; ii < part.info().faceCount; ii++ )
    {
		myFaces[ii].resize(face_counts[ii]);
        for( int jj=0; jj < face_counts[ii]; jj++ )
        {
			myFaces[ii][jj] = vertex_list[ curr_index ];
            cout << "vertex :" << curr_index << ", belonging to face: " << ii <<", index: "
                << vertex_list[ curr_index ] << " of points array\n";
            curr_index++;
        }
    }

    // should be a per-face vector
	faceCount = face_counts[0];

    delete[] face_counts;
    delete[] vertex_list;

}


void HEngineApp::process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name)
{
    // Get the attribute values.
    HAPI_AttributeInfo attrib_info = part.attribInfo(attrib_owner, attrib_name);
    float *attrib_data = part.getNewFloatAttribData(attrib_info, attrib_name);

	myVertices.resize(attrib_info.count);

    for (int elem_index=0; elem_index < attrib_info.count; ++elem_index)
    {
		for (int tuple_index=0; tuple_index < attrib_info.tupleSize;
			++tuple_index)
		{
			myVertices[elem_index][tuple_index] = attrib_data[
                elem_index * attrib_info.tupleSize + tuple_index ];
		    cout << attrib_data[
			    elem_index * attrib_info.tupleSize + tuple_index]
			<< " ";
		}
		cout << endl;
    }

    delete [] attrib_data;
}
