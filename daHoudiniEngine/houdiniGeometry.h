#ifndef __HE_HOUDINI_GEOMETRY__
#define __HE_HOUDINI_GEOMETRY__

#include <cyclops/cyclops.h>
// #include "cyclopsConfig.h"
// #include "EffectNode.h"
// #include "Uniforms.h"
// #include "SceneManager.h"

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>

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
 		Ref<osg::Geometry> geometry;
	} HGeom;


	class HoudiniGeometry : public ModelGeometry
	{
	public:
		static HoudiniGeometry* create(const String& name)
		{
			return new HoudiniGeometry(name);
		}

		HoudiniGeometry(const String& name);

		//! Adds a vertex and return its index.
		int addVertex(const Vector3f& v);
		int addVertex(const Vector3f& v, const int drawableIndex);

		//! Replaces an existing vertex
		void setVertex(int index, const Vector3f& v);
		void setVertex(int index, const Vector3f& v, const int drawableIndex);
		//! Retrieves an existing vertex
		Vector3f getVertex(int index);
		Vector3f getVertex(int index, const int drawableIndex);
		//! Adds a vertex color and return its index. The color will be applied
		//! to the vertex with the same index as this color.
		int addColor(const Color& c);
		int addColor(const Color& c, const int drawableIndex);

		Color getColor(int index);
		Color getColor(int index, const int drawableIndex);
		//! Replaces an existing color
		void setColor(int index, const Color& c);
		void setColor(int index, const Color& c, const int drawableIndex);

		void setVertexListSize(int size) { setVertexListSize(size, 0); }
		void setVertexListSize(int size, const int drawableIndex) { hgeoms[drawableIndex].vertices->resize(size); }

		//! Adds a primitive set
		void addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex);
		void addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex, const int drawableIndex);

		void addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex);
		void addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex, const int drawableIndex);

		//! Removes all vertices, colors and primitives from this object
		void clear();


		int addDrawable(const int count);
// 		const String& getName() { return myName; }
// 		osg::Geode* getOsgNode() { return myNode; }

		int addNormal(const Vector3f& v);
		int addNormal(const Vector3f& v, const int drawableIndex);
		void setNormal(int index, const Vector3f& v);
		void setNormal(int index, const Vector3f& v, const int drawableIndex);
		Vector3f getNormal(int index);
		Vector3f getNormal(int index, const int drawableIndex);

		inline int getNormalCount() { return getNormalCount(0); }
		inline int getVertexCount() { return getVertexCount(0); }
		inline int getColorCount() { return getColorCount(0); }

		inline int getNormalCount(const int drawableIndex) {
			return (hgeoms[drawableIndex].normals == NULL) ? 0 : hgeoms[drawableIndex].normals->size();
		}
		inline int getVertexCount(const int drawableIndex) {
			return hgeoms[drawableIndex].vertices->size();
		}
		inline int getColorCount(const int drawableIndex) {
			return (hgeoms[drawableIndex].colors == NULL) ? 0 : hgeoms[drawableIndex].colors->size();
		}

		inline int getPrimitiveSetCount() { return getPrimitiveSetCount(0); }
		inline int getPrimitiveSetCount(const int drawableIndex) {
			return hgeoms[drawableIndex].geometry->getPrimitiveSetList().size();
		}

		void dirty();

	private:
// 		String myName;
// 		Ref<osg::Geode> myNode;
		vector < HGeom > hgeoms;
//  		vector< Ref<osg::Vec3Array> > myVerts;
//  		vector< Ref<osg::Vec4Array> > myCols;
//			vector< Ref<osg::Vec3Array> > myNorms;
//  		vector< Ref<osg::Geometry> > myGeoms;

// 		Ref<osg::Vec3Array> myVertices;
// 		Ref<osg::Vec4Array> myColors;
// 		Ref<osg::Geometry> myGeometry;
// 		Ref<osg::Vec3Array> myNormals;

	};
};

#endif


/*
/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory,
 *							University of Illinois at Chicago
 * Authors:
 *  Alessandro Febretti		febret@gmail.com
 *-----------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer. Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-----------------------------------------------------------------------------
 * What's in this file:
 *  ModelGeometry lets users define their own custom geometry for drawing.
 ****************************************************************************** /
#ifndef __CY_MODEL_GEOMETRY__
#define __CY_MODEL_GEOMETRY__

#include "cyclopsConfig.h"
#include "EffectNode.h"
#include "Uniforms.h"
#include "SceneManager.h"

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>

#define OMEGA_NO_GL_HEADERS
#include <omega.h>
#include <omegaOsg/omegaOsg.h>
#include <omegaToolkit.h>

namespace cyclops {
	using namespace omega;
	using namespace omegaOsg;

	///////////////////////////////////////////////////////////////////////////
	class CY_API ModelGeometry: public ReferenceType
	{
	public:
		//! Creation method, to reflect the python API of most objects.
		static ModelGeometry* create(const String& name)
		{
			return new ModelGeometry(name);
		}

	public:
		ModelGeometry(const String& name);

		//! Adds a vertex and return its index.
		int addVertex(const Vector3f& v);
		//! Replaces an existing vertex
		void setVertex(int index, const Vector3f& v);
		//! Retrieves an existing vertex
		Vector3f getVertex(int index);
		//! Adds a vertex color and return its index. The color will be applied
		//! to the vertex with the same index as this color.
		int addColor(const Color& c);
		Color getColor(int index);
		//! Replaces an existing color
		void setColor(int index, const Color& c);

		void setVertexListSize(int size) { myVertices->resize(size); }

		//! Adds a primitive set
		void addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex);

		void addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex);

		//! Removes all vertices, colors and primitives from this object
		void clear();

		const String& getName() { return myName; }
		osg::Geode* getOsgNode() { return myNode; }

	protected:
		String myName;
		Ref<osg::Vec3Array> myVertices;
		Ref<osg::Vec4Array> myColors;
		Ref<osg::Geode> myNode;
		Ref<osg::Geometry> myGeometry;
	};
};

#endif

*/