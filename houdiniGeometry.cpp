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
	myNode = new osg::Group();

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
		for (int g = 0; g < hobjs[obj].hgeoms.size(); ++g) {
			for (int i = 0; i < hobjs[obj].hgeoms[g].hparts.size(); ++i) {
				if (hobjs[obj].hgeoms[g].hparts[i].colors != NULL) hobjs[obj].hgeoms[g].hparts[i].colors->clear();
				if (hobjs[obj].hgeoms[g].hparts[i].normals != NULL) hobjs[obj].hgeoms[g].hparts[i].normals->clear();
				hobjs[obj].hgeoms[g].hparts[i].vertices->clear();
				hobjs[obj].hgeoms[g].hparts[i].geometry->removePrimitiveSet(0, hobjs[obj].hgeoms[g].hparts[i].geometry->getNumPrimitiveSets());
				hobjs[obj].hgeoms[g].hparts[i].geometry->dirtyBound();
			}
		}
	}
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
