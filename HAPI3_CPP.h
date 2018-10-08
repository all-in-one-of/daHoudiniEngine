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

Based on example file from SideFX

******************************************************************************/

#ifndef __HAPI_CPP_h__
#define __HAPI_CPP_h__

#include <HAPI/HAPI.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
// for cout
#include <iostream>

namespace hapi
{

//----------------------------------------------------------------------------
// Common error handling:

// Instances of this class are thrown whenever an underlying HAPI function
// call fails.
class Failure
{
public:
    Failure(HAPI_Result result)
    : result(result)
    {}

    // Retrieve details about the last non-successful HAPI function call.
    // You would typically call this method after catching a Failure exception,
    // but it can be called as a static method after calling a C HAPI function.
    static std::string lastErrorMessage(HAPI_Session* session)
    {
	int buffer_length;
	HAPI_GetStatusStringBufLength(
			session,
            HAPI_STATUS_CALL_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length);

	char * buf = new char[ buffer_length ];

	HAPI_GetStatusString(session, HAPI_STATUS_CALL_RESULT, buf, buffer_length);
        std::string result(buf);
	return result;
    }

    // Retrieve details about the last non-successful HAPI_CookAsset() or
    // HAPI_InstantiateAsset() function call.    
    static std::string lastCookErrorMessage(HAPI_Session* session)
    {
	int buffer_length;
	HAPI_GetStatusStringBufLength(
			session,
            HAPI_STATUS_COOK_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length);

	char * buf = new char[ buffer_length ];

	HAPI_GetStatusString(session, HAPI_STATUS_CALL_RESULT, buf, buffer_length);
        std::string result(buf);
	return result;
    }

    HAPI_Result result;
};

// This helper functions is used internally by the classes below.
static bool hasCallFailed(HAPI_Result result)
{
    return (result != HAPI_RESULT_SUCCESS);
}
static void throwOnFailure(HAPI_Result result)
{
    if (hasCallFailed(result))
	    throw Failure(result);
}

//----------------------------------------------------------------------------
// Utility functions:

// Return a std::string corresponding to a string handle.
static std::string getString(HAPI_Session* session, int string_handle)
{
    // A string handle of 0 means an invalid string handle -- similar to
    // a null pointer.  Since we can't return NULL, though, return an empty
    // string.
    if (string_handle == 0)
	return "";

    int buffer_length;
    throwOnFailure(HAPI_GetStringBufLength(session, string_handle, &buffer_length));

    char * buf = new char[ buffer_length ];

    throwOnFailure(HAPI_GetString(session, string_handle, buf, buffer_length));
    std::string result(buf);
    return result;
}

//----------------------------------------------------------------------------
// Classes:
// TODO:
// based on https://www.sidefx.com/docs/hengine/_h_a_p_i__assets.html
// there may be a need to include a Node class?
// assets are nodes, 
// objects are transform nodes, can contain object nodes or sops (geos)
// geos are nodes, they contain parts to render
//     usually, using getDisplayGeoInfo() gets what you want to show
//     may need to get more geos if you want to show templated geometry?
//     geos can be editable, ie put a mesh in here as part of otl
//     to cook
// parts are not nodes
//     could be meshes, curves, volumes, instances, etc
//     calculated based on primitive groups in houdini
//     n + 1 parts for n groups, other part contains primitives not in groups
//     if multiple primitives in group, mutliple parts for each
// all nodes have Parm info
// materials aren't either..

class Object;
class Parm;
class Asset;

class Node
{
public:
    Node(int id, HAPI_Session* mySession)
    : nodeid(id), _nodeInfo(NULL), session(mySession)
    {}

    Node(const Node &node)
    : nodeid(node.nodeid)
    , _nodeInfo(node._nodeInfo ? new HAPI_NodeInfo(*node._nodeInfo) : NULL)
	, session(node.session)
    {}

    ~Node()
    {
	delete this->_nodeInfo;
    }

    Node &operator=(const Node &node)
    {
	if (this != &node)
	{
	    delete this->_nodeInfo;
	    this->nodeid = node.nodeid;
	    this->_nodeInfo = node._nodeInfo
		? new HAPI_NodeInfo(*node._nodeInfo) : NULL;
	}
	return *this;
    }

    const HAPI_NodeInfo &nodeInfo() const
    {
	if (!this->_nodeInfo)
	{
	    this->_nodeInfo = new HAPI_NodeInfo();
	    throwOnFailure(HAPI_GetNodeInfo(
		session,
		nodeid, this->_nodeInfo));
	}
	return *this->_nodeInfo;
    }

    std::vector<Parm> parms() const;
    std::map<std::string, Parm> parmMap() const;

    bool isValid() const
    {
	HAPI_Bool is_valid = 0;
	try
	{
        // always succeeds
	    throwOnFailure(HAPI_IsNodeValid(
		session,
		this->nodeid, this->nodeInfo().uniqueHoudiniNodeId, &is_valid));
	    return is_valid;
	}
	catch (Failure &failure)
	{
	    return false;
	}
    }

    std::string name() const
    { return getString(session, nodeInfo().nameSH); }

    std::string internalNodePath() const
    { return getString(session, nodeInfo().internalNodePathSH); }

    void deleteNode() const
    { throwOnFailure(HAPI_DeleteNode(session, this->nodeid)); }

    void cook() const
    { throwOnFailure(HAPI_CookNode(session, this->nodeid, NULL)); }

    void cook(HAPI_CookOptions* cook_options) const
    { throwOnFailure(HAPI_CookNode(session, this->nodeid, cook_options)); }

    int nodeid;
	HAPI_Session* session;

protected:
    mutable HAPI_NodeInfo *_nodeInfo;
};

class Object;
class Parm;

class Asset : public Node
{
public:
    Asset(int nodeid, HAPI_Session* mySession)
    : Node(nodeid, mySession), _info(NULL)
    {}

    Asset(const Asset &asset) : Node(asset)
    , _info(asset._info ? new HAPI_AssetInfo(*asset._info) : NULL)
    {
    }

    ~Asset()
    {
	delete this->_info;
    }

    Asset &operator=(const Asset &asset)
    {
	if (this != &asset)
	{
        Node::operator=(asset);
	    delete this->_info;
	    this->_info = asset._info ? new HAPI_AssetInfo(*asset._info) : NULL;
	}
	return *this;
    }

    const HAPI_AssetInfo &info() const
    {
	if (!this->_info)
	{
	    this->_info = new HAPI_AssetInfo();
	    throwOnFailure(HAPI_GetAssetInfo(session, this->nodeid, this->_info));
	}
	return *this->_info;
    }

    std::vector<Object> objects() const;
    std::vector<HAPI_Transform> transforms() const;

    std::string label() const
    { return getString(session, info().labelSH); }

    std::string filePath() const
    { return getString(session, info().filePathSH); }

    HAPI_Transform getTransform( 
        HAPI_RSTOrder rst_order, int relative_to_node_id) const
    {
	HAPI_Transform result;
	throwOnFailure(HAPI_GetObjectTransform(
		session,
	    this->nodeid, relative_to_node_id, rst_order, &result));
	return result;
    }

    void getTransformAsMatrix(float result_matrix[16]) const
    {
        HAPI_Transform transform =
            this->getTransform( HAPI_SRT, -1 );
	throwOnFailure(HAPI_ConvertTransformQuatToMatrix(
		session,
	    &transform, result_matrix ) );
    }

private:
    mutable HAPI_AssetInfo *_info;
};

class Geo;

class Object
{
public:
    Object(int asset_id, int object_id, HAPI_Session* mySession)
    : asset(asset_id, mySession), id(object_id), _info(NULL), session(mySession)
    {}

    Object(const Asset &asset, int id)
    : asset(asset), id(id), _info(NULL), session(asset.session)
    {}

    Object(const Object &object)
    : asset(object.asset)
    , id(object.id)
    , _info(object._info ? new HAPI_ObjectInfo(*object._info) : NULL)
	, session(object.session)
    {}

    ~Object()
    { delete this->_info; }

    Object &operator=(const Object &object)
    {
	if (this != &object)
	{
	    delete this->_info;
	    asset = object.asset;
	    this->id = object.id;
	    this->_info = object._info
		? new HAPI_ObjectInfo(*object._info) : NULL;
	}
	return *this;
    }

    const HAPI_ObjectInfo &info() const
    {
	if (!this->_info)
	{
		std::cout << "Object: No info, fetching.." << std::endl;
		std::cout << "Object:   asset id: " << this->asset.nodeid << std::endl;
		std::cout << "Object:   object id: " << this->id << std::endl;
        _info = new HAPI_ObjectInfo();
		// this doesn't work!!
// 		throwOnFailure(HAPI_GetObjectInfo(
// 			session,
// 			this->asset.nodeid,
// 			_info));
		// this works..
        int objectCount;
        throwOnFailure(HAPI_ComposeObjectList( 
            session, this->asset.nodeid, NULL, &objectCount ));

		throwOnFailure(HAPI_GetComposedObjectList( 
		session, 
		this->asset.nodeid, 
		_info, 
		this->id, 
		1));
		cout << "Object: Check some info.." << std::endl << 
		"Object:   nodeId: " << _info->nodeId << std::endl << 
		"Object:   isVisible: " << _info->isVisible << std::endl << 
		"Object:   instancer: "<< _info->isInstancer << std::endl;
	}
	return *this->_info;
    }

    std::vector<Geo> geos() const;
    Geo displayGeo() const;

    std::string name() const
    { return getString(session, info().nameSH); }

    std::string objectInstancePath() const
    { return getString(session, info().objectInstancePathSH); }

    Asset asset;
    int id;
	HAPI_Session* session;

private:
    mutable HAPI_ObjectInfo *_info;
};

class Part;

// todo geos:
// expose group names
class Geo
{
public:
    Geo(const Object &object, int id)
    : object(object), id(id), _info(NULL), session(object.session)
    {}

    Geo(int asset_id, int object_id, int geo_id, HAPI_Session* mySession)
    : object(asset_id, object_id, mySession), id(geo_id), _info(NULL), session(mySession)
    {}

    Geo(const Geo &geo)
    : object(geo.object)
    , id(geo.id)
    , _info(geo._info ? new HAPI_GeoInfo(*geo._info) : NULL)
	, session(geo.session)
    {}

    ~Geo()
    { delete _info; }

    Geo &operator=(const Geo &geo)
    {
	if (this != &geo)
	{
	    delete this->_info;
	    this->object = geo.object;
	    this->id = geo.id;
	    this->_info = geo._info ? new HAPI_GeoInfo(*geo._info) : NULL;
	}
	return *this;
    }

    const HAPI_GeoInfo &info() const
    {
	if (!this->_info)
	{
		std::cout << "Geo: no info, fetching.." << std::endl;
		std::cout << "Geo:   with object id " << this->object.info().nodeId << std::endl;
	    this->_info = new HAPI_GeoInfo();
	    throwOnFailure(HAPI_GetDisplayGeoInfo(
		session,
		this->object.info().nodeId, this->_info));
	}
	return *this->_info;
    }

    std::string name() const
    { return getString(session, info().nameSH); }

    std::vector<Part> parts() const;

    Object object;
    int id;
	HAPI_Session* session;

private:
    mutable HAPI_GeoInfo *_info;
};

// TODO parts:
// put in getFaceCounts, getVertexList, getGroupMembership
// xyz points are actually point attributes, convenience function for it?
// 
class Part
{
public:
    Part(const Geo &geo, int id)
    : geo(geo), id(id), _info(NULL), session(geo.session)
    {}

    Part(int asset_id, int object_id, int geo_id, int part_id, HAPI_Session* mySession)
    : geo(asset_id, object_id, geo_id, mySession), id(part_id), _info(NULL), session(mySession)
    {}

    Part(const Part &part)
    : geo(part.geo)
    , id(part.id)
    , _info(part._info ? new HAPI_PartInfo(*part._info) : NULL)
	, session(part.session)
    {}

    ~Part()
    { delete _info; }

    Part &operator=(const Part &part)
    {
	if (this != &part)
	{
	    delete this->_info;
	    this->geo = part.geo;
	    this->id = part.id;
	    this->_info = part._info ? new HAPI_PartInfo(*part._info) : NULL;
	}
	return *this;
    }

    const HAPI_PartInfo &info() const
    {
	if (!this->_info)
	{
	    this->_info = new HAPI_PartInfo();
		std::cout << "Part: no info, fetching.." << std::endl;
		std::cout << "Part:   with geo's nodeid " << this->geo.info().nodeId << std::endl;
	    throwOnFailure(HAPI_GetPartInfo(
		    session,
		    this->geo.info().nodeId, 
            this->id, 
            this->_info ));
	}
	return *this->_info;
    }

    std::string name() const
    { return getString(session, info().nameSH); }

    int numAttribs(HAPI_AttributeOwner attrib_owner) const
    {
    	return this->info().attributeCounts[attrib_owner];
    }

    std::vector<std::string> attribNames(HAPI_AttributeOwner attrib_owner) const
    {
	int num_attribs = numAttribs(attrib_owner);
	std::vector<std::string> result;

    // HAPI_GetAttributeNames() will fail if num_attribs given is 0, even if
    // that is the number of attributes. Return empty string vector if that is the case.
    if (num_attribs == 0) {
        return result;
    }
	std::vector<int> attrib_names_sh(num_attribs);

	throwOnFailure(HAPI_GetAttributeNames(
		session,
	    this->geo.info().nodeId, 
	    this->id, attrib_owner, &attrib_names_sh[0], num_attribs));

	for (int attrib_index=0; attrib_index < int(attrib_names_sh.size());
		++attrib_index)
	    result.push_back(getString(session, attrib_names_sh[attrib_index]));
	return result;
    }

    HAPI_AttributeInfo attribInfo(
	HAPI_AttributeOwner attrib_owner, const char *attrib_name) const
    {
	HAPI_AttributeInfo result;
	throwOnFailure(HAPI_GetAttributeInfo(
		session,
	    this->geo.info().nodeId,
	    this->id, attrib_name, attrib_owner, &result));
	return result;
    }

    float *getNewFloatAttribData(
	HAPI_AttributeInfo &attrib_info, const char *attrib_name,
	int start=0, int length=-1) const
    {
	if (length < 0)
	    length = attrib_info.count - start;

	float *result = new float[attrib_info.count * attrib_info.tupleSize];
	throwOnFailure(HAPI_GetAttributeFloatData(
		session,
	    this->geo.info().nodeId, 
	    this->id, attrib_name, &attrib_info, /*stride=*/-1,
        result, /*start=*/0, attrib_info.count));
	return result;
    }

    int *getNewIntAttribData(
	HAPI_AttributeInfo &attrib_info, const char *attrib_name,
	int start=0, int length=-1) const
    {
	if (length < 0)
	    length = attrib_info.count - start;

	int *result = new int[attrib_info.count * attrib_info.tupleSize];
	throwOnFailure(HAPI_GetAttributeIntData(
		session,
	    this->geo.info().nodeId, 
	    this->id, attrib_name, &attrib_info, /*stride=*/-1,
        result, /*start=*/0, attrib_info.count));
	return result;
    }

    std::vector< HAPI_NodeId > materialNodeIdsOnFaces(bool &all_same) const;

    Geo geo;
    int id;
	HAPI_Session* session;

private:
    mutable HAPI_PartInfo *_info;
};

class ParmChoice;

// todo: 
// get only folders of parms
// implement the radio button thing
// implement parameter presets (https://www.sidefx.com/docs/hengine/_h_a_p_i__parameters.html#HAPI_Parameters_Presets)
class Parm
{
public:
    // This constructor is required only for std::map::operator[] for the case
    // where a Parm object does not exist in the map.
    Parm()
    { throw std::out_of_range("Invalid parameter name"); }

    Parm(int node_id, const HAPI_ParmInfo &info,
	HAPI_ParmChoiceInfo *all_choice_infos, HAPI_Session* mySession);

    const HAPI_ParmInfo &info() const
    { return _info; }

    std::string name() const
    { return getString(session, _info.nameSH); }

    std::string label() const
    { return getString(session, _info.labelSH); }

    int getIntValue(int sub_index) const
    {
	int result;
	throwOnFailure(HAPI_GetParmIntValues(
		session,
	    this->node_id, &result, this->_info.intValuesIndex + sub_index,
	    /*length=*/1));
	return result;
    }

    float getFloatValue(int sub_index) const
    {
	float result;
	throwOnFailure(HAPI_GetParmFloatValues(
		session,
	    this->node_id, &result, this->_info.floatValuesIndex + sub_index,
	    /*length=*/1));
	return result;
    }

    std::string getStringValue(int sub_index) const
    {
	int string_handle;
	throwOnFailure(HAPI_GetParmStringValues(
		session,
	    this->node_id, true, &string_handle,
	    this->_info.stringValuesIndex + sub_index, /*length=*/1));
	return getString(session, string_handle);
    }

    void setIntValue(int sub_index, int value)
    {
	throwOnFailure(HAPI_SetParmIntValues(
		session,
	    this->node_id, &value, this->_info.intValuesIndex + sub_index,
	    /*length=*/1));
    }

    void setFloatValue(int sub_index, float value)
    {
	throwOnFailure(HAPI_SetParmFloatValues(
		session,
	    this->node_id, &value, this->_info.floatValuesIndex + sub_index,
	    /*length=*/1));
    }

    void setStringValue(int sub_index, const char *value)
    {
	throwOnFailure(HAPI_SetParmStringValue(
		session,
	    this->node_id, value, this->_info.id, sub_index));
    }

    void insertMultiparmInstance(int instance_position)
    {
	throwOnFailure(HAPI_InsertMultiparmInstance(
		session,
	    this->node_id, this->_info.id, instance_position));
    }

    void removeMultiparmInstance(int instance_position)
    {
	throwOnFailure(HAPI_RemoveMultiparmInstance(
		session,
	    this->node_id, this->_info.id, instance_position));
    }

    int node_id;
    std::vector<ParmChoice> choices;
	HAPI_Session* session;

private:
    HAPI_ParmInfo _info;
};

class ParmChoice
{
public:
    ParmChoice(HAPI_ParmChoiceInfo &info, HAPI_Session* mySession)
    : _info(info), session(mySession)
    {}

    const HAPI_ParmChoiceInfo &info() const
    { return _info; }

    std::string label() const
    { return getString(session, _info.labelSH); }

    std::string value() const
    { return getString(session, _info.valueSH); }

	HAPI_Session* session;
private:
    HAPI_ParmChoiceInfo _info;
};

// Methods that could not be declared inside the classes:

inline Parm::Parm(int node_id, const HAPI_ParmInfo &info,
	HAPI_ParmChoiceInfo *all_choice_infos, HAPI_Session* mySession)
    : node_id(node_id), _info(info), session(mySession)
{
    for (int i=0; i < info.choiceCount; ++i)
	this->choices.push_back(ParmChoice(
	    all_choice_infos[info.choiceIndex + i], session));
}

inline std::vector<Object> Asset::objects() const
{
    std::vector<Object> result;

    int objectCount;
    throwOnFailure(HAPI_ComposeObjectList( 
        session, nodeid, NULL, &objectCount ));

    for (int object_id=0; object_id < objectCount; ++object_id) {
	    result.push_back(Object(*this, object_id));
    }
    std::cout << "Asset: getting " << result.size() << " objects" << std::endl;
    return result;
}

inline std::vector< HAPI_Transform > Asset::transforms() const
{
    int object_count = 0;
    throwOnFailure(HAPI_ComposeObjectList(
        session, nodeid, NULL, &object_count ));
    std::vector< HAPI_ObjectInfo > object_infos( object_count );
    throwOnFailure(HAPI_GetComposedObjectList(
        this->session, 
        this->nodeid, 
        object_infos.data(), 
        0, 
        object_count ));
    std::vector< HAPI_Transform > result( object_count );
    throwOnFailure(HAPI_GetComposedObjectTransforms(
        this->session, 
        this->nodeid, 
        HAPI_RSTORDER_DEFAULT, 
        result.data(), 
        0, 
        object_count ));
    std::cout << "Object: getting " <<  object_count << " transforms" << std::endl;

    return result;
}

inline std::vector<Parm> Node::parms() const
{
    // Get all the parm infos.
    int num_parms = nodeInfo().parmCount;
    std::vector<HAPI_ParmInfo> parm_infos(num_parms);
    throwOnFailure(HAPI_GetParameters(
	this->session,
	this->nodeid, &parm_infos[0], /*start=*/0, num_parms));

    // Get all the parm choice infos.
    std::vector<HAPI_ParmChoiceInfo> parm_choice_infos(
	this->nodeInfo().parmChoiceCount);
    throwOnFailure(HAPI_GetParmChoiceLists(
	session,
	this->nodeid, &parm_choice_infos[0], /*start=*/0,
	this->nodeInfo().parmChoiceCount));

    // Build and return a vector of Parm objects.
    std::vector<Parm> result;
    for (int i=0; i < num_parms; ++i)
	result.push_back(Parm(
	    this->nodeid, parm_infos[i], &parm_choice_infos[0], this->session));
    return result;
}

inline std::map<std::string, Parm> Node::parmMap() const
{
    std::vector<Parm> parms = this->parms();

    std::map<std::string, Parm> result;
    for (int i=0; i < int(parms.size()); ++i)
	result.insert(std::make_pair(parms[i].name(), parms[i]));

    return result;
}

inline std::vector<Geo> Object::geos() const
{
    int geoCount = 1;
    std::vector<Geo> result;

    result.push_back(this->displayGeo());

    return result;

    // old way
    /*
    std::vector<Geo> result;
    for (int geo_id=0; geo_id < info().geoCount; ++geo_id)
	result.push_back(Geo(*this, geo_id));
    return result;
    */
}
inline Geo Object::displayGeo() const
{
    std::cout << "Object: fetching display geo" << std::endl;
    return Geo(*this, 0);

}

inline std::vector<Part> Geo::parts() const
{
    std::vector<Part> result;
    std::cout << "Geo: getting parts for geo with id " << this->id << " part count is " << info().partCount << std::endl;
    for (int part_id=0; part_id < info().partCount; ++part_id)
	result.push_back(Part(*this, part_id));
    return result;
}

inline std::vector< HAPI_NodeId > Part::materialNodeIdsOnFaces(bool &all_same) const
{
    std::vector< HAPI_NodeId > result( info().faceCount );
    throwOnFailure(HAPI_GetMaterialNodeIdsOnFaces(
		session,
		geo.info().nodeId,
		id,
		&all_same /* are_all_the_same*/,
		result.data(), 
		0, info().faceCount ));
    return result;
}

};

#endif
