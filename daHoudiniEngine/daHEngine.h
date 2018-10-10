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

******************************************************************************/


#ifndef __HOUDINI_ENGINE_H__
#define __HOUDINI_ENGINE_H__

#ifdef WIN32
        #ifndef HE_STATIC
                #ifdef daHengine_EXPORTS
                   #define HE_API    __declspec(dllexport)
                #else
                   #define HE_API    __declspec(dllimport)
                #endif
        #else
                #define HE_API
        #endif
#else
        #define HE_API
#endif

#ifndef DA_ENABLE_HENGINE
	#define DA_ENABLE_HENGINE 0
#endif


#include <cyclops/cyclops.h>
#if DA_ENABLE_HENGINE > 0
#include "HAPI3_CPP.h"
#endif

#define OMEGA_NO_GL_HEADERS
#include <omega.h>
#include <omegaOsg/omegaOsg.h>
#include <omegaToolkit.h>

#include "daHoudiniEngine/houdiniAsset.h"

#define hlog(msg) if(HoudiniEngine::isLoggingEnabled()) olog(StringUtils::logLevel, msg)
#define hflog(fmt, args) if(HoudiniEngine::isLoggingEnabled()) oflog(StringUtils::logLevel, fmt, args)

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

	typedef vector<String> MyList;

#if DA_ENABLE_HENGINE > 0
	#define ENSURE_SUCCESS(session, result) \
	    if ((result) != HAPI_RESULT_SUCCESS) \
	    { \
		oferror("failure at %1%:%2%", %__FILE__ %__LINE__); \
		oferror("%1% '%2%'", %hapi::Failure::lastErrorMessage(session) %result);\
		exit(1); \
	    }

	#define ENSURE_COOK_SUCCESS(session, result) \
	    if ((result) != HAPI_STATE_READY) \
	    { \
		oferror("cook failure at %1%:%2%", %__FILE__ %__LINE__); \
		oferror("%1% '%2%'", %hapi::Failure::lastCookErrorMessage(session) %result);\
		exit(1); \
	    }

	static std::string get_string(HAPI_Session* session, int string_handle);

	class HE_API RefAsset: public hapi::Asset, public ReferenceType
	{
	public:
		RefAsset(int nodeid, HAPI_Session* mySession) : Asset(nodeid, mySession)
		{ oflog(Debug, "[RefAsset] from id %1%", %nodeid); }
		RefAsset(const hapi::Asset &asset) : hapi::Asset(asset)
		{ oflog(Debug, "[RefAsset] from asset %1%", %asset.name()); }

		~RefAsset()
		{ oflog(Debug, "[~RefAsset] id %1%", %nodeid); }

	};

#endif
	//forward references
	class HE_API HoudiniGeometry;
	class HE_API HoudiniUiParm;

	class BillboardCallback;

	///////////////////////////////////////////////////////////////////////////////////////////////////
	class HE_API HoudiniEngine: public EngineModule
	{
	public:
		HoudiniEngine();
		~HoudiniEngine();

#if DA_ENABLE_HENGINE > 0
		static HoudiniEngine* instance() { return myInstance; }

		// log level for Houdini Engine
		// StringUtils::Verbose when logging enabled
		static StringUtils::LogLevel logLevel;

		// >>>> subclassed modules
		//! Convenience method to create the module, register and initialize it.
		static HoudiniEngine* createAndInitialize();

		virtual void initialize();

		virtual void update(const UpdateContext& context);
		virtual void onMenuItemEvent(MenuItem* mi);
		virtual void onSelectedChanged(SceneNode* source, bool value);


		virtual void handleEvent(const Event& evt);

		virtual void commitSharedData(SharedOStream& out);
		virtual void updateSharedData(SharedIStream& in);

		// <<<<<< subclassed modules

		// helper functions
		// helper functions
		void process_asset(
			const hapi::Asset &asset
		);
		void process_object(
			const hapi::Object &object,
			const int objIndex,
			HoudiniGeometry* hg
		);
		void process_geo(
			const hapi::Geo &geo,
			const int objIndex,
			const int geoIndex,
			HoudiniGeometry* hg
		);
		void process_part(
			const hapi::Part &part,
			const int objIndex,
			const int geoIndex,
			const int partIndex,
			HoudiniGeometry* hg
		);

		void process_materials(
			const hapi::Part &part,
			HoudiniGeometry* hg
		);

		void process_float_attrib(
		    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
		    const char *attrib_name, vector<Vector3f>& points
		);

		void process_attrib(
		    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
		    const char *attrib_name, vector<float>& vals
		);

		void process_attrib(
		    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
		    const char *attrib_name, vector<int>& vals
		);

		// from HAPI 3.1
// 		CreateInProcessSession (HAPI_Session *session);
// 		StartThriftSocketServer (const HAPI_ThriftServerOptions *options, int port, HAPI_ProcessId *process_id);
// 		CreateThriftSocketSession (HAPI_Session *session, const char *host_name, int port);
// 		StartThriftNamedPipeServer (const HAPI_ThriftServerOptions *options, const char *pipe_name, HAPI_ProcessId *process_id);
// 		CreateThriftNamedPipeSession (HAPI_Session *session, const char *pipe_name);
// 		BindCustomImplementation (HAPI_SessionType session_type, const char *dll_path);
// 		CreateCustomSession (HAPI_SessionType session_type, void *session_info, HAPI_Session *session);
// 		IsSessionValid (const HAPI_Session *session);
// 		CloseSession (const HAPI_Session *session);
// 		IsInitialized (const HAPI_Session *session);
// 		Initialize (const HAPI_Session *session, const HAPI_CookOptions *cook_options, HAPI_Bool use_cooking_thread, int cooking_thread_stack_size, const char *houdini_environment_files, const char *otl_search_path, const char *dso_search_path, const char *image_dso_search_path, const char *audio_dso_search_path);
// 		Cleanup (const HAPI_Session *session);
// 		GetEnvInt (HAPI_EnvIntType int_type, int *value);
// 		GetSessionEnvInt (const HAPI_Session *session, HAPI_SessionEnvIntType int_type, int *value);
// 		GetServerEnvInt (const HAPI_Session *session, const char *variable_name, int *value);
// 		GetServerEnvString (const HAPI_Session *session, const char *variable_name, HAPI_StringHandle *value);
// 		SetServerEnvInt (const HAPI_Session *session, const char *variable_name, int value);
// 		SetServerEnvString (const HAPI_Session *session, const char *variable_name, const char *value);
// 		GetStatus (const HAPI_Session *session, HAPI_StatusType status_type, int *status);
// 		GetStatusStringBufLength (const HAPI_Session *session, HAPI_StatusType status_type, HAPI_StatusVerbosity verbosity, int *buffer_length);
// 		GetStatusString (const HAPI_Session *session, HAPI_StatusType status_type, char *string_value, int length);
// 		ComposeNodeCookResult (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_StatusVerbosity verbosity, int *buffer_length);
// 		GetComposedNodeCookResult (const HAPI_Session *session, char *string_value, int length);
// 		CheckForSpecificErrors (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ErrorCodeBits errors_to_look_for, HAPI_ErrorCodeBits *errors_found);
// 		GetCookingTotalCount (const HAPI_Session *session, int *count);
// 		GetCookingCurrentCount (const HAPI_Session *session, int *count);
// 		ConvertTransform (const HAPI_Session *session, const HAPI_TransformEuler *transform_in, HAPI_RSTOrder rst_order, HAPI_XYZOrder rot_order, HAPI_TransformEuler *transform_out);
// 		ConvertMatrixToQuat (const HAPI_Session *session, const float *matrix, HAPI_RSTOrder rst_order, HAPI_Transform *transform_out);
// 		ConvertMatrixToEuler (const HAPI_Session *session, const float *matrix, HAPI_RSTOrder rst_order, HAPI_XYZOrder rot_order, HAPI_TransformEuler *transform_out);
// 		ConvertTransformQuatToMatrix (const HAPI_Session *session, const HAPI_Transform *transform, float *matrix);
// 		ConvertTransformEulerToMatrix (const HAPI_Session *session, const HAPI_TransformEuler *transform, float *matrix);
// 		PythonThreadInterpreterLock (const HAPI_Session *session, HAPI_Bool locked);
// 		GetStringBufLength (const HAPI_Session *session, HAPI_StringHandle string_handle, int *buffer_length);
// 		GetString (const HAPI_Session *session, HAPI_StringHandle string_handle, char *string_value, int length);
// 		GetTime (const HAPI_Session *session, float *time);
// 		SetTime (const HAPI_Session *session, float time);
// 		GetTimelineOptions (const HAPI_Session *session, HAPI_TimelineOptions *timeline_options);
// 		SetTimelineOptions (const HAPI_Session *session, const HAPI_TimelineOptions *timeline_options);
// 		LoadAssetLibraryFromFile (const HAPI_Session *session, const char *file_path, HAPI_Bool allow_overwrite, HAPI_AssetLibraryId *library_id);
// 		LoadAssetLibraryFromMemory (const HAPI_Session *session, const char *library_buffer, int library_buffer_length, HAPI_Bool allow_overwrite, HAPI_AssetLibraryId *library_id);
// 		GetAvailableAssetCount (const HAPI_Session *session, HAPI_AssetLibraryId library_id, int *asset_count);
// 		GetAvailableAssets (const HAPI_Session *session, HAPI_AssetLibraryId library_id, HAPI_StringHandle *asset_names_array, int asset_count);
// 		GetAssetInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_AssetInfo *asset_info);
// 		Interrupt (const HAPI_Session *session);
// 		LoadHIPFile (const HAPI_Session *session, const char *file_name, HAPI_Bool cook_on_load);
// 		SaveHIPFile (const HAPI_Session *session, const char *file_path, HAPI_Bool lock_nodes);
// 		IsNodeValid (const HAPI_Session *session, HAPI_NodeId node_id, int unique_node_id, HAPI_Bool *answer);
// 		GetNodeInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_NodeInfo *node_info);
// 		GetNodePath (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_NodeId relative_to_node_id, HAPI_StringHandle *path);
// 		GetManagerNodeId (const HAPI_Session *session, HAPI_NodeType node_type, HAPI_NodeId *node_id);
// 		ComposeChildNodeList (const HAPI_Session *session, HAPI_NodeId parent_node_id, HAPI_NodeTypeBits node_type_filter, HAPI_NodeFlagsBits node_flags_filter, HAPI_Bool recursive, int *count);
// 		GetComposedChildNodeList (const HAPI_Session *session, HAPI_NodeId parent_node_id, HAPI_NodeId *child_node_ids_array, int count);
// 		CreateNode (const HAPI_Session *session, HAPI_NodeId parent_node_id, const char *operator_name, const char *node_label, HAPI_Bool cook_on_creation, HAPI_NodeId *new_node_id);
// 		CreateInputNode (const HAPI_Session *session, HAPI_NodeId *node_id, const char *name);
// 		CookNode (const HAPI_Session *session, HAPI_NodeId node_id, const HAPI_CookOptions *cook_options);
// 		DeleteNode (const HAPI_Session *session, HAPI_NodeId node_id);
// 		RenameNode (const HAPI_Session *session, HAPI_NodeId node_id, const char *new_name);
// 		ConnectNodeInput (const HAPI_Session *session, HAPI_NodeId node_id, int input_index, HAPI_NodeId node_id_to_connect);
// 		DisconnectNodeInput (const HAPI_Session *session, HAPI_NodeId node_id, int input_index);
// 		QueryNodeInput (const HAPI_Session *session, HAPI_NodeId node_to_query, int input_index, HAPI_NodeId *connected_node_id);
// 		GetNodeInputName (const HAPI_Session *session, HAPI_NodeId node_id, int input_idx, HAPI_StringHandle *name);
// 		GetParameters (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmInfo *parm_infos_array, int start, int length);
// 		GetParmInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, HAPI_ParmInfo *parm_info);
// 		GetParmIdFromName (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, HAPI_ParmId *parm_id);
// 		GetParmInfoFromName (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, HAPI_ParmInfo *parm_info);
// 		GetParmTagName (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, int tag_index, HAPI_StringHandle *tag_name);
// 		GetParmTagValue (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, const char *tag_name, HAPI_StringHandle *tag_value);
// 		ParmHasTag (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, const char *tag_name, HAPI_Bool *has_tag);
// 		ParmHasExpression (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index, HAPI_Bool *has_expression);
// 		GetParmWithTag (const HAPI_Session *session, HAPI_NodeId node_id, const char *tag_name, HAPI_ParmId *parm_id);
// 		GetParmExpression (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index, HAPI_StringHandle *value);
// 		RevertParmToDefault (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index);
// 		RevertParmToDefaults (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name);
// 		SetParmExpression (const HAPI_Session *session, HAPI_NodeId node_id, const char *value, HAPI_ParmId parm_id, int index);
// 		RemoveParmExpression (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, int index);
// 		GetParmIntValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index, int *value);
// 		GetParmIntValues (const HAPI_Session *session, HAPI_NodeId node_id, int *values_array, int start, int length);
// 		GetParmFloatValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index, float *value);
// 		GetParmFloatValues (const HAPI_Session *session, HAPI_NodeId node_id, float *values_array, int start, int length);
// 		GetParmStringValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index, HAPI_Bool evaluate, HAPI_StringHandle *value);
// 		GetParmStringValues (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_Bool evaluate, HAPI_StringHandle *values_array, int start, int length);
// 		GetParmNodeValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, HAPI_NodeId *value);
// 		GetParmFile (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, const char *destination_directory, const char *destination_file_name);
// 		GetParmChoiceLists (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmChoiceInfo *parm_choices_array, int start, int length);
// 		SetParmIntValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index, int value);
// 		SetParmIntValues (const HAPI_Session *session, HAPI_NodeId node_id, const int *values_array, int start, int length);
// 		SetParmFloatValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, int index, float value);
// 		SetParmFloatValues (const HAPI_Session *session, HAPI_NodeId node_id, const float *values_array, int start, int length);
// 		SetParmStringValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *value, HAPI_ParmId parm_id, int index);
// 		SetParmNodeValue (const HAPI_Session *session, HAPI_NodeId node_id, const char *parm_name, HAPI_NodeId value);
// 		InsertMultiparmInstance (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, int instance_position);
// 		RemoveMultiparmInstance (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, int instance_position);
// 		GetHandleInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_HandleInfo *handle_infos_array, int start, int length);
// 		GetHandleBindingInfo (const HAPI_Session *session, HAPI_NodeId node_id, int handle_index, HAPI_HandleBindingInfo *handle_binding_infos_array, int start, int length);
// 		GetPresetBufLength (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PresetType preset_type, const char *preset_name, int *buffer_length);
// 		GetPreset (const HAPI_Session *session, HAPI_NodeId node_id, char *buffer, int buffer_length);
// 		SetPreset (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PresetType preset_type, const char *preset_name, const char *buffer, int buffer_length);
// 		GetObjectInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ObjectInfo *object_info);
// 		GetObjectTransform (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_NodeId relative_to_node_id, HAPI_RSTOrder rst_order, HAPI_Transform *transform);
// 		ComposeObjectList (const HAPI_Session *session, HAPI_NodeId parent_node_id, const char *categories, int *object_count);
// 		GetComposedObjectList (const HAPI_Session *session, HAPI_NodeId parent_node_id, HAPI_ObjectInfo *object_infos_array, int start, int length);
// 		GetComposedObjectTransforms (const HAPI_Session *session, HAPI_NodeId parent_node_id, HAPI_RSTOrder rst_order, HAPI_Transform *transform_array, int start, int length);
// 		GetInstancedObjectIds (const HAPI_Session *session, HAPI_NodeId object_node_id, HAPI_NodeId *instanced_node_id_array, int start, int length);
// 		GetInstanceTransforms (const HAPI_Session *session, HAPI_NodeId object_node_id, HAPI_RSTOrder rst_order, HAPI_Transform *transforms_array, int start, int length);
// 		SetObjectTransform (const HAPI_Session *session, HAPI_NodeId node_id, const HAPI_TransformEuler *trans);
// 		GetDisplayGeoInfo (const HAPI_Session *session, HAPI_NodeId object_node_id, HAPI_GeoInfo *geo_info);
// 		GetGeoInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_GeoInfo *geo_info);
// 		GetPartInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_PartInfo *part_info);
// 		GetFaceCounts (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int *face_counts_array, int start, int length);
// 		GetVertexList (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int *vertex_list_array, int start, int length);
// 		GetAttributeInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, HAPI_AttributeOwner owner, HAPI_AttributeInfo *attr_info);
// 		GetAttributeNames (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_AttributeOwner owner, HAPI_StringHandle *attribute_names_array, int count);
// 		GetAttributeIntData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, HAPI_AttributeInfo *attr_info, int stride, int *data_array, int start, int length);
// 		GetAttributeInt64Data (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, HAPI_AttributeInfo *attr_info, int stride, HAPI_Int64 *data_array, int start, int length);
// 		GetAttributeFloatData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, HAPI_AttributeInfo *attr_info, int stride, float *data_array, int start, int length);
// 		GetAttributeFloat64Data (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, HAPI_AttributeInfo *attr_info, int stride, double *data_array, int start, int length);
// 		GetAttributeStringData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, HAPI_AttributeInfo *attr_info, HAPI_StringHandle *data_array, int start, int length);
// 		GetGroupNames (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_GroupType group_type, HAPI_StringHandle *group_names_array, int group_count);
// 		GetGroupMembership (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_GroupType group_type, const char *group_name, HAPI_Bool *membership_array_all_equal, int *membership_array, int start, int length);
// 		GetInstancedPartIds (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_PartId *instanced_parts_array, int start, int length);
// 		GetInstancerPartTransforms (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_RSTOrder rst_order, HAPI_Transform *transforms_array, int start, int length);
// 		SetPartInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const HAPI_PartInfo *part_info);
// 		SetFaceCounts (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const int *face_counts_array, int start, int length);
// 		SetVertexList (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const int *vertex_list_array, int start, int length);
// 		AddAttribute (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, const HAPI_AttributeInfo *attr_info);
// 		SetAttributeIntData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, const HAPI_AttributeInfo *attr_info, const int *data_array, int start, int length);
// 		SetAttributeInt64Data (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, const HAPI_AttributeInfo *attr_info, const HAPI_Int64 *data_array, int start, int length);
// 		SetAttributeFloatData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, const HAPI_AttributeInfo *attr_info, const float *data_array, int start, int length);
// 		SetAttributeFloat64Data (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, const HAPI_AttributeInfo *attr_info, const double *data_array, int start, int length);
// 		SetAttributeStringData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, const HAPI_AttributeInfo *attr_info, const char **data_array, int start, int length);
// 		AddGroup (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_GroupType group_type, const char *group_name);
// 		SetGroupMembership (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_GroupType group_type, const char *group_name, const int *membership_array, int start, int length);
// 		CommitGeo (const HAPI_Session *session, HAPI_NodeId node_id);
// 		RevertGeo (const HAPI_Session *session, HAPI_NodeId node_id);
// 		GetMaterialNodeIdsOnFaces (const HAPI_Session *session, HAPI_NodeId geometry_node_id, HAPI_PartId part_id, HAPI_Bool *are_all_the_same, HAPI_NodeId *material_ids_array, int start, int length);
// 		GetMaterialInfo (const HAPI_Session *session, HAPI_NodeId material_node_id, HAPI_MaterialInfo *material_info);
// 		RenderCOPToImage (const HAPI_Session *session, HAPI_NodeId cop_node_id);
// 		RenderTextureToImage (const HAPI_Session *session, HAPI_NodeId material_node_id, HAPI_ParmId parm_id);
// 		GetImageInfo (const HAPI_Session *session, HAPI_NodeId material_node_id, HAPI_ImageInfo *image_info);
// 		SetImageInfo (const HAPI_Session *session, HAPI_NodeId material_node_id, const HAPI_ImageInfo *image_info);
// 		GetImagePlaneCount (const HAPI_Session *session, HAPI_NodeId material_node_id, int *image_plane_count);
// 		GetImagePlanes (const HAPI_Session *session, HAPI_NodeId material_node_id, HAPI_StringHandle *image_planes_array, int image_plane_count);
// 		ExtractImageToFile (const HAPI_Session *session, HAPI_NodeId material_node_id, const char *image_file_format_name, const char *image_planes, const char *destination_folder_path, const char *destination_file_name, int *destination_file_path);
// 		ExtractImageToMemory (const HAPI_Session *session, HAPI_NodeId material_node_id, const char *image_file_format_name, const char *image_planes, int *buffer_size);
// 		GetImageMemoryBuffer (const HAPI_Session *session, HAPI_NodeId material_node_id, char *buffer, int length);
// 		GetSupportedImageFileFormatCount (const HAPI_Session *session, int *file_format_count);
// 		GetSupportedImageFileFormats (const HAPI_Session *session, HAPI_ImageFileFormat *formats_array, int file_format_count);
// 		SetAnimCurve (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_ParmId parm_id, int parm_index, const HAPI_Keyframe *curve_keyframes_array, int keyframe_count);
// 		SetTransformAnimCurve (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_TransformComponent trans_comp, const HAPI_Keyframe *curve_keyframes_array, int keyframe_count);
// 		ResetSimulation (const HAPI_Session *session, HAPI_NodeId node_id);
// 		GetVolumeInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_VolumeInfo *volume_info);
// 		GetFirstVolumeTile (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_VolumeTileInfo *tile);
// 		GetNextVolumeTile (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_VolumeTileInfo *tile);
// 		GetVolumeVoxelFloatData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int x_index, int y_index, int z_index, float *values_array, int value_count);
// 		GetVolumeTileFloatData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, float fill_value, const HAPI_VolumeTileInfo *tile, float *values_array, int length);
// 		GetVolumeVoxelIntData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int x_index, int y_index, int z_index, int *values_array, int value_count);
// 		GetVolumeTileIntData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int fill_value, const HAPI_VolumeTileInfo *tile, int *values_array, int length);
// 		GetHeightFieldData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, float *values_array, int start, int length);
// 		SetVolumeInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const HAPI_VolumeInfo *volume_info);
// 		SetVolumeTileFloatData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const HAPI_VolumeTileInfo *tile, const float *values_array, int length);
// 		SetVolumeTileIntData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const HAPI_VolumeTileInfo *tile, const int *values_array, int length);
// 		SetVolumeVoxelFloatData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int x_index, int y_index, int z_index, const float *values_array, int value_count);
// 		SetVolumeVoxelIntData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int x_index, int y_index, int z_index, const int *values_array, int value_count);
// 		GetVolumeBounds (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, float *x_min, float *y_min, float *z_min, float *x_max, float *y_max, float *z_max, float *x_center, float *y_center, float *z_center);
// 		SetHeightFieldData (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const char *name, const float *values_array, int start, int length);
// 		GetCurveInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, HAPI_CurveInfo *info);
// 		GetCurveCounts (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int *counts_array, int start, int length);
// 		GetCurveOrders (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, int *orders_array, int start, int length);
// 		GetCurveKnots (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, float *knots_array, int start, int length);
// 		SetCurveInfo (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const HAPI_CurveInfo *info);
// 		SetCurveCounts (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const int *counts_array, int start, int length);
// 		SetCurveOrders (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const int *orders_array, int start, int length);
// 		SetCurveKnots (const HAPI_Session *session, HAPI_NodeId node_id, HAPI_PartId part_id, const float *knots_array, int start, int length);
// 		GetBoxInfo (const HAPI_Session *session, HAPI_NodeId geo_node_id, HAPI_PartId part_id, HAPI_BoxInfo *box_info);
// 		GetSphereInfo (const HAPI_Session *session, HAPI_NodeId geo_node_id, HAPI_PartId part_id, HAPI_SphereInfo *sphere_info);
// 		GetActiveCacheCount (const HAPI_Session *session, int *active_cache_count);
// 		GetActiveCacheNames (const HAPI_Session *session, HAPI_StringHandle *cache_names_array, int active_cache_count);
// 		GetCacheProperty (const HAPI_Session *session, const char *cache_name, HAPI_CacheProperty cache_property, int *property_value);
// 		SetCacheProperty (const HAPI_Session *session, const char *cache_name, HAPI_CacheProperty cache_property, int property_value);
// 		SaveGeoToFile (const HAPI_Session *session, HAPI_NodeId node_id, const char *file_name);
// 		LoadGeoFromFile (const HAPI_Session *session, HAPI_NodeId node_id, const char *file_name);
// 		GetGeoSize (const HAPI_Session *session, HAPI_NodeId node_id, const char *format, int *size);
// 		SaveGeoToMemory (const HAPI_Session *session, HAPI_NodeId node_id, char *buffer, int length);
// 		LoadGeoFromMemory (const HAPI_Session *session, HAPI_NodeId node_id, const char *format, const char *buffer, int length);


		// * old stuff
 		// GetAvailableAssets (const HAPI_Session *session, HAPI_AssetLibraryId library_id, HAPI_StringHandle *asset_names_array, int asset_count);
		// returns a list of the available assets for the given library_id
		// TODO: make this for all libraries and return a dict?
		boost::python::list getAvailableAssets(int library_id);


		int loadAssetLibraryFromFile(const String& otlFile);
		int getAvailableAssetCount() { return myAssetCount; };
		int instantiateAsset(const String& asset);
		int instantiateAssetById(int asset_id);
		HoudiniAsset* instantiateGeometry(const String& asset);

		Ref< RefAsset > getAsset(int asset_id) { return instancedHEAssets[asset_id]; }

		HoudiniGeometry* getHG(const String& asset) { return myHoudiniGeometrys[asset]; };

		Menu* getMenu(const String& asset) { return NULL; } // return this asset's parameter menu

		Container* getParmCont(int i, int asset_id = 0);
		Container* getParmBase(int i, int asset_id = 0);
		void doIt(int asset_id = 0);
		void test(int arg, int arg2);

		void createMenu(const int asset_id);
		void createParms(const int asset_id, Container* assetCont);
		void initializeParameters(const String& asset_name);

		// python methods for HAPI Parameters
		boost::python::dict getParameters(const String& asset_name);
		void setParameterValue(const String& asset_name, const String& parm_name, boost::python::object value);
		boost::python::object getParameterValue(const String& asset_name, const String& parm_name);
		void insertMultiparmInstance(const String& asset_name, const String& parm_name, int pos);
		void removeMultiparmInstance(const String& asset_name, const String& parm_name, int pos);

		boost::python::list getParameterChoices(const String& asset_name, const String& parm_name);

        int getIntegerParameterValue(const String& asset_name, int param_id, int sub_index);
        void setIntegerParameterValue(const String& asset_name, int param_id, int sub_index, int value);

        float getFloatParameterValue(const String& asset_name, int param_id, int sub_index);
        void setFloatParameterValue(const String& asset_name, int param_id, int sub_index, float value);

        String getStringParameterValue(const String& asset_name, int param_id, int sub_index);
        void setStringParameterValue(const String& asset_name, int param_id, int sub_index, const String& value);

		float getFps();

		float getTime();
		void setTime(float time);

		void cook();
        void cook_one(hapi::Asset* asset);
		void wait_for_cook();

		void setCookOptions(HAPI_CookOptions co) { myCookOptions = co; };
		HAPI_CookOptions getCookOptions() { return myCookOptions; };

		void setLoggingEnabled(const bool toggle);
		bool isLoggingEnabled() { return HoudiniEngine::myLogEnabled; };

		void showMappings();

		void printParms(int asset_id);

		// print the scenegraph of the houdini asset
		void printGraph(const String& asset_name);

		Container* getContainerForAsset(int n);
		Container* getHoudiniCont() { return houdiniCont; };
		Container* getStagingCont() { return stagingCont; };


	private:

		void createMenuItem(const String& asset_name, ui::Menu* menu, hapi::Parm* parm);
		void createParm(const String& asset_name, Container* cont, hapi::Parm* parm);
		// */

	private:
		//helper function
		void removeConts(Container* cont);

		SceneManager* mySceneManager;

		// Scene editor. This will be used to manipulate the object.
		SceneEditorModule* myEditor;

		// Menu stuff.
		MenuManager* myMenuManager;
		MenuItem* myQuitMenuItem;

		// Houdini Engine Stuff
	    int library_id; // need to put into an array for multiple libraries?

	    // indicate when to update geos
		bool updateGeos;

		// this is the model definition, not the instance of it
		// I change the verts, faces, normals, etc in this and HoudiniAssets
		// in the scene get updated accordingly
		typedef Dictionary<String, Ref<HoudiniGeometry> > HGDictionary;
		typedef Dictionary<int, Ref<RefAsset> > Mapping;

		// geometries
		HGDictionary myHoudiniGeometrys;

		// Materials/textures
		vector <Ref<PixelData> > pds;

		// HoudiniAsset instances
		// used for updating materials/textures of assets
		Dictionary<String, Ref<HoudiniAsset> > assetInstances;

		// this is only maintained on the master
		Mapping instancedHEAssets;

		//parameters
		// make it look like this:
		//  Houdini Engine > Container
		// Container:
		// ._________._________.
		// | Asset 1 |_Asset_2_|_________.
		// | ._____._____.               |
		// | | FL1 |_FL2_|______.        |
		// | |                  |        |
		// | | A |22| ----||--  |        |
		// | |                  |        |
		// | | B |43| --||----  |        |
		// | |   |11| ||------  |        |
		// | |                  |        |
		// | | C | Hello World ||        |
		// | |__________________|        |
		// |_____________________________|

		typedef struct MenuObject {
			MenuItem* mi;
			Menu* m;
		} MenuObject;

		typedef Dictionary<String, vector<MenuItem> > Menus;
		typedef Dictionary<String, vector<Container*> > ParmConts;
		ui::Menu* houdiniMenu;
		ui::Container* houdiniCont; // the menu container
		ui::Container* assetChoiceCont; // the container to indicate which asset to show
		ui::Container* stagingCont; // the container to show the contents of the selected folder

		Vector<Container*> assetConts; // keep refs to parameters for this asset

		// the link between widget and parmId
		Dictionary < int, String > widgetIdToParmName; // UI Widget -> HAPI_Parm (asset.parmMap())

		// houdiniUiParms by asset_ID
		// can't forward reference nested class HoudiniUiParm, so this is current
		// workaround
		Dictionary < int, vector<Ref<ReferenceType> > > uiParms;

		// asset name to id
		Dictionary < String, int > assetNameToIds;

		// parm value container..
		typedef struct {
			int type;
			Vector<int> intValues;
			Vector<float> floatValues;
			Vector<String> stringValues;
		} ParmStruct;

		// material container..
		// TODO: better way to do this?
		typedef struct {
			int matId;
			int partId;
			int geoId;
			int objId;
			Dictionary<String, ParmStruct> parms;
		} MatStruct;
        // asset name to material parms
        // eg: assetMaterialParms["cluster1"][4]["ogl_diff"]
        Dictionary < String, Vector< MatStruct > > assetMaterialParms;

		// logging
		static bool myLogEnabled;

		// session
		HAPI_Session* session;

		int myAssetCount;

		// build a list of widgets to remove
		Vector<Widget* > removeTheseWidgets;

		static HoudiniEngine* myInstance;

		HAPI_CookOptions myCookOptions;

#endif
	};
};

#endif
