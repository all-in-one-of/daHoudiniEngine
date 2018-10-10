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

daHEngine
	module to display geometry from Houdini Engine in omegalib
	this file does the copying of HEngine geometry to omegalib geometry

******************************************************************************/

#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>
#include <daHEngine.static.cpp>

// for printVisitor
#include <osgUtil/PrintVisitor>
#include <ostream>

using namespace houdiniEngine;

// Temporary debugging visitor (TODO: move this somewhere else to use more
// often?)
class MyPrintVisitor: public osgUtil::PrintVisitor
{
    public:

        MyPrintVisitor(std::ostream& out, int indent=0, int step=2):
			osgUtil::PrintVisitor(out, indent, step)
		{
		}

        void apply(osg::Node& node) {
			output()<<node.libraryName()<<"::"<<node.className()<<std::endl;

			enter();
			traverse(node);
			leave();
		}

        void apply(osg::PositionAttitudeTransform& node) {
			output()<<node.libraryName()<<"::"<<node.className()<<std::endl;
			output()<<"pos: ("<<node.getPosition()[0]<<
				","<<node.getPosition()[1]<<
				","<<node.getPosition()[2]<<
				")"<<std::endl;
			enter();
			traverse(node);
			leave();
		}

        void apply(osg::Geode& geo) {
			output()<<geo.libraryName()<<"::"<<geo.className()<<std::endl;
			enter();
			for (int i = 0; i < geo.getNumDrawables(); ++i) {
				apply(*(geo.getDrawable(i)));
			}
			leave();
		}

		void apply(osg::Drawable& draw) {
			output()<<"  "<< draw.className() << "(" << draw.getName() <<")" << std::endl;
			output()<<"    Verts:"<< draw.asGeometry()->getVertexArray()->getNumElements() << std::endl;
			output()<<"    PrimSets:"<< draw.asGeometry()->getNumPrimitiveSets() << std::endl;
		}

    protected:

        MyPrintVisitor& operator = (const MyPrintVisitor&) { return *this; }

};

void HoudiniEngine::printGraph(const String& asset_name) {

	HoudiniGeometry* hg = myHoudiniGeometrys[asset_name];

	MyPrintVisitor mpv(std::cout,0, 2);

	mpv.apply(*(hg->getOsgNode()));
	ofmsg("[HoudiniEngine::printGraph] materials under %1%", %asset_name);
	typedef Dictionary <String, ParmStruct > PS;
	typedef Dictionary < String, Vector< MatStruct > > Amps;
    foreach(Amps::Item amp, assetMaterialParms) {
		ofmsg("[HoudiniEngine::printGraph] %1%", %amp.first);

		for (int i = 0; i < amp.second.size(); ++i) {
			ofmsg("[HoudiniEngine::printGraph]   mat id: %1%, part: %2% geo: %3% obj: %4%", 
				%amp.second[i].matId 
				%amp.second[i].partId
				%amp.second[i].geoId
				%amp.second[i].objId
			);
			ologaddnewline(false);
			foreach(PS::Item ps, amp.second[i].parms) {
				ofmsg("[HoudiniEngine::printGraph]     %1%:%2% ", %ps.first %ps.second.type);
				for (int j = 0; j < ps.second.intValues.size(); ++j) {
					if (j == 0) omsg("    int ");
					ofmsg("%1% ", %ps.second.intValues[j]);
				}
				for (int j = 0; j < ps.second.floatValues.size(); ++j) {
					if (j == 0) omsg("    float ");
					ofmsg("%1% ", %ps.second.floatValues[j]);
				}
				for (int j = 0; j < ps.second.stringValues.size(); ++j) {
					if (j == 0) omsg("    string ");
					ofmsg("%1% ", %ps.second.stringValues[j]);
				}
				omsg("\n");
			}
			ologaddnewline(true);
		}

	}
}

// put houdini engine asset data into a houdiniGeometry
void HoudiniEngine::process_asset(const hapi::Asset &asset)
{

	String s = ostr("%1%", %asset.name());

	HoudiniGeometry* hg;

	hflog("[HoudiniEngine::process_asset] asset '%1%'", %s);

	if (myHoudiniGeometrys.count(s) > 0) {
		hg = myHoudiniGeometrys[s];
	} else {
		hg = HoudiniGeometry::create(s);
		myHoudiniGeometrys[s] = hg;
	}

    vector<hapi::Object> objects = asset.objects();
	vector<HAPI_Transform> objTransforms = asset.transforms();

	// ofmsg("process_assets: clear %1% materials", %assetMaterialParms[s].size());
	// assetMaterialParms[s].clear();

	hflog("[HoudiniEngine::process_asset] %1%: %2% objects %3% transforms", %asset.name() %objects.size() %objTransforms.size());

	hg->objectsChanged = asset.info().haveObjectsChanged;
	hflog("[HoudiniEngine::process_asset] %1%: %2% objects %3%", %asset.name() %objects.size() %(hg->objectsChanged == 1 ? "Changed" : ""));
	// forcing true for testing
	// hg->objectsChanged = true;
	// ofmsg("process_assets: %1%: %2% objects Forced Changed", %asset.name() %objects.size() );

	// set number of objects in HoudiniGeometry to match
	if (hg->getObjectCount() < objects.size()) {
		hg->addObject(objects.size() - hg->getObjectCount());

        for (int i=0; i < objects.size(); i++) {
            hg->setObjectName(i, objects[i].name());
        }
	}

	// if (hg->objectsChanged) {
	// still need to traverse this, as an object may not change, but geos in it can change
	if (true) {
		hflog("[HoudiniEngine::process_asset] iterating through %1% objects", %objects.size());
		for (int object_index=0; object_index < int(objects.size()); ++object_index)
	    {
			process_object(objects[object_index], object_index, hg);
			// if (hg->getTransformChanged(object_index)) {
			if (true) {
				hg->getOsgNode()->asGroup()->getChild(object_index)->asTransform()->
					asPositionAttitudeTransform()->setPosition(osg::Vec3d(
						objTransforms[object_index].position[0],
						objTransforms[object_index].position[1],
						objTransforms[object_index].position[2]
					)
				);

				hg->getOsgNode()->asGroup()->getChild(object_index)->asTransform()->
					asPositionAttitudeTransform()->setAttitude(osg::Quat(
						objTransforms[object_index].rotationQuaternion[0],
						objTransforms[object_index].rotationQuaternion[1],
						objTransforms[object_index].rotationQuaternion[2],
						objTransforms[object_index].rotationQuaternion[3]
					)
				);

				hg->getOsgNode()->asGroup()->getChild(object_index)->asTransform()->
					asPositionAttitudeTransform()->setScale(osg::Vec3d(
						objTransforms[object_index].scale[0],
						objTransforms[object_index].scale[1],
						objTransforms[object_index].scale[2]
					)
				);
			}
	    }
	}

	if (mySceneManager->getModel(s) == NULL) {
		hflog("[HoudiniEngine::process_asset] %1% not in sceneManager, adding..", %s);
		mySceneManager->addModel(hg);
	}

	if (myLogEnabled) {
		printGraph(s);
	}
}

void HoudiniEngine::process_object(const hapi::Object &object, const int objIndex, HoudiniGeometry* hg)
{
	HAPI_ObjectInfo objInfo = object.info();

	// TODO check for instancing, then do things differently
	if (objInfo.isInstancer > 0) {
		hflog("[HoudiniEngine::process_object]   INSTANCE:  instance path: %1%: %2%", %object.name() %object.objectInstancePath());
	}
	// don't use vector, as there should be only one geo
	// unless we are exposing editable nodes (not yet)
	vector<hapi::Geo> geos = object.geos();

	// adjust geo count
	if (hg->getGeodeCount(objIndex) < geos.size()) {
		hg->addGeode(geos.size() - hg->getGeodeCount(objIndex), objIndex);

		for (int i=0; i < geos.size(); i++) {
			hg->setGeodeName(i, objIndex, geos[i].name());
		}
	}

	hg->setGeosChanged(objInfo.haveGeosChanged, objIndex);
	hflog("[HoudiniEngine::process_object]   %1%/%2%: %3% %4%",
		%(objIndex + 1)
		%geos.size()
		%object.name()
		%(hg->getGeosChanged(objIndex) == 1 ? "Changed" : "")
	);

	// if (hg->getGeosChanged(objIndex)) {
	if (true) {
		hflog("[HoudiniEngine::process_object]   iterating through %1% geos", %geos.size());

		for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
		{
			process_geo(geos[geo_index], objIndex, geo_index, hg);
		}
	}

	hg->setTransformChanged(objInfo.hasTransformChanged, objIndex);
	hflog("[HoudiniEngine::process_object]   Transform changed: %2%", %objIndex %(hg->getTransformChanged(objIndex) == 1 ? "Yes" : "No"));
}


void HoudiniEngine::process_geo(const hapi::Geo &geo, const int objIndex, const int geoIndex, HoudiniGeometry* hg)
{
	vector<hapi::Part> parts = geo.parts();

	if (hg->getDrawableCount(geoIndex, objIndex) < parts.size()) {
		hg->addDrawable(parts.size() - hg->getDrawableCount(geoIndex, objIndex), geoIndex, objIndex);
	}

	hg->setGeoChanged(geo.info().hasGeoChanged, geoIndex, objIndex);
	hflog("[HoudiniEngine::process_geo]     %1%:%2%/%3% %4% %5% %6%",
		%(objIndex + 1)
		%(geoIndex + 1)
		%parts.size()
		%(hg->getGeoChanged(geoIndex, objIndex) == 1 ? "Changed" : "")
		%(geo.info().isDisplayGeo == 1 ? "Display" : "-")
		%(geo.info().isTemplated == 1 ? "Template" : "-")
	);

	hg->clearGeode(geoIndex, objIndex);
	// if (hg->getGeoChanged(geoIndex, objIndex)) {
	if (true) {
		hflog("[HoudiniEngine::process_geo] iterating through %1% parts", %parts.size());
		for (int part_index=0; part_index < int(parts.size()); ++part_index)
		{
			if (geo.info().isDisplayGeo) {
				hflog("[HoudiniEngine::process_geo]     processing %1%", %parts[part_index].name());

				// update the geometry and materials in each part
				process_part(parts[part_index], objIndex, geoIndex, part_index, hg);
			}
		}
	}
}

// TODO: expand on this.. (to do with curve rendering)
Vector3f bez(float t, Vector3f a, Vector3f b) {
	return Vector3f(
		a[0] + t *(b[0] - a[0]),
		a[1] + t *(b[1] - a[1]),
		a[2] + t *(b[2] - a[2])
	);
}

// TODO: incrementally update the geometry?
// send a new version, and still have the old version?
//     write it out to a file based on a hash of parameters
void HoudiniEngine::process_part(const hapi::Part &part, const int objIndex, const int geoIndex, const int partIndex, HoudiniGeometry* hg)
{
	hflog("[HoudiniEngine::process_part] processing %1%", %part.name());
	// TODO: is there a better way to convert from Vector3f to osg::Vec3?
	// Vector3f is from the Eigen lib
	// Vec3Array is from osg
	vector<Vector3f> points;
	vector<Vector3f> normals;
	vector<Vector3f> colors;
	vector<float> alphas;

	vector<float> ppx;
	vector<float> ppy;
	vector<float> ppz;
	vector<int> object_ids;
	// texture coordinates
	vector<Vector3f> uvs;

	bool has_point_normals = false;
	bool has_vertex_normals = false;
	bool has_point_colors = false;
	bool has_point_alphas = false;
	bool has_vertex_colors = false;
	bool has_primitive_colors = false;

	bool has_point_uvs = false;
	bool has_vertex_uvs = false;

	bool has_pivotpoint_x = false;
	bool has_pivotpoint_y = false;
	bool has_pivotpoint_z = false;
	bool has_object_id = false;

	// 	ofmsg("clearing %1%", %hg->getName());
	// 	hg->clear();

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

	// TODO: improve this using HAPI_AttributeTypeInfo:
	// HAPI_ATTRIBUTE_TYPE_INVALID 	
	// HAPI_ATTRIBUTE_TYPE_NONE 	
	// HAPI_ATTRIBUTE_TYPE_POINT 	
	// HAPI_ATTRIBUTE_TYPE_HPOINT 	
	// HAPI_ATTRIBUTE_TYPE_VECTOR 	
	// HAPI_ATTRIBUTE_TYPE_NORMAL 	
	// HAPI_ATTRIBUTE_TYPE_COLOR 	
	// HAPI_ATTRIBUTE_TYPE_QUATERNION 	
	// HAPI_ATTRIBUTE_TYPE_MATRIX3 	
	// HAPI_ATTRIBUTE_TYPE_MATRIX 	
	// HAPI_ATTRIBUTE_TYPE_ST 	
	// HAPI_ATTRIBUTE_TYPE_HIDDEN 	
	// HAPI_ATTRIBUTE_TYPE_BOX2 	
	// HAPI_ATTRIBUTE_TYPE_BOX 	
	// HAPI_ATTRIBUTE_TYPE_TEXTURE 	
	// HAPI_ATTRIBUTE_TYPE_MAX

    vector<std::string> point_attrib_names = part.attribNames(
	HAPI_ATTROWNER_POINT);

	hlog("[HoudiniEngine::process_part]     Attributes");
	ologaddnewline(false);

	hlog("[HoudiniEngine::process_part]     Point:  ");

    for (int attrib_index=0; attrib_index < int(point_attrib_names.size());
	    ++attrib_index) {

		hflog("%1% ", %point_attrib_names[attrib_index]);

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
		if (point_attrib_names[attrib_index] == "Alpha") {
			has_point_alphas = true;
		    process_attrib(part, HAPI_ATTROWNER_POINT, "Alpha", alphas);
		}
		if (point_attrib_names[attrib_index] == "uv") {
			has_point_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "uv", uvs);
		}

		// TODO: add support for automatic camera-facing of elements
		// need to effectively unpack the geometry to parts again
		// then make each one auto look-at camera.. refer to daPly/vertexData.cpp
		// for a way to do it in osg
		// otherwise, look at packed primitives in houdini engine
		if (point_attrib_names[attrib_index] == "pivotpoint_x") {
			has_pivotpoint_x = true;
		    process_attrib(part, HAPI_ATTROWNER_POINT, "pivotpoint_x", ppx);
			hflog("ppx %1%", %ppx[0]);
		}

		if (point_attrib_names[attrib_index] == "pivotpoint_y") {
			has_pivotpoint_y = true;
		    process_attrib(part, HAPI_ATTROWNER_POINT, "pivotpoint_y", ppy);
			hflog("ppy %1%", %ppy[0]);
		}
		if (point_attrib_names[attrib_index] == "pivotpoint_z") {
			has_pivotpoint_z = true;
		    process_attrib(part, HAPI_ATTROWNER_POINT, "pivotpoint_z", ppz);
			hflog("ppz %1%", %ppz[0]);
		}
		// Object_ids are consecutive, and should really be a primitive attribute
		if (point_attrib_names[attrib_index] == "object_id") {
			has_object_id = true;
			hlog("this object has object_ids");
		    process_attrib(part, HAPI_ATTROWNER_POINT, "object_id", object_ids);
			hflog("object_id %1%", %object_ids[0]);
		}
	}

	hlog("\n");

    vector<std::string> vertex_attrib_names = part.attribNames(
	HAPI_ATTROWNER_VERTEX);

	hlog("[HoudiniEngine::process_part]     Vert:   ");

	for (int attrib_index=0; attrib_index < int(vertex_attrib_names.size());
	    ++attrib_index) {

		hflog("%1% ", %vertex_attrib_names[attrib_index]);

		if (vertex_attrib_names[attrib_index] == "N") {
			has_vertex_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "N", normals);
		}
		if (vertex_attrib_names[attrib_index] == "uv") {
			has_vertex_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "uv", uvs);
		}
	}

 	hlog("\n");

   vector<std::string> primitive_attrib_names = part.attribNames(
	HAPI_ATTROWNER_PRIM);

	hlog("[HoudiniEngine::process_part]     Prim:   ");

	for (int attrib_index=0; attrib_index < int(primitive_attrib_names.size());
	    ++attrib_index) {

		hflog("%1% ", %primitive_attrib_names[attrib_index]);

		if (primitive_attrib_names[attrib_index] == "Cd") {
			has_primitive_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_PRIM, "Cd", colors);
		}
	}

	hlog("\n");

    vector<std::string> detail_attrib_names = part.attribNames(
	HAPI_ATTROWNER_DETAIL);

	hlog("[HoudiniEngine::process_part]     Detail: ");

	for (int attrib_index=0; attrib_index < int(detail_attrib_names.size());
	    ++attrib_index) {

		hflog("%1% ", %detail_attrib_names[attrib_index]);

	}

	hlog("\n");
	ologaddnewline(true);

	// construct primitive sets based on type of part
	// HAPI_PARTTYPE_INVALID
	// HAPI_PARTTYPE_MESH
	// HAPI_PARTTYPE_CURVE
	// HAPI_PARTTYPE_VOLUME
	// HAPI_PARTTYPE_INSTANCER
	// HAPI_PARTTYPE_BOX
	// HAPI_PARTTYPE_SPHERE
	// HAPI_PARTTYPE_MAX

	if (part.info().type == HAPI_PARTTYPE_INSTANCER) {
		hlog("[HoudiniEngine::process_part]     Instancer (TODO)");
	}

	// TODO: render curves
	if (part.info().type == HAPI_PARTTYPE_CURVE) {
 		hlog("[HoudiniEngine::process_part]     Curve: (TODO)");
#if 0
 		ofmsg ("process_asset:     Curve: %1% %2%", %part.name() %part.id);

		HAPI_CurveInfo curve_info;
		ENSURE_SUCCESS(session, HAPI_GetCurveInfo(
			session,
			part.geo.info().nodeId,
	        part.id,
			&curve_info
		) );

		if ( curve_info.curveType == HAPI_CURVETYPE_LINEAR )
			omsg("curve mesh type = Linear");
		else if ( curve_info.curveType == HAPI_CURVETYPE_BEZIER )
			omsg("curve mesh type = Bezier");
		else if ( curve_info.curveType == HAPI_CURVETYPE_NURBS )
			omsg("curve mesh type = Nurbs");
		else
			omsg("curve mesh type = Unknown");

		ofmsg("curve count: %1%", %curve_info.curveCount);
		int vertex_offset = 0;
		int knot_offset = 0;
		int segments = 20;

		for ( int i = 0; i < curve_info.curveCount; i++ ) {
			ofmsg("curve %1% of %2%", %(i + 1) %curve_info.curveCount);
			// Number of CVs
			int num_vertices;
			HAPI_GetCurveCounts(
				session,
				part.geo.info().nodeId,
		        part.id,
				&num_vertices,
				i,
				1
			);
			ofmsg("num vertices: %1%", %num_vertices);
			// Order of this particular curve
			int order;
			if ( curve_info.order != HAPI_CURVE_ORDER_VARYING
				&& curve_info.order != HAPI_CURVE_ORDER_INVALID )
				order = curve_info.order;
			else
				HAPI_GetCurveOrders(
					session,
					part.geo.info().nodeId,
			        part.id,
					&order,
					i,
					1
				);
			ofmsg("curve order: %1%", %order);
			// If there's not enough vertices, then don't try to
			// create the curve.
			if ( num_vertices < order )
			{
				ofmsg("not enought vertices on curve %1% of %2%: skipping",
					%i
					%curve_info.curveCount);
				// The curve at i will have numVertices vertices, and may have
				// some knots. The knot count will be numVertices + order for
				// nurbs curves.
				vertex_offset += num_vertices * 4;
				knot_offset += num_vertices + order;
				continue;
			}

			ofmsg("points size: %1%", %points.size());
			ofmsg("vert size: %1%", %num_vertices);

			// draw the curve
			// TODO: add bezier function, segmentalise curves
			hg->addPrimitiveOsg(
				osg::PrimitiveSet::LINE_STRIP,
				vertex_offset * segments,
				(vertex_offset + num_vertices) * segments - 1,
				partIndex,
				geoIndex,
				objIndex
			);

			for (int j = 0; j < num_vertices - 1; ++j) {
				for (int seg = 0; seg < segments; ++seg) {

					Vector3f p = bez(
						(float) seg / segments,
						points[vertex_offset + j],
					    points[vertex_offset + j + 1]
					);

					hg->addVertex(p, partIndex, geoIndex, objIndex);
					ofmsg("%1%", %p);
					if(has_point_colors) {
						hg->addColor(Color(
							colors[vertex_offset + j][0],
							colors[vertex_offset + j][1],
							colors[vertex_offset + j][2],
							1.0
						), partIndex, geoIndex, objIndex);
					} else if (has_primitive_colors) {
						hg->addColor(Color(
							colors[i][0],
							colors[i][1],
							colors[i][2],
							1.0
						), partIndex, geoIndex, objIndex);
					}
				}
			}

			vertex_offset += num_vertices;

			omsg("done this curve");

			// if (curve_info.hasKnots) {
			// 	std::vector< float > knots;
			// 	knots.resize( num_vertices + order );
			// 	HAPI_GetCurveKnots(
			// 		session,
			// 		part.geo.info().nodeId,
			// 		part.geo.object.id,
			// 		part.geo.id,
			//         part.id,
			// 		&knots.front(),
			// 		knot_offset,
			// 		num_vertices + order

			// 	);
			// 	for( int j = 0; j < num_vertices + order; j++ ) {
			// 		cout << "knot " << j << ": " << knots[ j ] << endl;
			// 	}
			// }

		}
		hg->dirty();
		return;
#endif
	}

	if (part.info().type == HAPI_PARTTYPE_MESH) {
		hflog("[HoudiniEngine::process_part]      Mesh (faceCount: %1% pointCount: %2% vertCount: %3%)", 
			%part.info().faceCount
			%part.info().pointCount
			%part.info().vertexCount
		);

		// no faces..
		if (part.info().faceCount == 0) {
			// but has points, so draw them?
			if (part.info().pointCount > 0) {
				// TODO: is there a better way to do this?
				foreach(Vector3f point, points) {
					hg->addVertex(point, partIndex, geoIndex, objIndex);
				}
				osg::PrimitiveSet::Mode myType = osg::PrimitiveSet::POINTS;
				hg->addPrimitiveOsg(myType, 0, part.info().pointCount - 1, partIndex, geoIndex, objIndex);

				// TODO: add a point sprite shader?
			}
			
			hg->dirty();
			return;
		}

		std::vector< int > face_counts ( part.info().faceCount );
		ENSURE_SUCCESS(session,  HAPI_GetFaceCounts(
			session,
			part.geo.info().nodeId,
			part.id,
			face_counts.data(),
			0,
			part.info().faceCount
		) );

		std::vector< int > vertex_list ( part.info().vertexCount );
		ENSURE_SUCCESS(session,  HAPI_GetVertexList(
			session,
			part.geo.info().nodeId,
			part.id,
			vertex_list.data(), 
			0, 
			part.info().vertexCount ) );
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

				if (prev_faceCount == 1) {
					myType = osg::PrimitiveSet::POINTS;
				} else if (prev_faceCount == 3) {
					myType = osg::PrimitiveSet::TRIANGLES;
				} else if (prev_faceCount == 4) {
					myType = osg::PrimitiveSet::QUADS;
				}

				// cout << "making primitive set for " << prev_faceCount << ", from " <<
				// 	prev_faceCountIndex << " plus " <<
				// 	curr_index - prev_faceCountIndex << endl;

				hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

				prev_faceCountIndex = curr_index;
				prev_faceCount = face_counts[ii];
			} else if ((ii > 0) && (prev_faceCount > 4)) {
				// cout << "making primitive set for " << prev_faceCount << ", plus " <<
				// 	prev_faceCountIndex << " to " <<
				// 	curr_index - prev_faceCountIndex << endl;
				hg->addPrimitiveOsg(osg::PrimitiveSet::TRIANGLE_FAN,
					prev_faceCountIndex,
					curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

				prev_faceCountIndex = curr_index;
				prev_faceCount = face_counts[ii];
			}

			// cout << "face (" << face_counts[ii] << "): " << ii << " ";
			for( int jj=0; jj < face_counts[ii]; jj++ )
			{

				int myIndex = curr_index + (face_counts[ii] - jj) % face_counts[ii];
				// cout << "i: " << vertex_list[myIndex] << " ";

				int lastIndex = hg->addVertex(points[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);

				if (has_point_normals) {
					hg->addNormal(normals[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
				} else if (has_vertex_normals) {
					hg->addNormal(normals[myIndex], partIndex, geoIndex, objIndex);
				}
				if(has_point_colors) {
					// ofmsg("alpha for %1%: %2% ", %myIndex %(has_point_alphas ? alphas[ myIndex ]: 1.0));
					hg->addColor(Color(
						colors[vertex_list[ myIndex ]][0],
						colors[vertex_list[ myIndex ]][1],
						colors[vertex_list[ myIndex ]][2],
						has_point_alphas ? alphas[vertex_list[ myIndex ]]: 1.0
					), partIndex, geoIndex, objIndex);
				} else if (has_primitive_colors) {
					hg->addColor(Color(
						colors[ii][0],
						colors[ii][1],
						colors[ii][2],
						has_point_alphas ? alphas[ myIndex ]: 1.0
					), partIndex, geoIndex, objIndex);
				}
				if (has_point_uvs) {
					hg->addUV(uvs[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
					// cout << "(p)uvs: " << uvs[vertex_list[ myIndex ]][0] << ", " << uvs[vertex_list[ myIndex ]][1] << endl;
				} else if (has_vertex_uvs) {
					// hg->addUV(uvs[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
					hg->addUV(uvs[myIndex], partIndex, geoIndex, objIndex);
					// cout << "(v)uvs: " << uvs[myIndex][0] << ", " << uvs[myIndex][1] << endl;
				}

				// cout << "v:" << myIndex << ", i: "
				// 	<< hg->getVertex(lastIndex) << endl; //" ";
			}

			curr_index += face_counts[ii];

		}

		if (prev_faceCount == 3) {
			myType = osg::PrimitiveSet::TRIANGLES;
		} else if (prev_faceCount == 4) {
			myType = osg::PrimitiveSet::QUADS;
		}
		if (prev_faceCount > 4) {
			myType = osg::PrimitiveSet::TRIANGLE_FAN;
		}

		hflog("[HoudiniEngine::process_part]    make primitive set face size %1%, from %2% plus %3%",
			%prev_faceCount
			%prev_faceCountIndex
			%(curr_index - prev_faceCountIndex)
		);

		hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

		// transparency override
		osg::StateSet* ss =  hg->getOsgNode(geoIndex, objIndex)->getDrawable(partIndex)->getOrCreateStateSet();
		hg->setTransparent(has_point_alphas, partIndex, geoIndex, objIndex);

		// Material handling
		process_materials(part, hg);

		// set transparency state set on this part if there are any alphas
		if (has_point_alphas) {
			hflog("[HoudiniEngine::process_part]    setting part %1% as transparent", %partIndex);
			ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
			ss->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | 
			osg::StateAttribute::OVERRIDE);
		} else {
			hflog("[HoudiniEngine::process_part]    setting part %1% as opaque", %partIndex);
			ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
			ss->setMode(GL_BLEND, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED | 
			osg::StateAttribute::OVERRIDE);
		}

		hg->dirty();

	}

	if (part.info().type == HAPI_PARTTYPE_VOLUME) {
		hlog("[HoudiniEngine::process_part]     Volume (TODO)");
	}

	// todo: check out some geometry shaders for this..
	if (part.info().type == HAPI_PARTTYPE_BOX) {
		hlog("[HoudiniEngine::process_part]     Box (TODO)");
	}
	// todo: check out some geometry shaders for this..
	if (part.info().type == HAPI_PARTTYPE_SPHERE) {
		hlog("[HoudiniEngine::process_part]     Sphere (TODO)");
	}

}

// get the part id from the Part, 
// use it to make a stateset for the relevant Geode in the HG
// set the various parms according to their names:
//     eg: diff -> Material.setDiffuseColour()
//     or if a uniform, add uniform to the stateset
//     shader should end up propogating changes
void HoudiniEngine::process_materials(const hapi::Part &part, HoudiniGeometry* hg) {
	bool all_same = false;

	int faceCount = part.info().faceCount;

	std::vector< HAPI_NodeId > mat_ids_on_faces = part.materialNodeIdsOnFaces(all_same);

	std::vector< int > mat_ids;

	hflog("[HoudiniEngine::process_materials]   all same? %1%", %(all_same ? "Yes" : "No"));

	// collect all material node ids
	if (all_same) {
		mat_ids.push_back(mat_ids_on_faces[0]);
	} else {
		for (vector<int>::iterator it = mat_ids_on_faces.begin(); it < mat_ids_on_faces.end(); ++it) {
			if (std::find(mat_ids.begin(), mat_ids.end(), *it) == mat_ids.end()) {
				mat_ids.push_back(*it);
			}
		}
	}

	hflog("[HoudiniEngine::process_materials]   unique mat ids: %1%", %mat_ids.size());
	for (int i =0; i < mat_ids.size(); ++i) {
		hflog("[HoudiniEngine::process_materials]   material %1% %2%", %i %mat_ids[i]);
	}

	// don't clear..
	// assetMaterialParms[part.geo.object.asset.name()].clear();

	for (int i = 0; i < mat_ids.size(); ++i) {

		HAPI_MaterialInfo mat_info;

		ENSURE_SUCCESS(session,  HAPI_GetMaterialInfo (
			session,
			mat_ids[i],
			&mat_info));

		// currently, sharing only what I will be using
		// TODO: share it all, then implement what I will use
		if (mat_info.exists) {
			hflog("[HoudiniEngine::process_materials]   Material info for %1%: nodeId: %2% hasChanged: %3%",
				%hg->getName() %mat_info.nodeId %mat_info.hasChanged
			);

			hapi::Node matNode(mat_info.nodeId, session);

			MatStruct* ms = NULL;

			for (int j = 0; j < assetMaterialParms[hg->getName()].size(); ++j) {
				if (assetMaterialParms[hg->getName()][j].matId == mat_info.nodeId) {
					ms = &assetMaterialParms[hg->getName()][j];
					break;
				}
			}

			if (ms == NULL) {
				ms = new MatStruct();
			}

			ms->matId = mat_info.nodeId;
			ms->partId = part.id;
			ms->geoId = part.geo.id;
			ms->objId = part.geo.object.id;

			hflog("[HoudiniEngine::process_materials]   set matId of %1% on part %2%, geo %3% object %4% of asset %5%",
				%mat_info.nodeId
				%part.id
				%part.geo.id
				%part.geo.object.id
				%hg->getName()
			);
			hg->setMatId(mat_info.nodeId, part.id, part.geo.id, part.geo.object.id);

			std::map<std::string, hapi::Parm> parmMap = matNode.parmMap();

			int diffuseMapParmId = -1;
			int normalMapParmId = -1;

			string diffuseMapName;
			string normalMapName;
			hlog("[HoudiniEngine::process_materials]   looking for diffuse map");
			if (parmMap.count("ogl_tex1")) {
				// only assign if not empty
				if (parmMap["ogl_tex1"].getStringValue(0) != "") {
					diffuseMapParmId = parmMap["ogl_tex1"].info().id;
					diffuseMapName = parmMap["ogl_tex1"].name();
					hflog("[HoudiniEngine::process_materials]   map found, value is %1%", %parmMap["ogl_tex1"].getStringValue(0));
				}
			} else if (parmMap.count("baseColorMap")) {
				// only assign if not empty
				if (parmMap["baseColorMap"].getStringValue(0) != "") {
					diffuseMapParmId = parmMap["baseColorMap"].info().id;
					diffuseMapName = parmMap["baseColorMap"].name();
					hflog("[HoudiniEngine::process_materials]   map found, value is %1%", %parmMap["baseColorMap"].getStringValue(0));
				}
			} else if (parmMap.count("map")) {
				// only assign if not empty
				if (parmMap["map"].getStringValue(0) != "") {
					diffuseMapParmId = parmMap["map"].info().id;
					diffuseMapName = parmMap["map"].name();
					hflog("[HoudiniEngine::process_materials]   map found, value is %1%", %parmMap["map"].getStringValue(0));
				}
			}

			hlog("[HoudiniEngine::process_materials]   looking for normal map");
			if (parmMap.count("ogl_normalmap")) {
				// only assign if not empty
				if (parmMap["ogl_normalmap"].getStringValue(0) != "") {
					normalMapParmId = parmMap["ogl_normalmap"].info().id;
					normalMapName = parmMap["ogl_normalmap"].name();
					hflog("[HoudiniEngine::process_materials]   map found, value is %1%", %parmMap["ogl_normalmap"].getStringValue(0));
				}
			}

			// hlog("[HoudiniEngine::process_materials]   looking for ambient colour");
			// if (parmMap.count("ogl_amb")) {
			// 	hflog("[HoudiniEngine::process_materials]   has ambient, value is %1%,%2%,%3%", 
			// 		%parmMap["ogl_amb"].getFloatValue(0)
			// 		%parmMap["ogl_amb"].getFloatValue(1)
			// 		%parmMap["ogl_amb"].getFloatValue(2)
			// 	);
			// 	ParmStruct ps;
			// 	ps.type = parmMap["ogl_amb"].info().type;
			// 	ps.floatValues.push_back(parmMap["ogl_amb"].getFloatValue(0));
			// 	ps.floatValues.push_back(parmMap["ogl_amb"].getFloatValue(1));
			// 	ps.floatValues.push_back(parmMap["ogl_amb"].getFloatValue(2));
			// 	ms->parms["ogl_amb"] = ps;

			// 	// update the state set for this attribute
			// 	if (assetInstances.count(hg->getName()) > 0) {
			// 		osg::StateSet* ss =  hg->getOsgNode(part.geo.id, part.geo.object.id)->getDrawable(part.id)->getOrCreateStateSet();
			// 		Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
			// 		if (mat == NULL) {
			// 			mat = new osg::Material();
			// 		}
			// 		mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(
			// 			ps.floatValues[0],
			// 			ps.floatValues[1],
			// 			ps.floatValues[2],
			// 			1.0
			// 		));
			// 		ss->setAttributeAndModes(mat, 
			// 			osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | 
			// 			osg::StateAttribute::OVERRIDE);
			// 	}
			// }

			hlog("[HoudiniEngine::process_materials]   looking for emission colour");
			if (parmMap.count("ogl_emit")) {
				hflog("[HoudiniEngine::process_materials]   has emission, value is %1%,%2%,%3%", 
					%parmMap["ogl_emit"].getFloatValue(0)
					%parmMap["ogl_emit"].getFloatValue(1)
					%parmMap["ogl_emit"].getFloatValue(2)
				);
				ParmStruct ps;
				ps.type = parmMap["ogl_emit"].info().type;
				ps.floatValues.push_back(parmMap["ogl_emit"].getFloatValue(0));
				ps.floatValues.push_back(parmMap["ogl_emit"].getFloatValue(1));
				ps.floatValues.push_back(parmMap["ogl_emit"].getFloatValue(2));
				ms->parms["ogl_emit"] = ps;

				// update the state set for this attribute
				if (assetInstances.count(hg->getName()) > 0) {
					osg::StateSet* ss =  hg->getOsgNode(part.geo.id, part.geo.object.id)->getDrawable(part.id)->getOrCreateStateSet();
					Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
					if (mat == NULL) {
						mat = new osg::Material();
					}
					mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(
						ps.floatValues[0],
						ps.floatValues[1],
						ps.floatValues[2],
						1.0
					));
					ss->setAttributeAndModes(mat, 
						osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | 
						osg::StateAttribute::OVERRIDE);
				}
			}

			hlog("[HoudiniEngine::process_materials]   looking for diffuse colour");
			if (parmMap.count("ogl_diff")) {
				hflog("[HoudiniEngine::process_materials]   has diffuse, value is %1%,%2%,%3%", 
					%parmMap["ogl_diff"].getFloatValue(0)
					%parmMap["ogl_diff"].getFloatValue(1)
					%parmMap["ogl_diff"].getFloatValue(2)
				);
				ParmStruct ps;
				ps.type = parmMap["ogl_diff"].info().type;
				ps.floatValues.push_back(parmMap["ogl_diff"].getFloatValue(0));
				ps.floatValues.push_back(parmMap["ogl_diff"].getFloatValue(1));
				ps.floatValues.push_back(parmMap["ogl_diff"].getFloatValue(2));
				ms->parms["ogl_diff"] = ps;

				// update the state set for this attribute
				if (assetInstances.count(hg->getName()) > 0) {
					osg::StateSet* ss =  hg->getOsgNode(part.geo.id, part.geo.object.id)->getDrawable(part.id)->getOrCreateStateSet();
					Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
					if (mat == NULL) {
						mat = new osg::Material();
					}
					mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(
						ps.floatValues[0],
						ps.floatValues[1],
						ps.floatValues[2],
						1.0
					));
					ss->setAttributeAndModes(mat, 
						osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | 
						osg::StateAttribute::OVERRIDE);
				}
			}

			// hlog("[HoudiniEngine::process_materials]   looking for specular colour");
			// if (parmMap.count("ogl_spec")) {
			// 	ofmsg("has specular, value is %1%,%2%,%3%", 
			// 		%parmMap["ogl_spec"].getFloatValue(0)
			// 		%parmMap["ogl_spec"].getFloatValue(1)
			// 		%parmMap["ogl_spec"].getFloatValue(2)
			// 	);
			// 	ParmStruct ps;
			// 	ps.type = parmMap["ogl_spec"].info().type;
			// 	ps.floatValues.push_back(parmMap["ogl_spec"].getFloatValue(0));
			// 	ps.floatValues.push_back(parmMap["ogl_spec"].getFloatValue(1));
			// 	ps.floatValues.push_back(parmMap["ogl_spec"].getFloatValue(2));
			// 	ms->parms["ogl_spec"] = ps;

			// 	// update the state set for this attribute
			// 	if (assetInstances.count(hg->getName()) > 0) {
			// 		osg::StateSet* ss =  hg->getOsgNode(part.geo.id, part.geo.object.id)->getDrawable(part.id)->getOrCreateStateSet();
			// 		Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
			// 		if (mat == NULL) {
			// 			mat = new osg::Material();
			// 		}
			// 		mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(
			// 			ps.floatValues[0],
			// 			ps.floatValues[1],
			// 			ps.floatValues[2],
			// 			1.0
			// 		));
			// 		ss->setAttributeAndModes(mat, 
			// 			osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | 
			// 			osg::StateAttribute::OVERRIDE);
			// 	}
			// }

			hlog("[HoudiniEngine::process_materials]   looking for alpha material colour");
			if (parmMap.count("ogl_alpha")) {
				hflog("[HoudiniEngine::process_materials]   has alpha, value is %1%", %parmMap["ogl_alpha"].getFloatValue(0));
				ParmStruct ps;
				ps.type = parmMap["ogl_alpha"].info().type;
				ps.floatValues.push_back(parmMap["ogl_alpha"].getFloatValue(0));
				ms->parms["ogl_apha"] = ps;

				const string name = "unif_alpha";
				// update the state set for this attribute
				if (assetInstances.count(hg->getName()) > 0) {
					osg::StateSet* ss =  hg->getOsgNode(part.geo.id, part.geo.object.id)->getDrawable(part.id)->getOrCreateStateSet();
					osg::Uniform* u =  ss->getOrCreateUniform(name, osg::Uniform::Type::FLOAT, 1);
					u->set(ps.floatValues[0]);
					ss->getUniformList()[name].second = osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | 
						osg::StateAttribute::OVERRIDE;

					// set as transparent
					if (ps.floatValues[0] < 0.95) {
						ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
						ss->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | 
						osg::StateAttribute::OVERRIDE);
					} else {
						ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
						ss->setMode(GL_BLEND, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED | 
						osg::StateAttribute::OVERRIDE);
					}
				}
			}

			hlog("[HoudiniEngine::process_materials]   looking for shininess");
			if (parmMap.count("ogl_rough")) {
				hflog("[HoudiniEngine::process_materials]   has shininess, value is %1%", %parmMap["ogl_rough"].getFloatValue(0));
				ParmStruct ps;
				ps.type = parmMap["ogl_rough"].info().type;
				ps.floatValues.push_back(parmMap["ogl_rough"].getFloatValue(0));
				ms->parms["ogl_rough"] = ps;
			}

			if (diffuseMapParmId >= 0) {

				hlog("[HoudiniEngine::process_materials]   diffuse map found..");
				// NOTE this works if the image is a png
				ENSURE_SUCCESS(session,  HAPI_RenderTextureToImage(
					session,
					mat_info.nodeId,
					diffuseMapParmId));


				HAPI_ImageInfo image_info;
				ENSURE_SUCCESS(session,  HAPI_GetImageInfo(
					session,
					mat_info.nodeId,
					&image_info));

				String packing;
				switch (image_info.packing) {
					case HAPI_IMAGE_PACKING_UNKNOWN:
					packing = "HAPI_IMAGE_PACKING_UNKNOWN";
					break;
					case HAPI_IMAGE_PACKING_SINGLE:
					packing = "HAPI_IMAGE_PACKING_SINGLE";
					break;
					case HAPI_IMAGE_PACKING_DUAL:
					packing = "HAPI_IMAGE_PACKING_DUAL";
					break;
					case HAPI_IMAGE_PACKING_RGB:
					packing = "HAPI_IMAGE_PACKING_RGB";
					break;
					case HAPI_IMAGE_PACKING_BGR:
					packing = "HAPI_IMAGE_PACKING_BGR";
					break;
					case HAPI_IMAGE_PACKING_RGBA:
					packing = "HAPI_IMAGE_PACKING_RGBA";
					break;
					case HAPI_IMAGE_PACKING_ABGR:
					packing = "HAPI_IMAGE_PACKING_ABGR";
					break;
					case HAPI_IMAGE_PACKING_MAX:
					packing = "HAPI_IMAGE_PACKING_MAX";
					break;
					default:
					break;
				}

				// set things
				// As of HE3.1 for Houdini 16.5.405, setImageInfo does nothing, so lets ignore
				// image_info.packing = HAPI_IMAGE_PACKING_RGBA;
				// image_info.dataFormat = HAPI_IMAGE_DATA_INT8;
				// image_info.interleaved = true; // RGBRGBRGB..


				// ENSURE_SUCCESS(session,  HAPI_SetImageInfo(
				// 	session,
				// 	mat_info.nodeId,
				// 	&image_info));


				hflog("[HoudiniEngine::process_materials]   width %1% height: %2% format: %3% dataFormat: %4% packing %5% interleaved %6% gamma %7%",
					%image_info.xRes
					%image_info.yRes
					%get_string(session, image_info.imageFileFormatNameSH)
					%image_info.dataFormat
					%packing
					%image_info.interleaved
					%image_info.gamma
				);

				// ---------

				HAPI_StringHandle imageSH;

				int planeCount = 0;

				ENSURE_SUCCESS(session,  HAPI_GetImagePlaneCount(
					session,
					mat_info.nodeId,
					&planeCount
				));

				hflog("[HoudiniEngine::process_materials]   image plane count: %1%", %planeCount);

				ENSURE_SUCCESS(session,  HAPI_GetImagePlanes(
					session,
					mat_info.nodeId,
					&imageSH,
					planeCount
				));

				int imgBufSize = -1;

				// get image planes into a buffer as RAW, so no mistakes
				// Could eventually optimise this..
				ENSURE_SUCCESS(session,  HAPI_ExtractImageToMemory(
					session,
					mat_info.nodeId,
					HAPI_RAW_FORMAT_NAME,
					"C A", /* image planes */
					&imgBufSize
				));

				char *myBuffer = new char[imgBufSize];

				// put into a buffer
				ENSURE_SUCCESS(session,  HAPI_GetImageMemoryBuffer(
					session,
					mat_info.nodeId,
					myBuffer,
					imgBufSize
				));

				Ref<PixelData> pd = PixelData::create(image_info.xRes, image_info.yRes, PixelData::FormatRgba);

				// manually set each pixel. Slow, but definitely correct
				// would be better to dump all pixel data
				pd->beginPixelAccess();
				for (int ii = 0; ii < image_info.xRes; ++ii) {
					for (int jj = 0; jj < image_info.yRes; ++jj) {
						int offset = 4 * ((image_info.yRes * ii) + jj);
						int r = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 0]));
						int g = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 1]));
						int b = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 2]));
						int a = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 3]));

						// y and x are swapped, account for it accordingly
						pd->setPixel(jj, ii,
							r,
							g,
							b,
							a
						);
					}
				}
				pd->endPixelAccess();
				pd->setDirty(true);

				// testing whether it's read properly
				// ImageUtils::saveImage("/tmp/spaceship1.jpg", pd, ImageUtils::FormatJpeg);

				// load into a pixelData bufferObject
				// this works!
				// Ref<PixelData> refPd = ImageUtils::decode((void *) myBuffer, image_info.xRes * image_info.yRes * 4);
				// pds.push_back(ImageUtils::decode((void *) myBuffer, image_info.xRes * image_info.yRes * 4));
				// pds.push_back(myBuffer);
				// pds.push_back(pd);

				// TODO: general case for texture names (diffuse, spec, env, etc)
				// osg::Texture2D* texture = mySceneManager->createTexture(diffuseMapName, pds[pds.size() - 1]);
				osg::Texture2D* texture = mySceneManager->createTexture(diffuseMapName, pd);
				ParmStruct ps;
				ps.type = parmMap["diffuseMapName"].info().type;
				ps.stringValues.push_back(parmMap["diffuseMapName"].getStringValue(0));
				ms->parms["diffuseMapName"] = ps;


				// need to set wrap modes too
				osg::Texture::WrapMode textureWrapMode;
				textureWrapMode = osg::Texture::REPEAT;

				texture->setWrap(osg::Texture2D::WRAP_R, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_S, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_T, textureWrapMode);

				// Update materials on each instance of this asset
				if (assetInstances.count(hg->getName()) > 0) {
					// update diffuse map texture
					hflog("[HoudiniEngine::process_materials]   updating %1%'s texture %2%", %hg->getName() %diffuseMapName);
					assetInstances[hg->getName()]->getMaterial()->setDiffuseTexture(diffuseMapName);
				}
			}

			if (normalMapParmId >= 0) {

				hlog("[HoudiniEngine::process_materials]   normal map found..");
				// NOTE this works if the image is a png
				ENSURE_SUCCESS(session,  HAPI_RenderTextureToImage(
					session,
					mat_info.nodeId,
					normalMapParmId));


				HAPI_ImageInfo image_info;
				ENSURE_SUCCESS(session,  HAPI_GetImageInfo(
					session,
					mat_info.nodeId,
					&image_info));

				String packing;
				switch (image_info.packing) {
					case HAPI_IMAGE_PACKING_UNKNOWN:
					packing = "HAPI_IMAGE_PACKING_UNKNOWN";
					break;
					case HAPI_IMAGE_PACKING_SINGLE:
					packing = "HAPI_IMAGE_PACKING_SINGLE";
					break;
					case HAPI_IMAGE_PACKING_DUAL:
					packing = "HAPI_IMAGE_PACKING_DUAL";
					break;
					case HAPI_IMAGE_PACKING_RGB:
					packing = "HAPI_IMAGE_PACKING_RGB";
					break;
					case HAPI_IMAGE_PACKING_BGR:
					packing = "HAPI_IMAGE_PACKING_BGR";
					break;
					case HAPI_IMAGE_PACKING_RGBA:
					packing = "HAPI_IMAGE_PACKING_RGBA";
					break;
					case HAPI_IMAGE_PACKING_ABGR:
					packing = "HAPI_IMAGE_PACKING_ABGR";
					break;
					case HAPI_IMAGE_PACKING_MAX:
					packing = "HAPI_IMAGE_PACKING_MAX";
					break;
					default:
					break;
				}

				// set things
				// As of HE3.1 for Houdini 16.5.405, setImageInfo does nothing, so lets ignore
				// image_info.packing = HAPI_IMAGE_PACKING_RGBA;
				// image_info.dataFormat = HAPI_IMAGE_DATA_INT8;
				// image_info.interleaved = true; // RGBRGBRGB..


				// ENSURE_SUCCESS(session,  HAPI_SetImageInfo(
				// 	session,
				// 	mat_info.nodeId,
				// 	&image_info));


				hflog("[HoudiniEngine::process_materials]   width %1% height: %2% format: %3% dataFormat: %4% packing %5% interleaved %6% gamma %7%",
					%image_info.xRes
					%image_info.yRes
					%get_string(session, image_info.imageFileFormatNameSH)
					%image_info.dataFormat
					%packing
					%image_info.interleaved
					%image_info.gamma
				);

				// ---------

				HAPI_StringHandle imageSH;

				int planeCount = 0;

				ENSURE_SUCCESS(session,  HAPI_GetImagePlaneCount(
					session,
					mat_info.nodeId,
					&planeCount
				));

				hflog("[HoudiniEngine::process_materials]   image plane count: %1%", %planeCount);

				ENSURE_SUCCESS(session,  HAPI_GetImagePlanes(
					session,
					mat_info.nodeId,
					&imageSH,
					planeCount
				));

				int imgBufSize = -1;

				// get image planes into a buffer as RAW, so no mistakes
				// Could eventually optimise this..
				ENSURE_SUCCESS(session,  HAPI_ExtractImageToMemory(
					session,
					mat_info.nodeId,
					HAPI_RAW_FORMAT_NAME,
					"C A", /* image planes */
					&imgBufSize
				));

				char *myBuffer = new char[imgBufSize];

				// put into a buffer
				ENSURE_SUCCESS(session,  HAPI_GetImageMemoryBuffer(
					session,
					mat_info.nodeId,
					myBuffer,
					imgBufSize
				));

				Ref<PixelData> pd = PixelData::create(image_info.xRes, image_info.yRes, PixelData::FormatRgba);

				// manually set each pixel. Slow, but definitely correct
				// would be better to dump all pixel data
				pd->beginPixelAccess();
				for (int ii = 0; ii < image_info.xRes; ++ii) {
					for (int jj = 0; jj < image_info.yRes; ++jj) {
						int offset = 4 * ((image_info.yRes * ii) + jj);
						int r = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 0]));
						int g = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 1]));
						int b = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 2]));
						int a = static_cast<int>(static_cast<unsigned char>(myBuffer[offset + 3]));

						// y and x are swapped, account for it accordingly
						pd->setPixel(jj, ii,
							r,
							g,
							b,
							a
						);
					}
				}
				pd->endPixelAccess();
				pd->setDirty(true);

				osg::Texture2D* texture = mySceneManager->createTexture(normalMapName, pd);
				ParmStruct ps;
				ps.type = parmMap["normalMapName"].info().type;
				ps.stringValues.push_back(parmMap["normalMapName"].getStringValue(0));
				ms->parms["normalMapName"] = ps;


				// need to set wrap modes too
				osg::Texture::WrapMode textureWrapMode;
				textureWrapMode = osg::Texture::REPEAT;

				texture->setWrap(osg::Texture2D::WRAP_R, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_S, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_T, textureWrapMode);

				// Update materials on each instance of this asset
				if (assetInstances.count(hg->getName()) > 0) {
					// update normal map texture
					hflog("[HoudiniEngine::process_materials]   updating %1%'s normal map to %2%", %hg->getName() %normalMapName);
					assetInstances[hg->getName()]->getMaterial()->setNormalTexture(normalMapName);
				}
			}

			assetMaterialParms[part.geo.object.asset.name()].push_back(*ms);

		} else {
			hflog("[HoudiniEngine::process_materials]   Could not get material %1% for %2%", %i %hg->getName());
		}
	}

}

void HoudiniEngine::process_float_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name, vector<Vector3f>& points)
{
	// useHAPI_AttributeInfo::storage for the data type

    // Get the attribute values.
    HAPI_AttributeInfo attrib_info = part.attribInfo(attrib_owner, attrib_name);
    float *attrib_data = part.getNewFloatAttribData(attrib_info, attrib_name);

// 	cout << attrib_name << " (" << attrib_info.tupleSize << ")" << endl;

	points.clear();
	points.resize(attrib_info.count);
    for (int elem_index=0; elem_index < attrib_info.count; ++elem_index)
    {

		Vector3f v;
		for (int tuple_index=0; tuple_index < attrib_info.tupleSize;
			++tuple_index)
		{
			v[tuple_index] = attrib_data[elem_index * attrib_info.tupleSize + tuple_index ];
		}
		points[elem_index] = v;
// 		cout << elem_index << ": " << v << endl;
    }

    delete [] attrib_data;
}

void HoudiniEngine::process_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name, vector<float>& vals)
{
	// useHAPI_AttributeInfo::storage for the data type

    // Get the attribute values.
    HAPI_AttributeInfo attrib_info = part.attribInfo(attrib_owner, attrib_name);
    float *attrib_data = part.getNewFloatAttribData(attrib_info, attrib_name);

// 	cout << attrib_name << " (" << attrib_info.tupleSize << ")" << endl;

	vals.clear();
	vals.resize(attrib_info.count);
    for (int elem_index=0; elem_index < attrib_info.count; ++elem_index) {
		vals[elem_index] = attrib_data[elem_index];
    }

    delete [] attrib_data;
}

void HoudiniEngine::process_attrib(
    const hapi::Part &part, HAPI_AttributeOwner attrib_owner,
    const char *attrib_name, vector<int>& vals)
{
	// useHAPI_AttributeInfo::storage for the data type

    // Get the attribute values.
    HAPI_AttributeInfo attrib_info = part.attribInfo(attrib_owner, attrib_name);
    int *attrib_data = part.getNewIntAttribData(attrib_info, attrib_name);

// 	cout << attrib_name << " (" << attrib_info.tupleSize << ")" << endl;

	vals.clear();
	vals.resize(attrib_info.count);
    for (int elem_index=0; elem_index < attrib_info.count; ++elem_index) {
		vals[elem_index] = attrib_data[elem_index];
    }

    delete [] attrib_data;
}


// TODO: make an async version of for omegalib
void HoudiniEngine::wait_for_cook()
{
    int status;
    do
    {
 		osleep(50); // sleeping..

		if (myLogEnabled) {
	        int statusBufSize = 0;
	        ENSURE_SUCCESS(session,  HAPI_GetStatusStringBufLength(
				session,
/* 	            HAPI_STATUS_COOK_STATE, HAPI_STATUSVERBOSITY_MESSAGES, */
	            HAPI_STATUS_COOK_STATE, HAPI_STATUSVERBOSITY_ERRORS,
	            &statusBufSize ) );
	        char * statusBuf = NULL;
	        if ( statusBufSize > 0 )
	        {
	            statusBuf = new char[statusBufSize];
	            ENSURE_SUCCESS(session,  HAPI_GetStatusString(
					session,
	                HAPI_STATUS_COOK_STATE, statusBuf, statusBufSize ) );
	        }
	        if ( statusBuf )
	        {
	            std::string result( statusBuf );
	            oflog(Debug, "[HoudiniEngine::wait_for_cook] %1%", %result);
	            delete[] statusBuf;
	        }
		}
		HAPI_GetStatus(session, HAPI_STATUS_COOK_STATE, &status);
	}
	while ( status > HAPI_STATE_MAX_READY_STATE );
	ENSURE_COOK_SUCCESS( session, status );
	hflog("[HoudiniEngine::wait_for_cook] cooked: %1%", %status);
}
