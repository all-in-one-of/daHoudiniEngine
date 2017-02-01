/**************************************************************************************************
 * THE OMEGA LIB PROJECT
 *-------------------------------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:
 *  Alessandro Febretti		febret@gmail.com
 *-------------------------------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************************************/
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

Based on ModelGeometry from the Cyclops project

******************************************************************************/


#ifndef __HE_HOUDINI_GEOMETRY__
#define __HE_HOUDINI_GEOMETRY__

#include <cyclops/cyclops.h>
// #include "cyclopsConfig.h"
// #include "EffectNode.h"
// #include "Uniforms.h"
// #include "SceneManager.h"

#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Billboard>

#define OMEGA_NO_GL_HEADERS
#include <omega.h>
#include <omegaOsg/omegaOsg.h>
#include <omegaToolkit.h>

#include <vector>

namespace houdiniEngine {
	using namespace cyclops;
	using namespace omega;
	using namespace omegaOsg;

	typedef struct {
 		Ref<osg::Vec3Array> vertices;
 		Ref<osg::Vec4Array> colors;
		Ref<osg::Vec3Array> normals;
		Ref<osg::Vec3Array> uvs;
 		Ref<osg::Geometry> geometry;
	} HPart;

	typedef struct {
		vector < HPart > hparts;
		Ref<osg::Geode> geode;
		bool geoChanged;
	} HGeom;

	typedef struct {
		vector < HGeom > hgeoms;
		Ref<osg::Transform> trans;
		bool transformChanged;
		bool geosChanged;
	} HObj;

	/*
	 * Houdini Geometry looks like this:
	 * based on info from http://www.sidefx.com/docs/hengine1.9/_h_a_p_i__objects_geos_parts.html
	 *
		This section assumes you have a successfully instantiated asset and a filled
		HAPI_AssetInfo struct. As a guide, start first by loading Objects, then Geos,
		and finally Parts.

		Objects

		Objects are the highest level nodes inside an asset. They are Houdini's
		equivalent of transform nodes. To get information on a asset's objects:

		Make sure you have a successfully instantiated asset and have a filled
		HAPI_AssetInfo struct as described in Assets.
		Allocate an array of HAPI_AssetInfo::objectCount HAPI_ObjectInfo's.
		Call HAPI_GetObjects() using HAPI_AssetInfo::id, the array you just allocated, 0
		for the start, and HAPI_AssetInfo::objectCount for the length. You can of course
		choose a different range within those values.
		Since objects are Houdini's transform nodes you're probably going to need their
		transforms as well which don't come as part of the HAPI_ObjectInfo struct. To
		get the object transforms, use HAPI_GetObjectTransforms() using the same asset
		id and object counts as with HAPI_GetObjects(). All transform information is
		provided in Houdini coordinates.

		Geos

		Geo nodes are equivalent to SOPs (Surface Operator nodes) in Houdini. Geos don't
		contain geometry directly, instead, they contain parts. To get information on a
		asset's geos:

		Make sure you have a valid filled HAPI_ObjectInfo struct as described in
		Objects.
		The geo ids will be the indicies from 0 to HAPI_ObjectInfo::geoCount - 1,
		inclusive. For each geo in this range, call HAPI_GetGeoInfo() to get a
		HAPI_GeoInfo struct filled.
		In most cases, there will only be a single Geo for each Object, corresponding to
		the visible SOP node in Houdini. However, there are cases where multiple Geos
		are exposed. For example, one might wish to expose multiple nodes in a SOP
		network. In these cases the node can be marked "templated" inside of Houdini,
		and its geometry will become exposed through HAPI. Another case where multiple
		Geos come into play is in the case of Intermediate Asset Results where we expose
		geometry midway through a cook, allow modifications on that geometry, and let
		the cook continue from that point with the modified geometry. You can
		distinguish between these different Geos by looking at the HAPI_GeoInfo::type,
		HAPI_GeoInfo::isDisplayGeo, and HAPI_GeoInfo::isTemplated fields of
		HAPI_GeoInfo.

		Parts

		Parts contain the actual mesh/geometry/attribute data you're interested in.

		Parts are computed based on the use of primitive groups within the geometry in
		Houdini. There will be 1 part per primitive group, plus another part containing
		all geometry that is not in a primitive group unless all primitives are part of
		at least one group. If within a primitive group there are multiple Houdini
		primitives, additional parts will be created per primitive. This splitting can
		be disabled via HAPI_CookOptions::splitGeosByGroup when passed in to
		HAPI_Initialize() or HAPI_CookAsset().

		To get information on a asset's parts:

		Make sure you have a valid filled HAPI_GeoInfo struct as described in Geos.
		The part ids will be the indicies from 0 to HAPI_GeoInfo::partCount - 1,
		inclusive. For each part in this range, call HAPI_GetPartInfo() to get a
		HAPI_PartInfo struct filled.
		Now you're ready to get the actual geometry data.

		You can get the array of face counts, where the nth integer in the array is the
		number of vertices the nth face has, by calling HAPI_GetFaceCounts() with 0 and
		HAPI_PartInfo::faceCount as your max range.

		You can get the array containing the vertex-point associations, where the ith
		element in the array is the point index to the ith vertex associates with, by
		calling HAPI_GetVertexList() with 0 and HAPI_PartInfo::vertexCount as your max
		range. As an example, if the first face had 3 vertices, the second face had 4
		vertices and the third face had 5 vertices, then the first 3 integers in the
		vertex_list are the point indices belonging to the first face, the next 4
		integers are those belonging to the second face, and the next 5 integers to the
		third face.

		The vertex list contains only the indices to the points because vertices share
		points. There is a separate list of points, which are a list of 3 vectors, and
		the vertices index into this list of points. Thus the integers you retrieve from
		the vertex list are indices into the point list.

		The actual point position information, along with all other geometry meta-data,
		is stored as Houdini attributes. See Attributes.
	*/
	/* Structure of HoudiniGeometry:
	 *
	 * PositionAttitudeTransform -> root node for the Asset
	 *   |
	 *   +-> PositionAttitudeTransform -> Transform/Object level
	 *         |
	 *         +-> Geode -> Geo/SOP level
	 *               |
	 *               +-> Geometry -> Part/Group in a Geo
	**/
	class HoudiniGeometry : public ModelGeometry
	{
	public:
		static HoudiniGeometry* create(const String& name)
		{
			return new HoudiniGeometry(name);
		}

		HoudiniGeometry(const String& name);

		//! Adds a vertex and return its index.
		int addVertex(
			const Vector3f& v,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		//! Replaces an existing vertex
		void setVertex(
			int index,
			const Vector3f& v,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);
		//! Retrieves an existing vertex
		Vector3f getVertex(
			int index,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);
		//! Adds a vertex color and return its index. The color will be applied
		//! to the vertex with the same index as this color.
		int addColor(
			const Color& c,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		Color getColor(
			int index,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		//! Replaces an existing color
		void setColor(
			int index,
			const Color& c,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		//! Adds a normal and return its index.
		int addNormal(
			const Vector3f& v,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);
		void setNormal(
			int index,
			const Vector3f& v,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);
		Vector3f getNormal(
			int index,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		//! Adds uv indices
		int addUV(
			const Vector3f& uv,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);
		void setUV(
			int index,
			const Vector3f& uv,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);
		Vector3f getUV(
			int index,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		void setVertexListSize(
			int size,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex) {
			hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->resize(size);
		};

		//! Adds a primitive set
		void addPrimitive(
			ProgramAsset::PrimitiveType type,
			int startIndex,
			int endIndex,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		void addPrimitiveOsg(
			osg::PrimitiveSet::Mode type,
			int startIndex,
			int endIndex,
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex
		);

		//! Removes all vertices, colors and primitives from this object
		void clear(const int drawableIndex, const int geodeIndex, const int objIndex);
		void clear(const int geodeIndex, const int objIndex);
		void clear(const int objIndex);
		void clear();

		int getDrawableCount(const int geodeIndex, const int objIndex) {
			if (objIndex < myNode->getNumChildren()) {
				if (geodeIndex < myNode->getChild(objIndex)->asGroup()->getNumChildren()) {
					return myNode->getChild(objIndex)->asGroup()->
						getChild(geodeIndex)->asGroup()->getNumChildren();
				}
			}
			return 0;
		};

		int addDrawable(const int count, const int geodeIndex, const int objIndex);

		int addGeode(const int count, const int objIndex);
		int getGeodeCount(const int objIndex) {
			if (objIndex < myNode->getNumChildren()) {
				return myNode->getChild(objIndex)->asGroup()->getNumChildren();
			}
			return 0;
		};
        void setGeodeName(const int geodeIndex, const int objIndex, const string& name) {
            hobjs[objIndex].hgeoms[geodeIndex].geode->setName(name);
        }

		int addBillboard(const int count, const int objIndex);

		int addObject(const int count);
		int getObjectCount() { return (myNode == NULL ? 0 : myNode->getNumChildren()); };
        void setObjectName(const int objIndex, const string& name) {
            hobjs[objIndex].trans->setName(name);
        }

		inline int getNormalCount(const int drawableIndex, const int geodeIndex, const int objIndex) {
			return (hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals == NULL) ?
			0 :
			hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->size();
		}
		inline int getVertexCount(const int drawableIndex, const int geodeIndex, const int objIndex) {
			return hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->size();
		}
		inline int getColorCount(const int drawableIndex, const int geodeIndex, const int objIndex) {
			return (hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors == NULL) ?
			0 :
			hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->size();
		}

		inline int getPrimitiveSetCount(
			const int drawableIndex,
			const int geodeIndex,
			const int objIndex) {
			return hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->getPrimitiveSetList().size();
		}

		osg::Node* getOsgNode() { return myNode; }
		osg::Geode* getOsgNode(const int geodeIndex) { return getOsgNode(0, 0); }
		osg::Geode* getOsgNode(const int geodeIndex, const int objIndex) {
			return myNode->getChild(objIndex)->asGroup()->getChild(geodeIndex)->asGeode();
		};

		bool objectsChanged;

		bool getTransformChanged(const int objIndex) {
			return hobjs[objIndex].transformChanged;
		}

		void setTransformChanged(bool value, const int objIndex) {
			hobjs[objIndex].transformChanged = value;
		}

		bool getGeosChanged(const int objIndex) {
			return hobjs[objIndex].geosChanged;
		}

		void setGeosChanged(bool value, const int objIndex) {
			hobjs[objIndex].geosChanged = value;
		}

		bool getGeoChanged(const int geodeIndex, const int objIndex) {
			return hobjs[objIndex].hgeoms[geodeIndex].geoChanged;
		}

		void setGeoChanged(bool value, const int geodeIndex, const int objIndex) {
			hobjs[objIndex].hgeoms[geodeIndex].geoChanged = value;
		}

		void dirty();

	private:
		vector < HObj > hobjs;
		osg::PositionAttitudeTransform* myNode;
	};
};

#endif
