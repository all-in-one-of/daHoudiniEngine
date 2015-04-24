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
    HAPI_NodeInfo node_info;
    ENSURE_SUCCESS( HAPI_GetNodeInfo( asset_info.nodeId, &node_info ) );
    // Get information about all the parameters.
    HAPI_ParmInfo *parm_infos = new HAPI_ParmInfo[ node_info.parmCount ];
    ENSURE_SUCCESS( HAPI_GetParameters(
        asset_info.nodeId, parm_infos, /*start=*/ 0, node_info.parmCount ) );


        cout << "Parms:" << endl;
        for ( int i=0; i < node_info.parmCount; ++i )
        {
            cout << "   name: " << get_string( parm_infos[ i ].nameSH ) << endl;
            cout << " values: (";
            for ( int sub_index = 0; sub_index < parm_infos[ i ].size; ++sub_index )
            {
                if ( sub_index != 0 )
                    cout << ", ";
                if ( HAPI_ParmInfo_IsInt( &parm_infos[ i ] ) )
                {
                    int parm_value;
                    ENSURE_SUCCESS( HAPI_GetParmIntValues(
                        asset_info.nodeId, &parm_value,
                        parm_infos[ i ].intValuesIndex + sub_index,
                        /*length=*/ 1 ) );
                    cout << parm_value;
                }
                else if ( HAPI_ParmInfo_IsFloat( &parm_infos[ i ] ) )
                {
                    float parm_value;
                    ENSURE_SUCCESS( HAPI_GetParmFloatValues(
                        asset_info.nodeId, &parm_value,
                        parm_infos[i].floatValuesIndex + sub_index,
                        /*length=*/ 1 ) );
                    cout << parm_value;
                }
                else if ( HAPI_ParmInfo_IsString( &parm_infos[ i ] ) )
                {
                    HAPI_StringHandle parm_value_sh;
                    ENSURE_SUCCESS( HAPI_GetParmStringValues(
                        asset_info.nodeId, true, &parm_value_sh,
                        parm_infos[ i ].stringValuesIndex + sub_index,
                        /*length=*/1 ) );
                    cout << get_string( parm_value_sh );
                }
            }
            cout << ")" << endl;
        }
        // Print information about the geometry contained inside the asset.
        //print_asset_info( asset_id, asset_info );
        delete [] parm_infos;
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
        HAPI_GetStatus( HAPI_STATUS_COOK_STATE, &status );
    }
    while ( status > HAPI_STATE_MAX_READY_STATE );
    ENSURE_COOK_SUCCESS( status );
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