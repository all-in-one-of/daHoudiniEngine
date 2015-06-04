#include <osg/Node>
#include <osg/Geometry>

#include <daHoudiniEngine/houdiniGeometry.h>

using namespace houdiniEngine;

///////////////////////////////////////////////////////////////////////////////
HoudiniGeometry::HoudiniGeometry(const String& name):
// 	myName(name)
	ModelGeometry(name)
{
	// create geometry and geodes to hold the data
	myNode = new osg::Geode();

	addDrawable(1);

// 	hgeoms.push_back(new HGeom();
// 	hgeoms[0].geometry.push_back(new osg::Geometry());
// 	osg::VertexBufferObject* vboP = hgeoms[0].geometry->getOrCreateVertexBufferObject();
// 	vboP->setUsage (GL_STREAM_DRAW);
//
// 	myVerts.push_back(new osg::Vec3Array());
//
// 	hgeoms[0].geometry->setUseDisplayList (false);
// 	hgeoms[0].geometry->setUseVertexBufferObjects(true);
// 	hgeoms[0].geometry->setVertexArray(myVerts[0]);
//   	myNode->addDrawable(hgeoms[0].geometry);
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addDrawable(const int count)
{
	for (int i = 0; i < count; ++i) {
		hgeoms.push_back(HGeom());
		hgeoms.back().geometry = new osg::Geometry();
		osg::VertexBufferObject* vboP = hgeoms.back().geometry->getOrCreateVertexBufferObject();
		vboP->setUsage (GL_STREAM_DRAW);

		hgeoms.back().vertices = new osg::Vec3Array();

		hgeoms.back().geometry->setUseDisplayList (false);
		hgeoms.back().geometry->setUseVertexBufferObjects(true);
		hgeoms.back().geometry->setVertexArray(hgeoms.back().vertices);
		myNode->addDrawable(hgeoms.back().geometry);
	}
	return myNode->getNumDrawables();
}


///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addVertex(const Vector3f& v)
{ return HoudiniGeometry::addVertex(v, 0); }
int HoudiniGeometry::addVertex(const Vector3f& v, const int drawableIndex)
{
	oassert(hgeoms.size() > drawableIndex);
	hgeoms[drawableIndex].vertices->push_back(osg::Vec3d(v[0], v[1], v[2]));
	hgeoms[drawableIndex].geometry->dirtyBound();
	return hgeoms[drawableIndex].vertices->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setVertex(int index, const Vector3f& v)
{ return HoudiniGeometry::setVertex(index, v, 0) ; }
void HoudiniGeometry::setVertex(int index, const Vector3f& v, const int drawableIndex)
{
	oassert(hgeoms[drawableIndex].vertices->size() > index);
	osg::Vec3f& c = hgeoms[drawableIndex].vertices->at(index);
	c[0] = v[0];
	c[1] = v[1];
	c[2] = v[2];
	hgeoms[drawableIndex].vertices->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Vector3f HoudiniGeometry::getVertex(int index)
{ return HoudiniGeometry::getVertex(index, 0) ; }
Vector3f HoudiniGeometry::getVertex(int index, const int drawableIndex)
{
	oassert(hgeoms[drawableIndex].vertices->size() > index);
	const osg::Vec3f& v = hgeoms[drawableIndex].vertices->at(index);
	return Vector3f(v[0], v[1], v[2]);
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addColor(const Color& c)
{ return HoudiniGeometry::addColor(c, 0) ; }
int HoudiniGeometry::addColor(const Color& c, const int drawableIndex)
{
	oassert(hgeoms.size() > drawableIndex);
	if(hgeoms[drawableIndex].colors == NULL)
	{
		hgeoms[drawableIndex].colors = new osg::Vec4Array();
		hgeoms[drawableIndex].geometry->setColorArray(hgeoms[drawableIndex].colors);
		hgeoms[drawableIndex].geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
	}
	hgeoms[drawableIndex].colors->push_back(osg::Vec4d(c[0], c[1], c[2], c[3]));
	return hgeoms[drawableIndex].colors->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setColor(int index, const Color& col)
{ return HoudiniGeometry::setColor(index, col, 0) ; }
void HoudiniGeometry::setColor(int index, const Color& col, const int drawableIndex)
{
	oassert(hgeoms[drawableIndex].colors != NULL && hgeoms[drawableIndex].colors->size() > index);
	osg::Vec4f& c = hgeoms[drawableIndex].colors->at(index);
	c[0] = col[0];
	c[1] = col[1];
	c[2] = col[2];
	c[3] = col[3];
	hgeoms[drawableIndex].colors->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Color HoudiniGeometry::getColor(int index)
{ return HoudiniGeometry::getColor(index, 0) ; }
Color HoudiniGeometry::getColor(int index, const int drawableIndex)
{
	oassert(hgeoms[drawableIndex].colors != NULL && hgeoms[drawableIndex].colors->size() > index);
	const osg::Vec4d& c = hgeoms[drawableIndex].colors->at(index);
	return Color(c[0], c[1], c[2], c[3]);
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex)
{ return HoudiniGeometry::addPrimitive(type, startIndex, endIndex, 0) ; }
void HoudiniGeometry::addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex, const int drawableIndex)
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
	hgeoms[drawableIndex].geometry->addPrimitiveSet(new osg::DrawArrays(osgPrimType, startIndex, endIndex));
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex)
{ return HoudiniGeometry::addPrimitiveOsg(type, startIndex, endIndex, 0) ; }
void HoudiniGeometry::addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex, const int drawableIndex)
{
	ofmsg("adding primitive set by given mode: %1% from %2% to %3%", %type %startIndex %endIndex);
	hgeoms[drawableIndex].geometry->addPrimitiveSet(new osg::DrawArrays(type, startIndex, endIndex));
}



///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::clear()
{
	for (int i = 0; i < hgeoms.size(); ++i) {
		if (hgeoms[i].colors != NULL) hgeoms[i].colors->clear();
		if (hgeoms[i].normals != NULL) hgeoms[i].normals->clear();
		hgeoms[i].vertices->clear();
		hgeoms[i].geometry->removePrimitiveSet(0, hgeoms[i].geometry->getNumPrimitiveSets());
		hgeoms[i].geometry->dirtyBound();
	}
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::dirty()
{
	for (int i = 0; i < hgeoms.size(); ++i) {
		if (hgeoms[i].colors != NULL) hgeoms[i].colors->dirty();
		hgeoms[i].vertices->dirty();
		if (hgeoms[i].normals != NULL) hgeoms[i].normals->dirty();
	}
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addNormal(const Vector3f& v)
{ return HoudiniGeometry::addNormal(v, 0) ; }
int HoudiniGeometry::addNormal(const Vector3f& v, const int drawableIndex)
{
	oassert(hgeoms.size() > drawableIndex);
	if(hgeoms[drawableIndex].normals == NULL) {
		hgeoms[drawableIndex].normals = new osg::Vec3Array();
		hgeoms[drawableIndex].geometry->setNormalArray(hgeoms[drawableIndex].normals);
		hgeoms[drawableIndex].geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
	}

	hgeoms[drawableIndex].normals->push_back(osg::Vec3d(v[0], v[1], v[2]));
	return hgeoms[drawableIndex].normals->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setNormal(int index, const Vector3f& v)
{ return HoudiniGeometry::setNormal(index, v, 0) ; }
void HoudiniGeometry::setNormal(int index, const Vector3f& v, const int drawableIndex)
{
	oassert(hgeoms[drawableIndex].normals->size() > index);
	osg::Vec3f& c = hgeoms[drawableIndex].normals->at(index);
	c[0] = v[0];
	c[1] = v[1];
	c[2] = v[2];
	hgeoms[drawableIndex].normals->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Vector3f HoudiniGeometry::getNormal(int index)
{ return HoudiniGeometry::getNormal(index, 0) ; }
Vector3f HoudiniGeometry::getNormal(int index, const int drawableIndex)
{
	oassert(hgeoms[drawableIndex].normals->size() > index);
	const osg::Vec3f& c = hgeoms[drawableIndex].normals->at(index);
	return Vector3f(c[0], c[1], c[2]);
}
