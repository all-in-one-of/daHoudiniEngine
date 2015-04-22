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

class HelloApplication;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HelloRenderPass: public RenderPass
{
public:
	HelloRenderPass(Renderer* client, HelloApplication* app): RenderPass(client, "HelloRenderPass"), myApplication(app) {}
	virtual void initialize();
	virtual void render(Renderer* client, const DrawContext& context);

private:
	HelloApplication* myApplication;

	Vector3s myNormals[6];
	Vector4i myFaces[6];
	Vector3s myVertices[8];
	Color myFaceColors[6];
};

static std::string get_string(int string_handle);
static void load_asset_and_print_info(const char *otl_file);
static void print_asset_info(const hapi::Asset &asset);
static void process_geo_part(const hapi::Part &part);

void process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HelloApplication: public EngineModule
{
public:
	HelloApplication(): EngineModule("HelloApplication") { enableSharedData(); }

	virtual void initializeRenderer(Renderer* r)
	{
		r->addRenderPass(new HelloRenderPass(r, this));
	}

	float getYaw() { return myYaw; }
	float getPitch() { return myPitch; }

	virtual void handleEvent(const Event& evt);
	virtual void commitSharedData(SharedOStream& out);
	virtual void updateSharedData(SharedIStream& in);

private:
	float myYaw;
	float myPitch;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloRenderPass::initialize()
{
	RenderPass::initialize();

	// Initialize cube normals.
	myNormals[0] = Vector3s(-1, 0, 0);
	myNormals[1] = Vector3s(0, 1, 0);
	myNormals[2] = Vector3s(1, 0, 0);
	myNormals[3] = Vector3s(0, -1, 0);
	myNormals[4] = Vector3s(0, 0, 1);
	myNormals[5] = Vector3s(0, 0, -1);

	// Initialize cube face indices.
	myFaces[0] = Vector4i(0, 1, 2, 3);
	myFaces[1] = Vector4i(3, 2, 6, 7);
	myFaces[2] = Vector4i(7, 6, 5, 4);
	myFaces[3] = Vector4i(4, 5, 1, 0);
	myFaces[4] = Vector4i(5, 6, 2, 1);
	myFaces[5] = Vector4i(7, 4, 0, 3);

	// Initialize cube face colors.
	myFaceColors[0] = Color::Aqua;
	myFaceColors[1] = Color::Orange;
	myFaceColors[2] = Color::Olive;
	myFaceColors[3] = Color::Navy;
	myFaceColors[4] = Color::Red;
	myFaceColors[5] = Color::Yellow;

	// Setup cube vertex data
	float size = 0.2f;
	myVertices[0][0] = myVertices[1][0] = myVertices[2][0] = myVertices[3][0] = -size;
	myVertices[4][0] = myVertices[5][0] = myVertices[6][0] = myVertices[7][0] = size;
	myVertices[0][1] = myVertices[1][1] = myVertices[4][1] = myVertices[5][1] = -size;
	myVertices[2][1] = myVertices[3][1] = myVertices[6][1] = myVertices[7][1] = size;
	myVertices[0][2] = myVertices[3][2] = myVertices[4][2] = myVertices[7][2] = size;
	myVertices[1][2] = myVertices[2][2] = myVertices[5][2] = myVertices[6][2] = -size;

    try
    {
	cout << "about to HAPI" << endl;
 	load_asset_and_print_info("test_asset.otl");
	cout << "done HAPI" << endl;
    }
    catch (hapi::Failure &failure)
    {
	cout << failure.lastErrorMessage() << endl;
	throw;
    }


}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloRenderPass::render(Renderer* client, const DrawContext& context)
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

		// Draw a box
		for (int i = 0; i < 6; i++)
		{
			glBegin(GL_QUADS);
			glColor3fv(myFaceColors[i].data());
			glNormal3fv(myNormals[i].data());
			glVertex3fv(myVertices[myFaces[i][0]].data());
			glVertex3fv(myVertices[myFaces[i][1]].data());
			glVertex3fv(myVertices[myFaces[i][2]].data());
			glVertex3fv(myVertices[myFaces[i][3]].data());
			glEnd();
		}

		client->getRenderer()->endDraw();
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::handleEvent(const Event& evt)
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
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::commitSharedData(SharedOStream& out)
{
	out << myYaw << myPitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::updateSharedData(SharedIStream& in)
{
 	in >> myYaw >> myPitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplicationBase entry point
int main(int argc, char** argv)
{
	Application<HelloApplication> app("ohello2");
	return omain(app, argc, argv);
}

void load_asset_and_print_info(const char *otl_file)
{
	cout << "about to init HAPI" << endl;
    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();

    ENSURE_SUCCESS(HAPI_Initialize(
	getenv("$HOME"),
	/*dso_search_path=*/NULL,
        &cook_options,
	/*use_cooking_thread=*/true,
	/*cooking_thread_max_size=*/-1));

	cout << "HAPI init" << endl; // segfault on hapi init after equalizer started.
	// segfault on eq starting when hapi run first..

    int library_id;
    int asset_id;
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

    int status;
    int update_counter = 0;
    do
    {
        //alternatively just use a sleep here - but didn't want to
        //platform dependent #ifdefs for this sample.
        if( !(update_counter % 20) )
        {
            int statusBufSize = 0;
            ENSURE_SUCCESS(HAPI_GetStatusStringBufLength(
                    HAPI_STATUS_COOK_STATE, HAPI_STATUSVERBOSITY_ERRORS,
                    &statusBufSize));

            char * statusBuf = NULL;
            if(statusBufSize > 0)
            {
                statusBuf = new char[statusBufSize];
                ENSURE_SUCCESS(HAPI_GetStatusString(HAPI_STATUS_COOK_STATE, statusBuf));
            }

            if(statusBuf)
            {
                std::string result( statusBuf );
                cout << "cooking...:" << result << "\n";
                delete[] statusBuf;
            }
        }
        update_counter++;
        HAPI_GetStatus(HAPI_STATUS_COOK_STATE, &status);
    } while( status > HAPI_STATE_MAX_READY_STATE );

    ENSURE_COOK_SUCCESS( status );

    // Print out all the parameters and their values.
    hapi::Asset asset(asset_id);
    cout << "Parms:" << endl;
    std::vector<hapi::Parm> parms = asset.parms();
    for (int i=0; i < int(parms.size()); ++i)
    {
	hapi::Parm &parm = parms[i];
	cout << "   name: " << parm.name() << endl;

	cout << " values: (";
	for (int sub_index=0; sub_index < parm.info().size; ++sub_index)
	{
	    if (sub_index != 0)
		cout << ", ";

	    if ( HAPI_ParmInfo_IsInt( &parm.info() ) )
		cout << parm.getIntValue(sub_index);
	    else if ( HAPI_ParmInfo_IsFloat( &parm.info() ) )
		cout << parm.getFloatValue(sub_index);
	    else if ( HAPI_ParmInfo_IsString( &parm.info() ) )
		cout << parm.getStringValue(sub_index);
	}
	cout << ")" << endl;
    }

    // Print information about the geometry contained inside the asset.
    print_asset_info(asset);

    // Change a parameter and recompute the geometry.
    hapi::Parm size_parm = asset.parmMap()["box1_size"];
    cout << "old parm value: " << size_parm.getFloatValue(0) << endl;
    size_parm.setFloatValue(0, 2.0);
    asset.cook();
    cout << "old parm value: " << size_parm.getFloatValue(0) << endl;

    // Print information about the new geometry contained inside the asset.
    print_asset_info(asset);

    ENSURE_SUCCESS(HAPI_Cleanup());
}

void print_asset_info(const hapi::Asset &asset)
{
    std::vector<hapi::Object> objects = asset.objects();
    for (int object_index=0; object_index < int(objects.size()); ++object_index)
    {
	std::vector<hapi::Geo> geos = objects[object_index].geos();

	for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
	{
	    std::vector<hapi::Part> parts = geos[geo_index].parts();

	    for (int part_index=0; part_index < int(parts.size()); ++part_index)
		process_geo_part(parts[part_index]);
	}
    }
}

void process_geo_part(const hapi::Part &part)
{
    cout << "object " << part.geo.object.id << ", geo " << part.geo.id
	 << ", part " << part.id << endl;

    // Print the list of point attribute names.
    cout << "attributes:" << endl;
    std::vector<std::string> attrib_names = part.attribNames(
	HAPI_ATTROWNER_POINT);
    for (int attrib_index=0; attrib_index < int(attrib_names.size());
	    ++attrib_index)
	cout << "    " << attrib_names[attrib_index] << endl;

    cout << "point positions:" << endl;
    process_float_attrib(part, HAPI_ATTROWNER_POINT, "P");
}

void process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name)
{
    // Get the attribute values.
    HAPI_AttributeInfo attrib_info = part.attribInfo(attrib_owner, attrib_name);
    float *attrib_data = part.getNewFloatAttribData(attrib_info, attrib_name);

    for (int elem_index=0; elem_index < attrib_info.count; ++elem_index)
    {
	for (int tuple_index=0; tuple_index < attrib_info.tupleSize;
		++tuple_index)
	{
	    cout << attrib_data[
		    elem_index * attrib_info.tupleSize + tuple_index]
		<< " ";
	}
	cout << endl;
    }

    delete [] attrib_data;
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
