#include <daHoudiniEngine/houdiniGeometry.h>

using namespace houdiniEngine;

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::clear()
{
	if (myColors != NULL) myColors->clear();
	if (myVertices != NULL) myVertices->clear();
	if (myNormals != NULL) myNormals->clear();
	myGeometry->removePrimitiveSet(0, myGeometry->getNumPrimitiveSets());
	myGeometry->dirtyBound();
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::dirty()
{
	if (myColors != NULL) myColors->dirty();
	if (myVertices != NULL) myVertices->dirty();
	if (myNormals != NULL) myNormals->dirty();
}

///////////////////////////////////////////////////////////////////////////////
int HoudiniGeometry::addNormal(const Vector3f& v)
{
	if(myNormals == NULL) {
		myNormals = new osg::Vec3Array();
		myGeometry->setNormalArray(myNormals);
		myGeometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
	}

	myNormals->push_back(osg::Vec3d(v[0], v[1], v[2]));
// 	myGeometry->dirtyBound();
	return myNormals->size() - 1;
}

///////////////////////////////////////////////////////////////////////////////
void HoudiniGeometry::setNormal(int index, const Vector3f& v)
{
	oassert(myNormals->size() > index);
	osg::Vec3f& c = myNormals->at(index);
	c[0] = v[0];
	c[1] = v[1];
	c[2] = v[2];
	myNormals->dirty();
}

///////////////////////////////////////////////////////////////////////////////
Vector3f HoudiniGeometry::getNormal(int index)
{
	oassert(myNormals->size() > index);
	const osg::Vec3f& c = myNormals->at(index);
	return Vector3f(c[0], c[1], c[2]);
}
