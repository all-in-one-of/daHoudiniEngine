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
	} HPart;

	typedef struct {
		vector < HPart > hparts;
		Ref<osg::Geode> geode;
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
		int addVertex(const Vector3f& v, const int drawableIndex, const int geodeIndex);

		//! Replaces an existing vertex
		void setVertex(int index, const Vector3f& v);
		void setVertex(int index, const Vector3f& v, const int drawableIndex);
		void setVertex(int index, const Vector3f& v, const int drawableIndex, const int geodeIndex);
		//! Retrieves an existing vertex
		Vector3f getVertex(int index);
		Vector3f getVertex(int index, const int drawableIndex);
		Vector3f getVertex(int index, const int drawableIndex, const int geodeIndex);
		//! Adds a vertex color and return its index. The color will be applied
		//! to the vertex with the same index as this color.
		int addColor(const Color& c);
		int addColor(const Color& c, const int drawableIndex);
		int addColor(const Color& c, const int drawableIndex, const int geodeIndex);

		Color getColor(int index);
		Color getColor(int index, const int drawableIndex);
		Color getColor(int index, const int drawableIndex, const int geodeIndex);
		//! Replaces an existing color
		void setColor(int index, const Color& c);
		void setColor(int index, const Color& c, const int drawableIndex);
		void setColor(int index, const Color& c, const int drawableIndex, const int geodeIndex);

		void setVertexListSize(int size) { setVertexListSize(size, 0); }
		void setVertexListSize(int size, const int drawableIndex) { setVertexListSize(size, drawableIndex, 0); }
		void setVertexListSize(int size, const int drawableIndex, const int geodeIndex) {
			hgeoms[geodeIndex].hparts[drawableIndex].vertices->resize(size);
		};

		//! Adds a primitive set
		void addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex);
		void addPrimitive(ProgramAsset::PrimitiveType type, int startIndex, int endIndex, const int drawableIndex);
		void addPrimitive(
			ProgramAsset::PrimitiveType type,
			int startIndex,
			int endIndex,
			const int drawableIndex,
			const int geodeIndex
		);

		void addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex);
		void addPrimitiveOsg(osg::PrimitiveSet::Mode type, int startIndex, int endIndex, const int drawableIndex);
		void addPrimitiveOsg(
			osg::PrimitiveSet::Mode type,
			int startIndex,
			int endIndex,
			const int drawableIndex,
			const int geodeIndex
		);

		//! Removes all vertices, colors and primitives from this object
		void clear();

		int getDrawableCount() { return getDrawableCount(0); };
		int getDrawableCount(const int geodeIndex) {
			return (myNode == NULL ?
			0 :
			myNode->getChild(geodeIndex)->asGeode()->getNumDrawables());
		};

		int addDrawable(const int count);
		int addDrawable(const int count, const int geodeIndex);

		int addGeode(const int count);

		int getGeodeCount() { return (myNode == NULL ? 0 : myNode->getNumChildren()); };

		int addNormal(const Vector3f& v);
		int addNormal(const Vector3f& v, const int drawableIndex);
		int addNormal(const Vector3f& v, const int drawableIndex, const int geodeIndex);
		void setNormal(int index, const Vector3f& v);
		void setNormal(int index, const Vector3f& v, const int drawableIndex);
		void setNormal(int index, const Vector3f& v, const int drawableIndex, const int geodeIndex);
		Vector3f getNormal(int index);
		Vector3f getNormal(int index, const int drawableIndex);
		Vector3f getNormal(int index, const int drawableIndex, const int geodeIndex);

		inline int getNormalCount() { return getNormalCount(0); }
		inline int getVertexCount() { return getVertexCount(0); }
		inline int getColorCount() { return getColorCount(0); }

		inline int getNormalCount(const int drawableIndex) { return getNormalCount(drawableIndex, 0); }
		inline int getVertexCount(const int drawableIndex) { return getVertexCount(drawableIndex, 0); }
		inline int getColorCount(const int drawableIndex) { return getColorCount(drawableIndex, 0); }

		inline int getNormalCount(const int drawableIndex, const int geodeIndex) {
			return (hgeoms[geodeIndex].hparts[drawableIndex].normals == NULL) ? 0 : hgeoms[geodeIndex].hparts[drawableIndex].normals->size();
		}
		inline int getVertexCount(const int drawableIndex, const int geodeIndex) {
			return hgeoms[geodeIndex].hparts[drawableIndex].vertices->size();
		}
		inline int getColorCount(const int drawableIndex, const int geodeIndex) {
			return (hgeoms[geodeIndex].hparts[drawableIndex].colors == NULL) ? 0 : hgeoms[geodeIndex].hparts[drawableIndex].colors->size();
		}

		inline int getPrimitiveSetCount() { return getPrimitiveSetCount(0); }
		inline int getPrimitiveSetCount(const int drawableIndex) { return getPrimitiveSetCount(drawableIndex, 0); }
		inline int getPrimitiveSetCount(const int drawableIndex, const int geodeIndex) {
			return hgeoms[geodeIndex].hparts[drawableIndex].geometry->getPrimitiveSetList().size();
		}

// 		const String& getName() { return myName; }
		osg::Geode* getOsgNode() { return getOsgNode(0); }
		osg::Geode* getOsgNode(const int geodeIndex) { return myNode->getChild(geodeIndex)->asGeode(); }
		osg::Group* getRootOsgNode() { return myNode; }

		void dirty();

	private:
		vector < HGeom > hgeoms;
		osg::Group* myNode;
	};
};

#endif
