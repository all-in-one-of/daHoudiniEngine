#include <HAPI/HAPI.h>
#include <stdlib.h>
#include <iostream>
#include <string>

using namespace std;

#define ENSURE_SUCCESS( result ) \
    if ( (result) != HAPI_RESULT_SUCCESS ) \
{ \
    cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
    cout << get_last_error() << endl; \
    exit( 1 ); \
}
#define ENSURE_COOK_SUCCESS( result ) \
    if ( (result) != HAPI_STATE_READY ) \
{ \
    cout << "failure at " << __FILE__ << ":" << __LINE__ << endl; \
    cout << get_last_cook_error() << endl; \
    exit( 1 ); \
}

static void print_asset_info( int asset_id, const HAPI_AssetInfo &asset_info );

static void process_geo_part(
    int asset_id, int object_id, int geo_id, int part_id );

static void process_float_attrib(
    int asset_id, int object_id, int geo_id, int part_id,
    HAPI_AttributeOwner attrib_owner,
    const char *attrib_name );

static std::string get_string( HAPI_StringHandle string_handle );
static std::string get_last_error();
static std::string get_last_cook_error();
static void wait_for_cook();

int main( int argc, char **argv )
{
    const char *otl_file = argc == 2 ? argv[ 1 ] : "test_asset.otl";
    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
    ENSURE_SUCCESS( HAPI_Initialize(
        NULL,
        /*dso_search_path=*/ NULL,
        &cook_options,
        /*use_cooking_thread=*/ true,
        /*cooking_thread_max_size=*/ -1 ) );
    int library_id;
    int asset_id;
    if ( HAPI_LoadAssetLibraryFromFile(
        otl_file,
        &library_id ) != HAPI_RESULT_SUCCESS )
    {
        cout << "Could not load " << otl_file << endl;
        exit( 1 );
    }
    HAPI_StringHandle asset_name_sh;
    ENSURE_SUCCESS( HAPI_GetAvailableAssets( library_id, &asset_name_sh, 1 ) );
    std::string asset_name = get_string( asset_name_sh );
    if ( HAPI_InstantiateAsset(
        asset_name.c_str(),
        /* cook_on_load */ true,
        &asset_id ) != HAPI_RESULT_SUCCESS )
    {
        cout << "Could not instantiate asset " << asset_name << endl;
        exit( 1 );
    }
    wait_for_cook();
    // Retrieve information about the asset.
    HAPI_AssetInfo asset_info;
    ENSURE_SUCCESS( HAPI_GetAssetInfo( asset_id, &asset_info ) );
    // Print information about the geometry contained inside the asset.
    print_asset_info( asset_id, asset_info );
    ENSURE_SUCCESS( HAPI_Cleanup() );
    return 0;
}
static void wait_for_cook()
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
static void print_asset_info( int asset_id, const HAPI_AssetInfo &asset_info )
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
static void process_geo_part(
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
            cout << "vertex :" << curr_index << ", belonging to face: " << ii <<", index: "
                << vertex_list[ curr_index ] << " of points array\n";
            curr_index++;
        }
    }
    delete[] face_counts;
    delete[] vertex_list;
}
void process_float_attrib(
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
            cout << attrib_data[
                elem_index * attrib_info.tupleSize + tuple_index ]
            << " ";
        }
        cout << endl;
    }
    delete [] attrib_data;
}
static std::string get_string( HAPI_StringHandle string_handle )
{
    // A string handle of 0 means an invalid string handle -- similar to
    // a null pointer.  Since we can't return NULL, though, return an empty
    // string.
    if ( string_handle == 0 )
        return "";
    int buffer_length;
    ENSURE_SUCCESS( HAPI_GetStringBufLength( string_handle, &buffer_length ) );
    char * buf = new char[ buffer_length ];
    ENSURE_SUCCESS( HAPI_GetString( string_handle, buf, buffer_length ) );
    std::string result( buf );
    delete[] buf;
    return result;
}
static std::string get_last_error()
{
    int buffer_length;
    HAPI_GetStatusStringBufLength(
        HAPI_STATUS_CALL_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length );
    char * buf = new char[ buffer_length ];
    HAPI_GetStatusString( HAPI_STATUS_CALL_RESULT, buf );
    std::string result( buf );
    delete[] buf;
    return result;
}
static std::string get_last_cook_error()
{
    int buffer_length;
    HAPI_GetStatusStringBufLength(
        HAPI_STATUS_COOK_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length );
    char * buf = new char[ buffer_length ];
    HAPI_GetStatusString( HAPI_STATUS_CALL_RESULT, buf );
    std::string result( buf );
    delete[] buf;
    return result;
}