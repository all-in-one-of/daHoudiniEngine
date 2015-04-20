#ifndef __HAPI_CPP_h__
#define __HAPI_CPP_h__

#include <HAPI/HAPI.h>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

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
    static std::string lastErrorMessage()
    {
	int buffer_length;
	HAPI_GetStatusStringBufLength(
            HAPI_STATUS_CALL_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length);

	char * buf = new char[ buffer_length ];

	HAPI_GetStatusString(HAPI_STATUS_CALL_RESULT, buf);
        std::string result(buf);
	return result;
    }

    // Retrieve details about the last non-successful HAPI_CookAsset() or
    // HAPI_InstantiateAsset() function call.    
    static std::string lastCookErrorMessage()
    {
	int buffer_length;
	HAPI_GetStatusStringBufLength(
            HAPI_STATUS_COOK_RESULT, HAPI_STATUSVERBOSITY_ERRORS, &buffer_length);

	char * buf = new char[ buffer_length ];

	HAPI_GetStatusString(HAPI_STATUS_CALL_RESULT, buf);
        std::string result(buf);
	return result;
    }

    HAPI_Result result;
};

// This helper function is used internally by the classes below.
static void throwOnFailure(HAPI_Result result)
{
    if (result != HAPI_RESULT_SUCCESS)
	throw Failure(result);
}

//----------------------------------------------------------------------------
// Utility functions:

// Return a std::string corresponding to a string handle.
static std::string getString(int string_handle)
{
    // A string handle of 0 means an invalid string handle -- similar to
    // a null pointer.  Since we can't return NULL, though, return an empty
    // string.
    if (string_handle == 0)
	return "";

    int buffer_length;
    throwOnFailure(HAPI_GetStringBufLength(string_handle, &buffer_length));

    char * buf = new char[ buffer_length ];

    throwOnFailure(HAPI_GetString(string_handle, buf, buffer_length));
    std::string result(buf);
    return result;
}

//----------------------------------------------------------------------------
// Classes:

class Object;
class Parm;

class Asset
{
public:
    Asset(int id)
    : id(id), _info(NULL), _nodeInfo(NULL)
    {}

    Asset(const Asset &asset)
    : id(asset.id)
    , _info(asset._info ? new HAPI_AssetInfo(*asset._info) : NULL)
    , _nodeInfo(asset._nodeInfo ? new HAPI_NodeInfo(*asset._nodeInfo) : NULL)
    {}

    ~Asset()
    {
	delete this->_info;
	delete this->_nodeInfo;
    }

    Asset &operator=(const Asset &asset)
    {
	if (this != &asset)
	{
	    delete this->_info;
	    delete this->_nodeInfo;
	    this->id = asset.id;
	    this->_info = asset._info ? new HAPI_AssetInfo(*asset._info) : NULL;
	    this->_nodeInfo = asset._nodeInfo
		? new HAPI_NodeInfo(*asset._nodeInfo) : NULL;
	}
	return *this;
    }

    const HAPI_AssetInfo &info() const
    {
	if (!this->_info)
	{
	    this->_info = new HAPI_AssetInfo();
	    throwOnFailure(HAPI_GetAssetInfo(this->id, this->_info));
	}
	return *this->_info;
    }

    const HAPI_NodeInfo &nodeInfo() const
    {
	if (!this->_nodeInfo)
	{
	    this->_nodeInfo = new HAPI_NodeInfo();
	    throwOnFailure(HAPI_GetNodeInfo(
		this->info().nodeId, this->_nodeInfo));
	}
	return *this->_nodeInfo;
    }

    std::vector<Object> objects() const;
    std::vector<Parm> parms() const;
    std::map<std::string, Parm> parmMap() const;

    bool isValid() const
    {
	int is_valid = 0;
	try
	{
	    // Note that calling info() might fail if the info isn't cached
	    // and the asest id is invalid.
	    throwOnFailure(HAPI_IsAssetValid(
		this->id, this->info().validationId, &is_valid));
	    return is_valid;
	}
	catch (Failure &failure)
	{
	    return false;
	}
    }

    std::string name() const
    { return getString(info().nameSH); }

    std::string label() const
    { return getString(info().labelSH); }

    std::string filePath() const
    { return getString(info().filePathSH); }

    void destroyAsset() const
    { throwOnFailure(HAPI_DestroyAsset(this->id)); }

    void cook() const
    { throwOnFailure(HAPI_CookAsset(this->id, NULL)); }

    HAPI_TransformEuler getTransform( 
        HAPI_RSTOrder rst_order, HAPI_XYZOrder rot_order) const
    {
	HAPI_TransformEuler result;
	throwOnFailure(HAPI_GetAssetTransform(
	    this->id, rst_order, rot_order, &result));
	return result;
    }

    void getTransformAsMatrix(float result_matrix[16]) const
    {
        HAPI_TransformEuler transform =
            this->getTransform( HAPI_SRT, HAPI_XYZ );
	throwOnFailure(HAPI_ConvertTransformEulerToMatrix(
	    &transform, result_matrix ) );
    }

    int id;

private:
    mutable HAPI_AssetInfo *_info;
    mutable HAPI_NodeInfo *_nodeInfo;
};

class Geo;

class Object
{
public:
    Object(int asset_id, int object_id)
    : asset(asset_id), id(object_id), _info(NULL)
    {}

    Object(const Asset &asset, int id)
    : asset(asset), id(id), _info(NULL)
    {}

    Object(const Object &object)
    : asset(object.asset)
    , id(object.id)
    , _info(object._info ? new HAPI_ObjectInfo(*object._info) : NULL)
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
	    this->_info = new HAPI_ObjectInfo();
	    throwOnFailure(HAPI_GetObjects(
		this->asset.id, this->_info, this->id, /*length=*/1));
	}
	return *this->_info;
    }

    std::vector<Geo> geos() const;

    std::string name() const
    { return getString(info().nameSH); }

    std::string objectInstancePath() const
    { return getString(info().objectInstancePathSH); }

    Asset asset;
    int id;

private:
    mutable HAPI_ObjectInfo *_info;
};

class Part;

class Geo
{
public:
    Geo(const Object &object, int id)
    : object(object), id(id), _info(NULL)
    {}

    Geo(int asset_id, int object_id, int geo_id)
    : object(asset_id, object_id), id(geo_id), _info(NULL)
    {}

    Geo(const Geo &geo)
    : object(geo.object)
    , id(geo.id)
    , _info(geo._info ? new HAPI_GeoInfo(*geo._info) : NULL)
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
	    this->_info = new HAPI_GeoInfo();
	    throwOnFailure(HAPI_GetGeoInfo(
		this->object.asset.id, this->object.id, this->id, this->_info));
	}
	return *this->_info;
    }

    std::string name() const
    { return getString(info().nameSH); }

    std::vector<Part> parts() const;

    Object object;
    int id;

private:
    mutable HAPI_GeoInfo *_info;
};

class Part
{
public:
    Part(const Geo &geo, int id)
    : geo(geo), id(id), _info(NULL)
    {}

    Part(int asset_id, int object_id, int geo_id, int part_id)
    : geo(asset_id, object_id, geo_id), id(part_id), _info(NULL)
    {}

    Part(const Part &part)
    : geo(part.geo)
    , id(part.id)
    , _info(part._info ? new HAPI_PartInfo(*part._info) : NULL)
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
	    throwOnFailure(HAPI_GetPartInfo(
		this->geo.object.asset.id, this->geo.object.id, this->geo.id,
		this->id, this->_info));
	}
	return *this->_info;
    }

    std::string name() const
    { return getString(info().nameSH); }

    int numAttribs(HAPI_AttributeOwner attrib_owner) const
    {
	switch (attrib_owner)
	{
	    case HAPI_ATTROWNER_VERTEX:
		return this->info().vertexAttributeCount;
	    case HAPI_ATTROWNER_POINT:
		return this->info().pointAttributeCount;
	    case HAPI_ATTROWNER_PRIM:
		return this->info().faceAttributeCount;
	    case HAPI_ATTROWNER_DETAIL:
		return this->info().detailAttributeCount;
	    case HAPI_ATTROWNER_MAX:
	    case HAPI_ATTROWNER_INVALID:
		break;
	}

	return 0;
    }

    std::vector<std::string> attribNames(HAPI_AttributeOwner attrib_owner) const
    {
	int num_attribs = numAttribs(attrib_owner);
	std::vector<int> attrib_names_sh(num_attribs);

	throwOnFailure(HAPI_GetAttributeNames(
	    this->geo.object.asset.id, this->geo.object.id, this->geo.id,
	    this->id, attrib_owner, &attrib_names_sh[0], num_attribs));

	std::vector<std::string> result;
	for (int attrib_index=0; attrib_index < int(attrib_names_sh.size());
		++attrib_index)
	    result.push_back(getString(attrib_names_sh[attrib_index]));
	return result;
    }

    HAPI_AttributeInfo attribInfo(
	HAPI_AttributeOwner attrib_owner, const char *attrib_name) const
    {
	HAPI_AttributeInfo result;
	throwOnFailure(HAPI_GetAttributeInfo(
	    this->geo.object.asset.id, this->geo.object.id, this->geo.id,
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
	    this->geo.object.asset.id, this->geo.object.id, this->geo.id,
	    this->id, attrib_name, &attrib_info, result,
	    /*start=*/0, attrib_info.count));
	return result;
    }

    Geo geo;
    int id;

private:
    mutable HAPI_PartInfo *_info;
};

class ParmChoice;

class Parm
{
public:
    // This constructor is required only for std::map::operator[] for the case
    // where a Parm object does not exist in the map.
    Parm()
    { throw std::out_of_range("Invalid parameter name"); }

    Parm(int node_id, const HAPI_ParmInfo &info,
	HAPI_ParmChoiceInfo *all_choice_infos);

    const HAPI_ParmInfo &info() const
    { return _info; }

    std::string name() const
    { return getString(_info.nameSH); }

    std::string label() const
    { return getString(_info.labelSH); }

    int getIntValue(int sub_index) const
    {
	int result;
	throwOnFailure(HAPI_GetParmIntValues(
	    this->node_id, &result, this->_info.intValuesIndex + sub_index,
	    /*length=*/1));
	return result;
    }

    float getFloatValue(int sub_index) const
    {
	float result;
	throwOnFailure(HAPI_GetParmFloatValues(
	    this->node_id, &result, this->_info.floatValuesIndex + sub_index,
	    /*length=*/1));
	return result;
    }

    std::string getStringValue(int sub_index) const
    {
	int string_handle;
	throwOnFailure(HAPI_GetParmStringValues(
	    this->node_id, true, &string_handle,
	    this->_info.stringValuesIndex + sub_index, /*length=*/1));
	return getString(string_handle);
    }

    void setIntValue(int sub_index, int value)
    {
	throwOnFailure(HAPI_SetParmIntValues(
	    this->node_id, &value, this->_info.intValuesIndex + sub_index,
	    /*length=*/1));
    }

    void setFloatValue(int sub_index, float value)
    {
	throwOnFailure(HAPI_SetParmFloatValues(
	    this->node_id, &value, this->_info.floatValuesIndex + sub_index,
	    /*length=*/1));
    }

    void setStringValue(int sub_index, const char *value)
    {
	throwOnFailure(HAPI_SetParmStringValue(
	    this->node_id, value, this->_info.id, sub_index));
    }

    void insertMultiparmInstance(int instance_position)
    {
	throwOnFailure(HAPI_InsertMultiparmInstance(
	    this->node_id, this->_info.id, instance_position));
    }

    void removeMultiparmInstance(int instance_position)
    {
	throwOnFailure(HAPI_RemoveMultiparmInstance(
	    this->node_id, this->_info.id, instance_position));
    }

    int node_id;
    std::vector<ParmChoice> choices;

private:
    HAPI_ParmInfo _info;
};

class ParmChoice
{
public:
    ParmChoice(HAPI_ParmChoiceInfo &info)
    : _info(info)
    {}

    const HAPI_ParmChoiceInfo &info() const
    { return _info; }

    std::string label() const
    { return getString(_info.labelSH); }

    std::string value() const
    { return getString(_info.valueSH); }

private:
    HAPI_ParmChoiceInfo _info;
};

// Methods that could not be declared inside the classes:

inline Parm::Parm(int node_id, const HAPI_ParmInfo &info,
	HAPI_ParmChoiceInfo *all_choice_infos)
    : node_id(node_id), _info(info)
{
    for (int i=0; i < info.choiceCount; ++i)
	this->choices.push_back(ParmChoice(
	    all_choice_infos[info.choiceIndex + i]));
}

inline std::vector<Object> Asset::objects() const
{
    std::vector<Object> result;
    for (int object_id=0; object_id < info().objectCount; ++object_id)
	result.push_back(Object(*this, object_id));
    return result;
}

inline std::vector<Parm> Asset::parms() const
{
    // Get all the parm infos.
    int num_parms = nodeInfo().parmCount;
    std::vector<HAPI_ParmInfo> parm_infos(num_parms);
    throwOnFailure(HAPI_GetParameters(
	this->info().nodeId, &parm_infos[0], /*start=*/0, num_parms));

    // Get all the parm choice infos.
    std::vector<HAPI_ParmChoiceInfo> parm_choice_infos(
	this->nodeInfo().parmChoiceCount);
    throwOnFailure(HAPI_GetParmChoiceLists(
	this->info().nodeId, &parm_choice_infos[0], /*start=*/0,
	this->nodeInfo().parmChoiceCount));

    // Build and return a vector of Parm objects.
    std::vector<Parm> result;
    for (int i=0; i < num_parms; ++i)
	result.push_back(Parm(
	    this->info().nodeId, parm_infos[i], &parm_choice_infos[0]));
    return result;
}

inline std::map<std::string, Parm> Asset::parmMap() const
{
    std::vector<Parm> parms = this->parms();

    std::map<std::string, Parm> result;
    for (int i=0; i < int(parms.size()); ++i)
	result.insert(std::make_pair(parms[i].name(), parms[i]));

    return result;
}

inline std::vector<Geo> Object::geos() const
{
    std::vector<Geo> result;
    for (int geo_id=0; geo_id < info().geoCount; ++geo_id)
	result.push_back(Geo(*this, geo_id));
    return result;
}

inline std::vector<Part> Geo::parts() const
{
    std::vector<Part> result;
    for (int part_id=0; part_id < info().partCount; ++part_id)
	result.push_back(Part(*this, part_id));
    return result;
}

};

#endif
