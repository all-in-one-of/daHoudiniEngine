#include "HAPI_CPP.h"
#include <stdlib.h>
#include <iostream>
#include <string>
using namespace std;

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

static std::string get_string(int string_handle);
static void load_asset_and_print_info(const char *otl_file);
static void print_asset_info(const hapi::Asset &asset);
static void process_geo_part(const hapi::Part &part);
void process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name);

int main(int argc, char **argv)
{
    const char *otl_file = argc == 2 ? argv[1] : "test_asset.otl";
    try
    {
	load_asset_and_print_info(otl_file);
    }
    catch (hapi::Failure &failure)
    {
	cout << failure.lastErrorMessage() << endl;
	throw;
    }
}

static void load_asset_and_print_info(const char *otl_file)
{
    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();

    ENSURE_SUCCESS(HAPI_Initialize(
	getenv("$HOME"),
	/*dso_search_path=*/NULL,
        &cook_options,
	/*use_cooking_thread=*/true,
	/*cooking_thread_max_size=*/-1));

    int library_id;
    int asset_id;
    if (HAPI_LoadAssetLibraryFromFile(
            otl_file,
            &library_id) != HAPI_RESULT_SUCCESS)
    {
        cout << "Could not load " << otl_file << endl;
        exit(1);
    }

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

static void print_asset_info(const hapi::Asset &asset)
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

static void process_geo_part(const hapi::Part &part)
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

