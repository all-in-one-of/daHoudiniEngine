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
    vector<hapi::Object> objects = asset.objects();
	ofmsg("process_assets: %1%: %2% objects", %asset.name() %objects.size());

	String s = ostr("%1%", %asset.name());

	HoudiniGeometry* hg;

	if (myHoudiniGeometrys.count(s) > 0) {
		hg = myHoudiniGeometrys[s];
	} else {
		hg = HoudiniGeometry::create(s);
		myHoudiniGeometrys[s] = hg;

	}

	hg->objectsChanged = asset.info().haveObjectsChanged;
	ofmsg("process_assets: Objects have changed:  %1%", %hg->objectsChanged);

	if (hg->getObjectCount() < objects.size()) {
		hg->addObject(objects.size() - hg->getObjectCount());
	}

	if (hg->objectsChanged) {
// 	if (true) {
		for (int object_index=0; object_index < int(objects.size()); ++object_index)
	    {
			HAPI_ObjectInfo objInfo = objects[object_index].info();

			// TODO check for instancing, then do things differently
			if (objInfo.isInstancer > 0) {
				ofmsg("instance path: %1%: %2%", %objects[object_index].name() %objects[object_index].objectInstancePath());
				ofmsg("%1%: %2%", %objects[object_index].id %objInfo.objectToInstanceId);
			}

			vector<hapi::Geo> geos = objects[object_index].geos();
			ofmsg("%1%: %2%: %3% geos", %objects[object_index].name() %object_index %geos.size());

			if (hg->getGeodeCount(object_index) < geos.size()) {
				hg->addGeode(geos.size() - hg->getGeodeCount(object_index), object_index);
			}

			hg->setGeosChanged(objInfo.haveGeosChanged, object_index);
			ofmsg("process_assets: Object Geos %1% have changed: %2%", %object_index %hg->getGeosChanged(object_index));

			if (hg->getGeosChanged(object_index)) {
// 			if (true) {

				for (int geo_index=0; geo_index < int(geos.size()); ++geo_index)
				{
				    vector<hapi::Part> parts = geos[geo_index].parts();
					ofmsg("process_assets: Geo %1%:%2% %3% parts", %object_index %geo_index %parts.size());

					if (hg->getDrawableCount(geo_index, object_index) < parts.size()) {
						hg->addDrawable(parts.size() - hg->getDrawableCount(geo_index, object_index), geo_index, object_index);
					}

					hg->setGeoChanged(geos[geo_index].info().hasGeoChanged, geo_index, object_index);
					ofmsg("process_assets: Geo %1%:%2% has changed:  %3%", %object_index %geo_index %hg->getGeosChanged(object_index));

					hg->clear(geo_index, object_index);
					if (hg->getGeoChanged(geo_index, object_index)) {
// 					if (true) {
					    for (int part_index=0; part_index < int(parts.size()); ++part_index)
						{
							ofmsg("processing %1% %2%", %s %parts[part_index].name());
							process_geo_part(parts[part_index], object_index, geo_index, part_index, hg);
						}
					}
				}
			}

 			hg->setTransformChanged(objInfo.hasTransformChanged, object_index);
			ofmsg("process_assets: Object %1% Transform has changed: %2%", %object_index %hg->getTransformChanged(object_index));
		}

		HAPI_Transform* objTransforms = new HAPI_Transform[objects.size()];
		// NB: this resets all ObjectInfo::hasTransformChanged flags to false
		ENSURE_SUCCESS(session, HAPI_GetObjectTransforms( session, asset.id, HAPI_RSTORDER_DEFAULT, objTransforms, 0, objects.size()));

		for (int object_index=0; object_index < int(objects.size()); ++object_index)
	    {
			if (hg->getTransformChanged(object_index)) {
// 			if (true) {
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
	    delete[] objTransforms;
	}

	// do asset matrix transforms
	HAPI_TransformEuler assetTransformEuler;
	ENSURE_SUCCESS(session,
		HAPI_GetAssetTransform(
			session,
			asset.id,
			HAPI_RSTORDER_DEFAULT,
			HAPI_XYZORDER_DEFAULT,
			&assetTransformEuler
		)
	);

	// should have the matrix here.. check it out.
	ofmsg("matrix pos   %1% %2% %3%", %assetTransformEuler.position[0]
		                              %assetTransformEuler.position[1]
		                              %assetTransformEuler.position[2]);

	ofmsg("matrix rot   %1% %2% %3%", %assetTransformEuler.rotationEuler[0]
		                              %assetTransformEuler.rotationEuler[1]
		                              %assetTransformEuler.rotationEuler[2]);

	ofmsg("matrix scale %1% %2% %3%", %assetTransformEuler.scale[0]
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
	vector<Vector3f> points;
	vector<Vector3f> normals;
	vector<Vector3f> colors;

	// texture coordinates
	vector<Vector3f> uvs;

	bool has_point_normals = false;
	bool has_vertex_normals = false;
	bool has_point_colors = false;
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

	omsg("Point Attributes:");

    for (int attrib_index=0; attrib_index < int(point_attrib_names.size());
	    ++attrib_index) {

		ofmsg("has %1%", %point_attrib_names[attrib_index]);

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
		if (point_attrib_names[attrib_index] == "uv") {
			has_point_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_POINT, "uv", uvs);
		}
	}

    vector<std::string> vertex_attrib_names = part.attribNames(
	HAPI_ATTROWNER_VERTEX);

	omsg("Vertex Attributes");

	for (int attrib_index=0; attrib_index < int(vertex_attrib_names.size());
	    ++attrib_index) {

		ofmsg("has %1%", %vertex_attrib_names[attrib_index]);

		if (vertex_attrib_names[attrib_index] == "N") {
			has_vertex_normals = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "N", normals);
		}
		if (vertex_attrib_names[attrib_index] == "uv") {
			has_vertex_uvs = true;
		    process_float_attrib(part, HAPI_ATTROWNER_VERTEX, "uv", uvs);
		}
	}

    vector<std::string> primitive_attrib_names = part.attribNames(
	HAPI_ATTROWNER_PRIM);

	omsg("Primitive Attributes");

	for (int attrib_index=0; attrib_index < int(primitive_attrib_names.size());
	    ++attrib_index) {

		ofmsg("has %1%", %primitive_attrib_names[attrib_index]);

		if (primitive_attrib_names[attrib_index] == "Cd") {
			has_primitive_colors = true;
		    process_float_attrib(part, HAPI_ATTROWNER_PRIM, "Cd", colors);
		}
	}

    vector<std::string> detail_attrib_names = part.attribNames(
	HAPI_ATTROWNER_DETAIL);

	omsg("Detail Attributes");

	for (int attrib_index=0; attrib_index < int(detail_attrib_names.size());
	    ++attrib_index) {

		ofmsg("has %1%", %detail_attrib_names[attrib_index]);

	}

	if (part.info().faceCount == 0) {
		// this is to do with instancing
 		ofmsg ("No faces, points count? %1%", %points.size());
		hg->dirty();
 		return;
	}

	// TODO: render curves
	// CONTINUE FROM HERE, rendering these curves!
	if (part.info().type == HAPI_PARTTYPE_CURVE) {
 		ofmsg ("This part is a curve: %1% %2%", %part.name() %part.id);

		HAPI_CurveInfo curve_info;
		ENSURE_SUCCESS(session, HAPI_GetCurveInfo(
			session,
			part.geo.object.asset.id,
			part.geo.object.id,
			part.geo.id,
	        part.id,
			&curve_info
		) );

		if ( curve_info.curveType == HAPI_CURVETYPE_LINEAR )
			std::cout << "curve mesh type = Linear" << std::endl;
		else if ( curve_info.curveType == HAPI_CURVETYPE_BEZIER )
			std::cout << "curve mesh type = Bezier" << std::endl;
		else if ( curve_info.curveType == HAPI_CURVETYPE_NURBS )
			std::cout << "curve mesh type = Nurbs" << std::endl;
		else
			std::cout << "curve mesh type = Unknown" << std::endl;

		std::cout << "curve count: " << curve_info.curveCount << std::endl;
		int vertex_offset = 0;
		int knot_offset = 0;
		int segments = 20;

		for ( int i = 0; i < curve_info.curveCount; i++ ) {
			std::cout
				<< "curve " << i + 1 << " of "
				<< curve_info.curveCount << ":" << std::endl;
			// Number of CVs
			int num_vertices;
			HAPI_GetCurveCounts(
				session,
				part.geo.object.asset.id,
				part.geo.object.id,
				part.geo.id,
		        part.id,
				&num_vertices,
				i,
				1
			);
			std::cout << "num vertices: " << num_vertices << std::endl;
			// Order of this particular curve
			int order;
			if ( curve_info.order != HAPI_CURVE_ORDER_VARYING
				&& curve_info.order != HAPI_CURVE_ORDER_INVALID )
				order = curve_info.order;
			else
				HAPI_GetCurveOrders(
					session,
					part.geo.object.asset.id,
					part.geo.object.id,
					part.geo.id,
			        part.id,
					&order,
					i,
					1
				);
			std::cout << "curve order: " << order << std::endl;
			// If there's not enough vertices, then don't try to
			// create the curve.
			if ( num_vertices < order )
			{
				std::cout << "not enought vertices on curve " << i << " of "
					<< curve_info.curveCount << ": skipping" << std::endl;
				// The curve at i will have numVertices vertices, and may have
				// some knots. The knot count will be numVertices + order for
				// nurbs curves.
				vertex_offset += num_vertices * 4;
				knot_offset += num_vertices + order;
				continue;
			}

			cout << "points size: " << points.size() << endl;
			cout << "vert size: " << num_vertices << endl;

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
					cout << p << endl;
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

// 			if (curve_info.hasKnots) {
// 				std::vector< float > knots;
// 				knots.resize( num_vertices + order );
// 				HAPI_GetCurveKnots(
// 					session,
// 					part.geo.object.asset.id,
// 					part.geo.object.id,
// 					part.geo.id,
// 			        part.id,
// 					&knots.front(),
// 					knot_offset,
// 					num_vertices + order
//
// 				);
// 				for( int j = 0; j < num_vertices + order; j++ ) {
// 					cout << "knot " << j << ": " << knots[ j ] << endl;
// 				}
// 			}

		}
		hg->dirty();
		return;
	}

    int *face_counts = new int[ part.info().faceCount ];
    ENSURE_SUCCESS(session,  HAPI_GetFaceCounts(
		session,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		face_counts,
		0,
		part.info().faceCount
	) );

	// Material handling (WIP)

	bool all_same = true;
	HAPI_MaterialId* mat_ids = new HAPI_MaterialId[ part.info().faceCount ];

    ENSURE_SUCCESS(session,  HAPI_GetMaterialIdsOnFaces(
		session,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		&all_same /* are_all_the_same*/,
		mat_ids, 0, part.info().faceCount ));

	HAPI_MaterialInfo mat_info;

	ENSURE_SUCCESS(session,  HAPI_GetMaterialInfo (
		session,
		part.geo.object.asset.id,
		mat_ids[0],
		&mat_info));

	if (mat_info.exists) {
		ofmsg("Material info for %1%: matId: %2% assetId: %3% nodeId: %4%",
			  %hg->getName() %mat_info.id %mat_info.assetId %mat_info.nodeId
		);

		HAPI_NodeInfo node_info;
		ENSURE_SUCCESS(session,  HAPI_GetNodeInfo(
			session,
			mat_info.nodeId, &node_info ));
		ofmsg("Node info for %1%: id: %2% assetId: %3% internal path: %4%",
			  %get_string( session, node_info.nameSH) %node_info.id %node_info.assetId
			  %get_string( session, node_info.internalNodePathSH)
		);

		HAPI_ParmInfo* parm_infos = new HAPI_ParmInfo[node_info.parmCount];
		ENSURE_SUCCESS(session,  HAPI_GetParameters(
			session,
			mat_info.nodeId,
			parm_infos,
			0 /* start */,
			node_info.parmCount));

		int mapIndex = -1;

		for (int i =0; i < node_info.parmCount; ++i) {
			// node parm output
// 			ofmsg("%1% %2% (%3%) id = %4%",
// 				  %i
// 				  %get_string( session, parm_infos[i].labelSH)
// 				  %get_string( session, parm_infos[i].nameSH)
// 				  %parm_infos[i].id);
			if (parm_infos[i].stringValuesIndex >= 0) {
				int sh = -1;
				ENSURE_SUCCESS(session,  HAPI_GetParmStringValues(
					session,
					mat_info.nodeId, true,
					&sh,
					parm_infos[i].stringValuesIndex, 1)
				);
				// more node parm output
// 				ofmsg("%1%  %2%: %3%", %i %parm_infos[i].id
// 				                       %get_string(session, sh));

				if (sh != -1 && (get_string(session, parm_infos[i].nameSH) == "baseColorMap")) {
// 				if (sh != -1 && (get_string(session, parm_infos[i].nameSH) == "map")) {
					mapIndex = i;
				}
			}
		}

        if (mapIndex == -1) {
		    ofmsg("Unsupported material found for %1%", %hg->getName());
        } else {

            // debug output
    // 		for (int i = 0; i < node_info.parmCount; ++i) {
    // 			ofmsg("index %1%: %2% '%3%'",
    // 				  %i
    // 				  %parm_infos[i].id
    // 				  %get_string(session, parm_infos[i].nameSH)
    // 			);
    // 		}

    // 		ofmsg("the texture path: assetId=%1% matInfo.id=%2% mapIndex=%3% parm_id=%4% string=%5%",
    // 			  %mat_info.assetId
    // 			  %mat_info.id
    // 			  %mapIndex
    // 			  %parm_infos[mapIndex].id
    // 			  %get_string(session, parm_infos[mapIndex].nameSH)
    // 		);

            // NOTE this works if the image is a png
            ENSURE_SUCCESS(session,  HAPI_RenderTextureToImage(
                session,
                mat_info.assetId,
                mat_info.id,
                parm_infos[mapIndex].id /* parmIndex for "map" */));


            // render using mantra
    //  		ENSURE_SUCCESS(session,  HAPI_RenderMaterialToImage(
    //  			mat_info.assetId,
    //  			mat_info.id,
    // 			HAPI_SHADER_MANTRA));
    // // 			HAPI_SHADER_OPENGL));

            HAPI_ImageInfo image_info;
            ENSURE_SUCCESS(session,  HAPI_GetImageInfo(
                session,
                mat_info.assetId,
                mat_info.id,
                &image_info));

            ofmsg("width %1% height: %2% format: %3% dataFormat: %4% packing %5%",
                  %image_info.xRes
                  %image_info.yRes
                  %get_string(session, image_info.imageFileFormatNameSH)
                  %image_info.dataFormat
                  %image_info.packing
            );
            // ---------

            HAPI_StringHandle imageSH;

            ENSURE_SUCCESS(session,  HAPI_GetImagePlanes(
                session,
                mat_info.assetId,
                mat_info.id,
                &imageSH,
                1
            ));

            int imgBufSize = -1;

            //TODO: get the image extraction working correctly..
            // needed to convert from PNG/JPG/etc to RGBA.. use decode() from omegalib

            // get image planes into a buffer (default is png.. change to RGBA?)
            ENSURE_SUCCESS(session,  HAPI_ExtractImageToMemory(
                session,
                mat_info.assetId,
                mat_info.id,
                HAPI_PNG_FORMAT_NAME,
    // 			NULL /* HAPI_DEFAULT_IMAGE_FORMAT_NAME */,
                "C A", /* image planes */
                &imgBufSize
            ));

            char *myBuffer = new char[imgBufSize];

            // put into a buffer
            ENSURE_SUCCESS(session,  HAPI_GetImageMemoryBuffer(
                session,
                mat_info.assetId,
                mat_info.id,
                myBuffer /* tried (char *)pd->map() */,
                imgBufSize
            ));

            // load into a pixelData bufferObject
            // this works!
    // 		Ref<PixelData> refPd = ImageUtils::decode((void *) myBuffer, image_info.xRes * image_info.yRes * 4);
            pds.push_back(ImageUtils::decode((void *) myBuffer, image_info.xRes * image_info.yRes * 4));

            // TODO: general case for texture names (diffuse, spec, env, etc)
            osg::Texture2D* texture = mySceneManager->createTexture("testing", pds[0]);

            // need to set wrap modes too
            osg::Texture::WrapMode textureWrapMode;
            textureWrapMode = osg::Texture::REPEAT;

            texture->setWrap(osg::Texture2D::WRAP_R, textureWrapMode);
            texture->setWrap(osg::Texture2D::WRAP_S, textureWrapMode);
            texture->setWrap(osg::Texture2D::WRAP_T, textureWrapMode);

            // should have something else here.. this doesn't seem to be working..
            // TODO: put textures in HoudiniGeometry, use default shader to show everything properly
    // 		hg->getOsgNode(geoIndex, objIndex)->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);

    // 		ofmsg("my image width %1% height: %2%, size %3%, bufSize %4%",
    // 			%refPd->getWidth() %refPd->getHeight() %refPd->getSize() %imgBufSize
    // 		);
        }
	} else {
		ofmsg("No material for %1%", %hg->getName());
	}

	// end materials

    int * vertex_list = new int[ part.info().vertexCount ];
    ENSURE_SUCCESS(session,  HAPI_GetVertexList(
		session,
		part.geo.object.asset.id,
		part.geo.object.id,
		part.geo.id,
        part.id,
		vertex_list, 0, part.info().vertexCount ) );
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

			if (prev_faceCount == 3) {
				myType = osg::PrimitiveSet::TRIANGLES;
			} else if (prev_faceCount == 4) {
				myType = osg::PrimitiveSet::QUADS;
			}

// 			cout << "making primitive set for " << prev_faceCount << ", from " <<
// 				prev_faceCountIndex << " plus " <<
// 				curr_index - prev_faceCountIndex << endl;

			hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

			prev_faceCountIndex = curr_index;
			prev_faceCount = face_counts[ii];
		} else if ((ii > 0) && (prev_faceCount > 4)) {
// 			cout << "making primitive set for " << prev_faceCount << ", plus " <<
// 				prev_faceCountIndex << " to " <<
// 				curr_index - prev_faceCountIndex << endl;
			hg->addPrimitiveOsg(osg::PrimitiveSet::TRIANGLE_FAN,
				prev_faceCountIndex,
			    curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

			prev_faceCountIndex = curr_index;
			prev_faceCount = face_counts[ii];
		}

// 		cout << "face (" << face_counts[ii] << "): " << ii << " ";
        for( int jj=0; jj < face_counts[ii]; jj++ )
        {

			int myIndex = curr_index + (face_counts[ii] - jj) % face_counts[ii];
// 			cout << "i: " << vertex_list[myIndex] << " ";

			int lastIndex = hg->addVertex(points[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);

			if (has_point_normals) {
				hg->addNormal(normals[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
			} else if (has_vertex_normals) {
				hg->addNormal(normals[myIndex], partIndex, geoIndex, objIndex);
			}
			if(has_point_colors) {
				hg->addColor(Color(
					colors[vertex_list[ myIndex ]][0],
					colors[vertex_list[ myIndex ]][1],
					colors[vertex_list[ myIndex ]][2],
					1.0
				), partIndex, geoIndex, objIndex);
			} else if (has_primitive_colors) {
				hg->addColor(Color(
					colors[ii][0],
					colors[ii][1],
					colors[ii][2],
					1.0
				), partIndex, geoIndex, objIndex);
			}
			if (has_point_uvs) {
				hg->addUV(uvs[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
// 				cout << "(p)uvs: " << uvs[vertex_list[ myIndex ]][0] << ", " << uvs[vertex_list[ myIndex ]][1] << endl;
			} else if (has_vertex_uvs) {
// 				hg->addUV(uvs[vertex_list[ myIndex ]], partIndex, geoIndex, objIndex);
				hg->addUV(uvs[myIndex], partIndex, geoIndex, objIndex);
// 				cout << "(v)uvs: " << uvs[myIndex][0] << ", " << uvs[myIndex][1] << endl;
			}

//             cout << "v:" << myIndex << ", i: "
// 				<< hg->getVertex(lastIndex) << endl; //" ";
        }

        curr_index += face_counts[ii];

    }

	cout << "face count is " << prev_faceCount << endl;

	if (prev_faceCount == 3) {
		myType = osg::PrimitiveSet::TRIANGLES;
	} else if (prev_faceCount == 4) {
		myType = osg::PrimitiveSet::QUADS;
	}
	if (prev_faceCount > 4) {
		myType = osg::PrimitiveSet::TRIANGLE_FAN;
	}

	cout << "making final primitive set for " << prev_faceCount << ", from " <<
		prev_faceCountIndex << " plus " <<
		curr_index - prev_faceCountIndex << endl;

	hg->addPrimitiveOsg(myType, prev_faceCountIndex, curr_index - prev_faceCountIndex, partIndex, geoIndex, objIndex);

	hg->dirty();

    delete[] face_counts;
    delete[] vertex_list;

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
