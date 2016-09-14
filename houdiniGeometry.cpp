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


#include <osg/Node>
#include <osg/Geometry>

#include <daHoudiniEngine/houdiniGeometry.h>

using namespace houdiniEngine;

///////////////////////////////////////////////////////////////////////////////
HoudiniGeometry::HoudiniGeometry(const String& name):
	ModelGeometry(name)
{
	// create geometry and geodes to hold the data
// 	myNode = new osg::Geode();
	myNode = new osg::PositionAttitudeTransform();

	addObject(1);
	addGeode(1, 0);
	addDrawable(1, 0, 0);

}

// add a drawable (osg::geometry) to a given geode
// drawables are stored in HParts for an HGeom
///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addDrawable(const int count, const int geodeIndex, const int objIndex)
{

	for (int i = 0; i < count; ++i) {
		hobjs[objIndex].hgeoms[geodeIndex].hparts.push_back(HPart());
		hobjs[objIndex].hgeoms[geodeIndex].hparts.back().geometry = new osg::Geometry();
		osg::VertexBufferObject* vboP = hobjs[objIndex].hgeoms[geodeIndex].hparts.back().geometry->getOrCreateVertexBufferObject();
		vboP->setUsage (GL_STREAM_DRAW);

		hobjs[objIndex].hgeoms[geodeIndex].hparts.back().vertices = new osg::Vec3Array();

		hobjs[objIndex].hgeoms[geodeIndex].hparts.back().geometry->setUseDisplayList (false);
		hobjs[objIndex].hgeoms[geodeIndex].hparts.back().geometry->setUseVertexBufferObjects(true);
		hobjs[objIndex].hgeoms[geodeIndex].hparts.back().geometry->setVertexArray(hobjs[objIndex].hgeoms[geodeIndex].hparts.back().vertices);
		hobjs[objIndex].hgeoms[geodeIndex].geode->addDrawable(hobjs[objIndex].hgeoms[geodeIndex].hparts.back().geometry);
	}
	return hobjs[objIndex].hgeoms[geodeIndex].geode->getNumDrawables();
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addGeode(const int count, const int objIndex)
{
	for (int i = 0; i < count; ++i) {
		hobjs[objIndex].hgeoms.push_back(HGeom());
		hobjs[objIndex].hgeoms.back().geode = new osg::Geode();
		hobjs[objIndex].pat->addChild(hobjs[objIndex].hgeoms.back().geode);
	}
	return hobjs[objIndex].pat->getNumChildren();
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addObject(const int count)
{
	for (int i = 0; i < count; ++i) {
		hobjs.push_back(HObj());
		hobjs.back().pat = new osg::PositionAttitudeTransform();
		myNode->addChild(hobjs.back().pat);
	}
	return myNode->getNumChildren();
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addVertex(const Vector3f& v, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts.size() > drawableIndex);
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->push_back(osg::Vec3d(v[0], v[1], v[2]));
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->dirtyBound();
	return hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setVertex(int index, const Vector3f& v, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->size() > index);
	osg::Vec3f& c = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->at(index);
	c[0] = v[0];
	c[1] = v[1];
	c[2] = v[2];
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Vector3f HoudiniGeometry::getVertex(int index, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->size() > index);
	const osg::Vec3f& v = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].vertices->at(index);
	return Vector3f(v[0], v[1], v[2]);
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addColor(const Color& c, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts.size() > drawableIndex);
	if(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors == NULL)
	{
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors = new osg::Vec4Array();
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->setColorArray(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors);
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
	}
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->push_back(osg::Vec4d(c[0], c[1], c[2], c[3]));
	return hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setColor(int index, const Color& col, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors != NULL && hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->size() > index);
	osg::Vec4f& c = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->at(index);
	c[0] = col[0];
	c[1] = col[1];
	c[2] = col[2];
	c[3] = col[3];
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Color HoudiniGeometry::getColor(int index, const int drawableIndex, const int geodeIndex, const int	objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors != NULL && hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->size() > index);
	const osg::Vec4d& c = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].colors->at(index);
	return Color(c[0], c[1], c[2], c[3]);
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	osg::PrimitiveSet::Mode osgPrimType;
	switch(type)
	{
	case ProgramAsset::Triangles:
		osgPrimType = osg::PrimitiveSet::TRIANGLES; break;
	case ProgramAsset::Points:
		osgPrimType = osg::PrimitiveSet::POINTS; break;
	case ProgramAsset::TriangleStrip:
		osgPrimType = osg::PrimitiveSet::TRIANGLE_STRIP; break;
	}
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->addPrimitiveSet(new osg::DrawArrays(osgPrimType, startIndex, endIndex));
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->addPrimitiveSet(new osg::DrawArrays(type, startIndex, endIndex));
}



///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::clear()
{
	for (int obj = 0; obj < hobjs.size(); ++obj) {
		clear(obj);
	}
}

void HoudiniGeometry::clear(const int objIndex)
{
	for (int g = 0; g < hobjs[objIndex].hgeoms.size(); ++g) {
		clear(g, objIndex);
	}
}
void HoudiniGeometry::clear(const int geodeIndex, const int objIndex)
{
	for (int i = 0; i < hobjs[objIndex].hgeoms[geodeIndex].hparts.size(); ++i) {
		clear(i, geodeIndex, objIndex);
	}
}
void HoudiniGeometry::clear(const int drawableIndex, const int geodeIndex, const int objIndex)
{
	HPart* hpart = &hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex];
	oassert(hpart != NULL);

	if (hpart->colors != NULL) hpart->colors->clear();
	if (hpart->normals != NULL) hpart->normals->clear();
	hpart->vertices->clear();
	hpart->geometry->removePrimitiveSet(0, hpart->geometry->getNumPrimitiveSets());
	hpart->geometry->dirtyBound();
}


///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::dirty()
{
	for (int obj = 0; obj < hobjs.size(); ++obj) {
		for (int g = 0; g < hobjs[obj].hgeoms.size(); ++g) {
			for (int i = 0; i < hobjs[obj].hgeoms[g].hparts.size(); ++i) {
				hobjs[obj].hgeoms[g].hparts[i].vertices->dirty();
				if (hobjs[obj].hgeoms[g].hparts[i].colors != NULL) hobjs[obj].hgeoms[g].hparts[i].colors->dirty();
				if (hobjs[obj].hgeoms[g].hparts[i].normals != NULL) hobjs[obj].hgeoms[g].hparts[i].normals->dirty();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addNormal(const Vector3f& v, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts.size() > drawableIndex);
	if(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals == NULL) {
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals = new osg::Vec3Array();
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->setNormalArray(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals);
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
	}

	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->push_back(osg::Vec3d(v[0], v[1], v[2]));
	return hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setNormal(int index, const Vector3f& v, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->size() > index);
	osg::Vec3f& c = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->at(index);
	c[0] = v[0];
	c[1] = v[1];
	c[2] = v[2];
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Vector3f HoudiniGeometry::getNormal(int index, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->size() > index);
	const osg::Vec3f& c = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].normals->at(index);
	return Vector3f(c[0], c[1], c[2]);
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addUV(const Vector3f& uv, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts.size() > drawableIndex);
	if(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs == NULL) {
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs = new osg::Vec3Array();
		hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].geometry->
			setTexCoordArray(0, hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs, osg::Array::BIND_PER_VERTEX);
	}

	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs->push_back(osg::Vec3d(uv[0], uv[1], uv[2]));
	return hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setUV(int index, const Vector3f& uv, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs->size() > index);
	osg::Vec3f& c = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs->at(index);
	c[0] = uv[0];
	c[1] = uv[1];
	c[2] = uv[2];
	hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Vector3f HoudiniGeometry::getUV(int index, const int drawableIndex, const int geodeIndex, const int objIndex)
{
	oassert(hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs->size() > index);
	const osg::Vec3f& c = hobjs[objIndex].hgeoms[geodeIndex].hparts[drawableIndex].uvs->at(index);
	return Vector3f(c[0], c[1], c[2]);
}

