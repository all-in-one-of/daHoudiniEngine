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
#include <omega.h>
#include <omegaGl.h>
#include "HAPI_CPP.h"

#include <stdlib.h>
#include <iostream>
#include <string>
using namespace std;

using namespace omega;

#define ENSURE_SUCCESS(result) \
    if ((result) != HAPI_RESULT_SUCCESS) \
    { \
	cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
	cout << hapi::Failure::lastErrorMessage() << endl;\
	exit(1); \
    }

#define ENSURE_COOK_SUCCESS(result) \
    if ((result) != HAPI_STATE_READY) \
    { \
	cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
	cout << hapi::Failure::lastCookErrorMessage() << endl;\
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

	Vector3s myNormals[6];
	Vector4i myFaces[6];
	Vector3s myVertices[8];
	Color myFaceColors[6];

	void load_asset_and_print_info(const char *otl_file);
	void print_asset_info( int asset_id, const HAPI_AssetInfo &asset_info );
	void process_geo_part(
	    int asset_id, int object_id, int geo_id, int part_id );

	void wait_for_cook();

	void process_float_attrib(
	    int asset_id, int object_id, int geo_id, int part_id,
	    HAPI_AttributeOwner attrib_owner,
	    const char *attrib_name );


	float getYaw() { return myYaw; }
	float getPitch() { return myPitch; }
	float getScale() { return myScale; }

	virtual void handleEvent(const Event& evt);
	virtual void commitSharedData(SharedOStream& out);
	virtual void updateSharedData(SharedIStream& in);

    int library_id;
	int asset_id;

private:

	void cook();

	float myYaw;
	float myPitch;
	float myScale;
	float myTx, myTy, myTz;
};

HEngineApp::HEngineApp():
	EngineModule("HEngineApp"),
	myScale(1.0)
{
	enableSharedData();

    try
    {

		const char *otl_file = "test_asset.otl";

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
	    print_asset_info( asset_id, asset_info );

    }
    catch (hapi::Failure &failure)
    {
	cout << failure.lastErrorMessage() << endl;
	throw;
    }
}

HEngineApp::~HEngineApp() {
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
	myApplication->myNormals[0] = Vector3s(-1, 0, 0);
	myApplication->myNormals[1] = Vector3s(0, 1, 0);
	myApplication->myNormals[2] = Vector3s(1, 0, 0);
	myApplication->myNormals[3] = Vector3s(0, -1, 0);
	myApplication->myNormals[4] = Vector3s(0, 0, 1);
	myApplication->myNormals[5] = Vector3s(0, 0, -1);

	// Initialize cube face indices.
	myApplication->myFaces[0] = Vector4i(0, 1, 2, 3);
	myApplication->myFaces[1] = Vector4i(3, 2, 6, 7);
	myApplication->myFaces[2] = Vector4i(7, 6, 5, 4);
	myApplication->myFaces[3] = Vector4i(4, 5, 1, 0);
	myApplication->myFaces[4] = Vector4i(5, 6, 2, 1);
	myApplication->myFaces[5] = Vector4i(7, 4, 0, 3);

	// Initialize cube face colors.
	myApplication->myFaceColors[0] = Color::Aqua;
	myApplication->myFaceColors[1] = Color::Orange;
	myApplication->myFaceColors[2] = Color::Olive;
	myApplication->myFaceColors[3] = Color::Navy;
	myApplication->myFaceColors[4] = Color::Red;
	myApplication->myFaceColors[5] = Color::Yellow;

	float size = 0.2f;

	// Setup cube vertex data
// 	myVertices[0][0] = myVertices[1][0] = myVertices[2][0] = myVertices[3][0] = -size;
// 	myVertices[4][0] = myVertices[5][0] = myVertices[6][0] = myVertices[7][0] = size;
// 	myVertices[0][1] = myVertices[1][1] = myVertices[4][1] = myVertices[5][1] = -size;
// 	myVertices[2][1] = myVertices[3][1] = myVertices[6][1] = myVertices[7][1] = size;
// 	myVertices[0][2] = myVertices[3][2] = myVertices[4][2] = myVertices[7][2] = size;
// 	myVertices[1][2] = myVertices[2][2] = myVertices[5][2] = myVertices[6][2] = -size;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineRenderPass::render(Renderer* client, const DrawContext& context)
{
	if(context.task == DrawContext::SceneDrawTask)
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

		// Draw a box
		for (int i = 0; i < 6; i++)
		{
			glBegin(GL_QUADS);
			glColor3fv(myApplication->myFaceColors[i].data());
			glNormal3fv(myApplication->myNormals[i].data());
			glVertex3fv(myApplication->myVertices[myApplication->myFaces[i][0]].data());
			glVertex3fv(myApplication->myVertices[myApplication->myFaces[i][1]].data());
			glVertex3fv(myApplication->myVertices[myApplication->myFaces[i][2]].data());
			glVertex3fv(myApplication->myVertices[myApplication->myFaces[i][3]].data());

// 			// remove this and just read the values of myVertices and myFaces
// 			float f = myApplication->getScale();
// 			Vector3s sv = f * myVertices[myFaces[i][0]];
// 			glVertex3fv(sv.data());
// 			sv = f * myVertices[myFaces[i][1]];
// 			glVertex3fv(sv.data());
// 			sv = f * myVertices[myFaces[i][2]];
// 			glVertex3fv(sv.data());
// 			sv = f * myVertices[myFaces[i][3]];
// 			glVertex3fv(sv.data());
			glEnd();
		}

		client->getRenderer()->endDraw();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineApp::cook()
{
    hapi::Asset asset(asset_id);

// 	std::map<std::string, hapi::Parm>::iterator it;

// 	for (it = asset.parmMap().begin(); it != asset.parmMap().end(); it++) {
// 	    cout << "key: " << it->first << endl;
// 	}

    // Change a parameter and recompute the geometry.
    hapi::Parm size_parm = asset.parmMap()["box1_size"];
    hapi::Parm center_parm = asset.parmMap()["box1_t"];
    cout << "old parm values: scale " <<
	    size_parm.getFloatValue(0) << ", " <<
	    size_parm.getFloatValue(1) << ", " <<
	    size_parm.getFloatValue(2) <<
	    " center: " <<
	    center_parm.getFloatValue(0) << ", " <<
	    center_parm.getFloatValue(1) << ", " <<
	    center_parm.getFloatValue(2) <<
	    endl;

    size_parm.setFloatValue(0, myScale);
    size_parm.setFloatValue(1, myScale);
    size_parm.setFloatValue(2, myScale);

    center_parm.setFloatValue(0, myTx);
    center_parm.setFloatValue(1, myTy);
    center_parm.setFloatValue(2, myTz);

	asset.cook();

	// making progress.. after cooking, what i need to do is update the arrays for each object
	// as well, and get rid of that custom size mult further up
    cout << "new parm values: " <<
	    size_parm.getFloatValue(0) << ", " <<
	    size_parm.getFloatValue(1) << ", " <<
	    size_parm.getFloatValue(2) <<
	    " center: " <<
	    center_parm.getFloatValue(0) << ", " <<
	    center_parm.getFloatValue(1) << ", " <<
	    center_parm.getFloatValue(2) <<
	    endl;


//     process_geo_part(asset_id, object_info.id, geo_info.id, part_index ); // hard coding for now
    process_geo_part(asset_id, 0, 0, 0 ); // hard coding for now

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

	if (update) {
		cook();
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineApp::commitSharedData(SharedOStream& out)
{
	out << myYaw << myPitch << myScale << myTx << myTy << myTz;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HEngineApp::updateSharedData(SharedIStream& in)
{
 	in >> myYaw >> myPitch >> myScale >> myTx >> myTy >> myTz;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplicationBase entry point
int main(int argc, char** argv)
{
	Application<HEngineApp> app("ohello2");
	return omain(app, argc, argv);
}

void HEngineApp::wait_for_cook()
{
    int status;
    int update_counter = 0;
    do
    {
        // Alternatively just use a sleep here - but didn't want to
        // platform dependent #ifdefs for this sample.
        if ( !( update_counter % 20 ) )
        {
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
                cout << "cooking...:" << result << "\n";
                delete[] statusBuf;
            }
        }
        update_counter++;
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

void HEngineApp::print_asset_info( int asset_id, const HAPI_AssetInfo &asset_info )
{
    HAPI_ObjectInfo * object_infos =
        new HAPI_ObjectInfo[ asset_info.objectCount ];
    ENSURE_SUCCESS( HAPI_GetObjects(
        asset_id, object_infos, /*start=*/ 0, asset_info.objectCount ) );
    for ( int object_index = 0; object_index < asset_info.objectCount;
        ++object_index )
    {
        HAPI_ObjectInfo &object_info = object_infos[object_index];
        for ( int geo_index = 0; geo_index < object_info.geoCount;
            ++geo_index )
        {
            HAPI_GeoInfo geo_info;
            ENSURE_SUCCESS( HAPI_GetGeoInfo(
                asset_id, object_info.id, geo_index, &geo_info ) );
            for ( int part_index = 0; part_index < geo_info.partCount;
                ++part_index )
            {
                process_geo_part(
                    asset_id, object_info.id, geo_info.id, part_index );
            }
        }
    }
    delete [] object_infos;
}

void HEngineApp::process_geo_part(
    int asset_id, int object_id, int geo_id, int part_id )
{
    cout << "object " << object_id << ", geo " << geo_id
        << ", part " << part_id << endl;
    HAPI_PartInfo part_info;
    ENSURE_SUCCESS( HAPI_GetPartInfo(
        asset_id, object_id, geo_id, part_id, &part_info ) );
    // Get the list of attribute names.
    int *attrib_names_sh = new int[ part_info.pointAttributeCount ];
    ENSURE_SUCCESS( HAPI_GetAttributeNames(
        asset_id, object_id, geo_id, part_id,
        HAPI_ATTROWNER_POINT,
        attrib_names_sh,
        part_info.pointAttributeCount ) );
    cout << "attributes:" << endl;
    for ( int attrib_index = 0; attrib_index < part_info.pointAttributeCount;
        ++attrib_index)
    {
        HAPI_StringHandle string_handle = attrib_names_sh[ attrib_index ];
        std::string attrib_name = get_string( string_handle );
        cout << "    " << attrib_name << endl;
    }
    delete [] attrib_names_sh;
    cout << "point positions:" << endl;
    process_float_attrib(
        asset_id, object_id, geo_id, part_id, HAPI_ATTROWNER_POINT, "P" );
    cout << "Number of faces: " << part_info.faceCount << endl;
    int * face_counts = new int[ part_info.faceCount ];
    ENSURE_SUCCESS( HAPI_GetFaceCounts(asset_id,object_id,geo_id,
        part_id,face_counts,0,part_info.faceCount ) );
    cout << "Facecounts:";
    for( int ii=0; ii < part_info.faceCount; ii++ )
    {
        cout << face_counts[ii] << ",";
    }
    cout << "\n";
    int * vertex_list = new int[ part_info.vertexCount ];
    ENSURE_SUCCESS( HAPI_GetVertexList( asset_id, object_id, geo_id,
        part_id, vertex_list, 0, part_info.vertexCount ) );
    cout << "Vertex Indices into Points array:\n";
    int curr_index = 0;
    for( int ii=0; ii < part_info.faceCount; ii++ )
    {
        for( int jj=0; jj < face_counts[ii]; jj++ )
        {
			myFaces[ii][jj] = vertex_list[ curr_index ];
            cout << "vertex :" << curr_index << ", belonging to face: " << ii <<", index: "
                << vertex_list[ curr_index ] << " of points array\n";
            curr_index++;
        }
    }
    delete[] face_counts;
    delete[] vertex_list;
}

void HEngineApp::process_float_attrib(
    int asset_id, int object_id, int geo_id, int part_id,
    HAPI_AttributeOwner attrib_owner,
    const char *attrib_name )
{
    // Get the attribute values.
    HAPI_AttributeInfo attrib_info;
    ENSURE_SUCCESS( HAPI_GetAttributeInfo(
        asset_id, object_id, geo_id, part_id, attrib_name,
        attrib_owner, &attrib_info ) );
    float *attrib_data = new float[
        attrib_info.count * attrib_info.tupleSize ];
        ENSURE_SUCCESS( HAPI_GetAttributeFloatData(
            asset_id, object_id, geo_id, part_id,
            attrib_name, &attrib_info, attrib_data,
            /*start=*/0, attrib_info.count ) );
    for ( int elem_index = 0; elem_index < attrib_info.count; ++elem_index )
    {
        for ( int tuple_index = 0; tuple_index < attrib_info.tupleSize;
            ++tuple_index)
        {
			myVertices[elem_index][tuple_index] = attrib_data[
                elem_index * attrib_info.tupleSize + tuple_index ];
            cout << attrib_data[
                elem_index * attrib_info.tupleSize + tuple_index ]
            << " ";
        }
        cout << endl;
    }
    delete [] attrib_data;
}
