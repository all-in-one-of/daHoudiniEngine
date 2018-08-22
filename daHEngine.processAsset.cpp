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

using namespace houdiniEngine;

// put houdini engine asset data into a houdiniGeometry
void HoudiniEngine::process_assets(const hapi::Asset &asset)
{

	String s = ostr("%1%", %asset.name());

	HoudiniGeometry* hg;

	ofmsg("processing asset %1%", %s);

	if (myHoudiniGeometrys.count(s) > 0) {
		hg = myHoudiniGeometrys[s];
	} else {
		hg = HoudiniGeometry::create(s);
		myHoudiniGeometrys[s] = hg;

	}

	omsg("got a houdiniGeometry, doing stuff with it now..");

    vector<hapi::Object> objects = asset.objects();

	omsg("got objects of asset, getting transforms");

	vector<HAPI_Transform> objTransforms = asset.transforms();

	ofmsg("process_assets: %1%: %2% objects %3% transforms", %asset.name() %objects.size() %objTransforms.size());

	hg->objectsChanged = asset.info().haveObjectsChanged;
	ofmsg("process_assets: %1%: %2% objects %3%", %asset.name() %objects.size() %(hg->objectsChanged == 1 ? "Changed" : ""));
	// forcing true for testing
	// hg->objectsChanged = true;
	// ofmsg("process_assets: %1%: %2% objects Forced Changed", %asset.name() %objects.size() );

	if (hg->getObjectCount() < objects.size()) {
		hg->addObject(objects.size() - hg->getObjectCount());

        for (int i=0; i < objects.size(); i++) {
            hg->setObjectName(i, objects[i].name());
        }
	}

	// if (hg->objectsChanged) {
	// still need to traverse this, as an object may not change, but geos in it can change
	if (true) {
		omsg("process_assets: let's look at objects");
		for (int object_index=0; object_index < int(objects.size()); ++object_index)
	    {
			HAPI_ObjectInfo objInfo = objects[object_index].info();

			// TODO check for instancing, then do things differently
			if (objInfo.isInstancer > 0) {
				ofmsg("instance path: %1%: %2%", %objects[object_index].name() %objects[object_index].objectInstancePath());
				ofmsg("%1%: %2%", %objects[object_index].id %objInfo.objectToInstanceId);
			}
			// don't use vector, as there should be only one geo
			// unless we are exposing editable nodes (not yet)
			vector<hapi::Geo> geos = objects[object_index].geos();

			if (hg->getGeodeCount(object_index) < geos.size()) {
				hg->addGeode(geos.size() - hg->getGeodeCount(object_index), object_index);

                for (int i=0; i < geos.size(); i++) {
                    hg->setGeodeName(i, object_index, geos[i].name());
                }
			}

			hg->setGeosChanged(objInfo.haveGeosChanged, object_index);
			ofmsg("process_assets:   %1%/%2%: %3% %4%",
				%(object_index + 1)
				%geos.size()
				%objects[object_index].name()
				%(hg->getGeosChanged(object_index) == 1 ? "Changed" : "")
			);

			// if (hg->getGeosChanged(object_index)) {
			if (true) {
				omsg("process_assets:   let's look at geos");

				for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
				{
				    vector<hapi::Part> parts = geos[geo_index].parts();

					if (hg->getDrawableCount(geo_index, object_index) < parts.size()) {
						hg->addDrawable(parts.size() - hg->getDrawableCount(geo_index, object_index), geo_index, object_index);
					}

					hg->setGeoChanged(geos[geo_index].info().hasGeoChanged, geo_index, object_index);
					ofmsg("process_assets:     %1%:%2%/%3% %4% %5% %6%",
						%(object_index + 1)
						%(geo_index + 1)
						%parts.size()
						%(hg->getGeoChanged(geo_index, object_index) == 1 ? "Changed" : "")
						%(geos[geo_index].info().isDisplayGeo == 1 ? "Display" : "-")
						%(geos[geo_index].info().isTemplated == 1 ? "Template" : "-")
					);

					hg->clearGeode(geo_index, object_index);
					// if (hg->getGeoChanged(geo_index, object_index)) {
					if (true) {
						ofmsg("process_assets:     let's look at parts, should be %1% of them", %parts.size());
					    for (int part_index=0; part_index < int(parts.size()); ++part_index)
						{
							if (geos[geo_index].info().isDisplayGeo) {
								ofmsg("process_assets:     processing %1% %2%", %s %parts[part_index].name());
								process_geo_part(parts[part_index], object_index, geo_index, part_index, hg);
							}
						}
					}
				}
			}

 			hg->setTransformChanged(objInfo.hasTransformChanged, object_index);
			ofmsg("process_assets:   Transform changed: %2%", %object_index %(hg->getTransformChanged(object_index) == 1 ? "Yes" : "No"));
		}

		for (int object_index=0; object_index < int(objects.size()); ++object_index)
	    {
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

	// do asset matrix transforms
	float asset_matrix[16];
	HAPI_TransformEuler assetTransformEuler;
	asset.getTransformAsMatrix(asset_matrix);
	ENSURE_SUCCESS(session,
		HAPI_ConvertMatrixToEuler(
			session,
			asset_matrix,
			HAPI_RSTORDER_DEFAULT,
			HAPI_XYZORDER_DEFAULT,
			&assetTransformEuler
		)
	);

	// ENSURE_SUCCESS(session,
	// 	HAPI_GetObjectTransform(
	// 		session,
	// 		asset.nodeid,
	// 		-1,
	// 		HAPI_RSTORDER_DEFAULT,
	// 		&assetTransform
	// 	)
	// );

	// should have the matrix here.. check it out.
	ofmsg("process_assets:   matrix pos   %1% %2% %3%", %assetTransformEuler.position[0]
		                              %assetTransformEuler.position[1]
		                              %assetTransformEuler.position[2]);

	ofmsg("process_assets:   matrix rot   %1% %2% %3%", %assetTransformEuler.rotationEuler[0]
		                              %assetTransformEuler.rotationEuler[1]
		                              %assetTransformEuler.rotationEuler[2]);

	ofmsg("process_assets:   matrix scale %1% %2% %3%", %assetTransformEuler.scale[0]
		                              %assetTransformEuler.scale[1]
		                              %assetTransformEuler.scale[2]);

	if (mySceneManager->getModel(s) == NULL) {
		mySceneManager->addModel(hg);
	}
}

// TODO: expand on this..
Vector3f bez(float t, Vector3f a, Vector3f b) {
	return Vector3f(
		a[0] + t *(b[0] - a[0]),
		a[1] + t *(b[1] - a[1]),
		a[2] + t *(b[2] - a[2])
	);
}

// TODO: incrementally update the geometry?
// send a new version, and still have the old version?
void HoudiniEngine::process_geo_part(const hapi::Part &part, const int objIndex, const int geoIndex, const int partIndex, HoudiniGeometry* hg)
{
	omsg("process_geo_part: entered");
	vector<Vector3f> points;
	vector<Vector3f> normals;
	vector<Vector3f> colors;
	vector<float> alphas;

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

	// TODO: improve this..
    vector<std::string> point_attrib_names = part.attribNames(
	HAPI_ATTROWNER_POINT);

	ologaddnewline(false);

	omsg("process_assets:     Point:  ");

    for (int attrib_index=0; attrib_index < int(point_attrib_names.size());
	    ++attrib_index) {

		ofmsg("%1% ", %point_attrib_names[attrib_index]);

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
			// for (int i = 0; i < alphas.size(); ++i) {
			// 	ofmsg("alpha %1%: %2% ", %i %alphas[i]);
			// }
		}
		if (point_attrib_names[attrib_index] == "uv") {
			has_point_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "uv", uvs);
		}
	}

	omsg("\n");

    vector<std::string> vertex_attrib_names = part.attribNames(
	HAPI_ATTROWNER_VERTEX);

	omsg("process_assets:     Vert:   ");

	for (int attrib_index=0; attrib_index < int(vertex_attrib_names.size());
	    ++attrib_index) {

		ofmsg("%1% ", %vertex_attrib_names[attrib_index]);

		if (vertex_attrib_names[attrib_index] == "N") {
			has_vertex_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "N", normals);
		}
		if (vertex_attrib_names[attrib_index] == "uv") {
			has_vertex_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "uv", uvs);
		}
	}

 	omsg("\n");

   vector<std::string> primitive_attrib_names = part.attribNames(
	HAPI_ATTROWNER_PRIM);

	omsg("process_assets:     Prim:   ");

	for (int attrib_index=0; attrib_index < int(primitive_attrib_names.size());
	    ++attrib_index) {

		ofmsg("%1% ", %primitive_attrib_names[attrib_index]);

		if (primitive_attrib_names[attrib_index] == "Cd") {
			has_primitive_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_PRIM, "Cd", colors);
		}
	}

	omsg("\n");

    vector<std::string> detail_attrib_names = part.attribNames(
	HAPI_ATTROWNER_DETAIL);

	omsg("process_assets:     Detail: ");

	for (int attrib_index=0; attrib_index < int(detail_attrib_names.size());
	    ++attrib_index) {

		ofmsg("%1% ", %detail_attrib_names[attrib_index]);

	}

	omsg("\n");
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
		omsg("process_assets:     Instancer (TODO)");
	}

	// TODO: render curves
	if (part.info().type == HAPI_PARTTYPE_CURVE) {
 		ofmsg ("process_assets:     Curve: %1% %2%", %part.name() %part.id);

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
	}

	if (part.info().type == HAPI_PARTTYPE_MESH) {
		ofmsg("process_assets:     Mesh (faceCount: %1% pointCount: %2% vertCount: %3%)", 
			%part.info().faceCount
			%part.info().pointCount
			%part.info().vertexCount
		);

		// no faces..
		if (part.info().faceCount == 0) {
			// but has points, so draw them?
			if (part.info().pointCount > 0) {
				foreach(Vector3f point, points) {
					hg->addVertex(point, partIndex, geoIndex, objIndex);
				}
				osg::PrimitiveSet::Mode myType = osg::PrimitiveSet::POINTS;
				hg->addPrimitiveOsg(myType, 0, part.info().pointCount - 1, partIndex, geoIndex, objIndex);
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

		// Material handling
		process_materials(part, hg);

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

		ofmsg("process_assets:   make primitive set face size %1%, from %2% plus %3%",
			%prev_faceCount
			%prev_faceCountIndex
			%(curr_index - prev_faceCountIndex)
		);

		hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

		hg->dirty();

	}

	if (part.info().type == HAPI_PARTTYPE_VOLUME) {
		omsg("process_assets:     Volume (TODO)");
	}

	if (part.info().type == HAPI_PARTTYPE_BOX) {
		omsg("process_assets:     Box (TODO)");
	}
	// todo: check out some geometry shaders for this..
	if (part.info().type == HAPI_PARTTYPE_SPHERE) {
		omsg("process_assets:     Sphere (TODO)");
	}

}


void HoudiniEngine::process_materials(const hapi::Part &part, HoudiniGeometry* hg) {
	bool all_same = true;

	std::vector< HAPI_NodeId > mat_ids_on_faces( part.info().faceCount );


	ENSURE_SUCCESS(session,  HAPI_GetMaterialNodeIdsOnFaces(
		session,
		part.geo.info().nodeId,
		part.id,
		&all_same /* are_all_the_same*/,
		mat_ids_on_faces.data(), 
		0, part.info().faceCount ));

	std::vector< int > mat_ids;

	if (!all_same) {
		for (vector<int>::iterator it = mat_ids.begin(); it < mat_ids.end(); ++it) {
			if (std::find(mat_ids.begin(), mat_ids.end(), *it) == mat_ids.end()) {
				mat_ids.push_back(*it);
			}
		}
	} else {
		mat_ids.push_back(mat_ids_on_faces[0]);
	}

	ofmsg(" unique mat ids: %1%", %mat_ids.size());
	for (int i =0; i < mat_ids.size(); ++i) {
		ofmsg("%1%", %mat_ids[i]);
	}

	for (int i = 0; i < mat_ids.size(); ++i) {

		HAPI_MaterialInfo mat_info;

		ENSURE_SUCCESS(session,  HAPI_GetMaterialInfo (
			session,
			mat_ids_on_faces[i],
			&mat_info));

		if (mat_info.exists) {
			ofmsg("process_assets:   Material info for %1%: nodeId: %2% hasChanged: %3%",
				%hg->getName() %mat_info.nodeId %mat_info.hasChanged
			);

			HAPI_NodeInfo node_info;

			hapi::Asset matNode(mat_info.nodeId, session);
			assetMaterialNodeIds[part.geo.object.asset.name()].push_back(mat_info.nodeId);

			std::map<std::string, hapi::Parm> parmMap = matNode.parmMap();

			int diffuseMapParmId = -1;
			int normalMapParmId = -1;

			string diffuseMapName;
			string normalMapName;

			omsg("looking for diffuse map");
			if (parmMap.count("ogl_tex1")) {
				// only assign if not empty
				if (parmMap["ogl_tex1"].getStringValue(0) != "") {
					diffuseMapParmId = parmMap["ogl_tex1"].info().id;
					diffuseMapName = parmMap["ogl_tex1"].name();
					ofmsg("map found, value is %1%", %parmMap["ogl_tex1"].getStringValue(0));
				}
			} else if (parmMap.count("baseColorMap")) {
				// only assign if not empty
				if (parmMap["baseColorMap"].getStringValue(0) != "") {
					diffuseMapParmId = parmMap["baseColorMap"].info().id;
					diffuseMapName = parmMap["baseColorMap"].name();
					ofmsg("map found, value is %1%", %parmMap["baseColorMap"].getStringValue(0));
				}
			} else if (parmMap.count("map")) {
				// only assign if not empty
				if (parmMap["map"].getStringValue(0) != "") {
					diffuseMapParmId = parmMap["map"].info().id;
					diffuseMapName = parmMap["map"].name();
					ofmsg("map found, value is %1%", %parmMap["map"].getStringValue(0));
				}
			}

			omsg("looking for normal map");
			if (parmMap.count("ogl_normalmap")) {
				// only assign if not empty
				if (parmMap["ogl_normalmap"].getStringValue(0) != "") {
					normalMapParmId = parmMap["ogl_normalmap"].info().id;
					normalMapName = parmMap["ogl_normalmap"].name();
					ofmsg("map found, value is %1%", %parmMap["ogl_normalmap"].getStringValue(0));
				}
			}

			// omsg("looking for diffuse colour");
			// if (parmMap.count("ogl_diff")) {
			// }

			omsg("looking for alpha material colour");
			if (parmMap.count("ogl_alpha")) {
				ofmsg("has alpha, value is %1%", %parmMap["ogl_alpha"].getFloatValue(0));
			}

			// omsg("looking for specular");
			// if (parmMap.count("ogl_spec")) {
			// }

			// omsg("looking for shininess");
			// if (parmMap.count("ogl_rough")) {
			// }

			if (diffuseMapParmId >= 0) {

				omsg("diffuse map found..");
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


				ofmsg("process_assets:   width %1% height: %2% format: %3% dataFormat: %4% packing %5% interleaved %6% gamma %7%",
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

				ofmsg("image plane count: %1%", %planeCount);

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
				assetMaterialParms[hg->getName()]["diffuseMapName"] = diffuseMapName;


				// need to set wrap modes too
				osg::Texture::WrapMode textureWrapMode;
				textureWrapMode = osg::Texture::REPEAT;

				texture->setWrap(osg::Texture2D::WRAP_R, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_S, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_T, textureWrapMode);

				// Update materials on each instance of this asset
				if (assetInstances.count(hg->getName()) > 0) {
					// update diffuse map texture
					ofmsg("updating %1%'s texture %2%", %hg->getName() %diffuseMapName);
					assetInstances[hg->getName()]->getMaterial()->setDiffuseTexture(diffuseMapName);
				}
			}

			if (normalMapParmId >= 0) {

				omsg("normal map found..");
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


				ofmsg("process_assets:   width %1% height: %2% format: %3% dataFormat: %4% packing %5% interleaved %6% gamma %7%",
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

				ofmsg("image plane count: %1%", %planeCount);

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
				assetMaterialParms[hg->getName()]["normalMapName"] = normalMapName;


				// need to set wrap modes too
				osg::Texture::WrapMode textureWrapMode;
				textureWrapMode = osg::Texture::REPEAT;

				texture->setWrap(osg::Texture2D::WRAP_R, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_S, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_T, textureWrapMode);

				// Update materials on each instance of this asset
				if (assetInstances.count(hg->getName()) > 0) {
					// update normal map texture
					ofmsg("updating %1%'s normal map to %2%", %hg->getName() %normalMapName);
					assetInstances[hg->getName()]->getMaterial()->setNormalTexture(normalMapName);
				}
			}


		} else {
			ofmsg("process_assets:   Invalid material %1% for %2%", %i %hg->getName());
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
	            ofmsg("cooking...:%1%", %result);
	            delete[] statusBuf;
	        }
		}
		HAPI_GetStatus(session, HAPI_STATUS_COOK_STATE, &status);
	}
	while ( status > HAPI_STATE_MAX_READY_STATE );
	ENSURE_COOK_SUCCESS( session, status );
	ofmsg("cooked :%1%", %status);
}
