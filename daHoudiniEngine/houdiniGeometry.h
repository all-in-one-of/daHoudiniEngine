#ifndef __HE_HOUDINI_GEOMETRY__
#define __HE_HOUDINI_GEOMETRY__

#include <cyclops/cyclops.h>

using namespace cyclops;

namespace houdiniEngine {
	class HoudiniGeometry : public ModelGeometry
	{
	public:
		static HoudiniGeometry* create(const String& name)
		{
			return new HoudiniGeometry(name);
		}

		int addNormal(const Vector3f& v);
		void setNormal(int index, const Vector3f& v);
		Vector3f getNormal(int index);
		virtual void clear();

		inline int getNormalCount() { return (myNormals == NULL) ? 0 : myNormals->size(); }
		inline int getVertexCount() { return myVertices->size(); }
		inline int getColorCount() { return (myColors == NULL) ? 0 : myColors->size(); }

		inline int getPrimitiveSetCount() { return myGeometry->getPrimitiveSetList().size(); }

		void dirty();

	public:
		HoudiniGeometry(const String& name): ModelGeometry(name) {}

	private:
		Ref<osg::Vec3Array> myNormals;

	};
};

#endif
