#include <osgDB/WriteFile>
#include <osgUtil/CullVisitor>
#include <omegaOsg/omegaOsg/SceneView.h>


#include <daHoudiniEngine/loaderTools.h>

using namespace houdiniEngine;


void LoaderTools::registerDAPlyLoader()
{
	// register our custom ply plugin
	osgDB::Registry* reg = osgDB::Registry::instance();

	osg::ref_ptr<ReaderWriterPLY> plyRW = new ReaderWriterPLY();
    reg->addReaderWriter(plyRW);
}


void LoaderTools::createBillboardNodes(Entity* node, std::string nameSubstr)
{
	BillboardMaker bMaker(nameSubstr);

	node->getOsgNode()->accept(bMaker);
	node->getOsgNode()->dirtyBound();

	// culling will cull text nodes nearer than a certain distance nodes for some weird reason
	// disabling culling for individual nodes or resetting the near far plane does not work
    SceneManager::instance()->getEngine()->getDefaultCamera()->setCullingEnabled(false);

}




BillboardMaker::BillboardMaker(std::string nameSubstr)
	: osg::NodeVisitor(	osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _name(nameSubstr)
{

}


// just for debug printing
class CullCallback : public osg::NodeCallback
{
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
        	osg::Group* aut = dynamic_cast<osg::Group*>(node);
        	if (aut != NULL){
        		const osg::BoundingSphere bb = node->getBound();

        		std::cout << bb.radius() << std::endl;
        	}

            traverse(node,nv);
        }
};


// This method gets called for every Group in the scene
//   graph. Check each node to see if its name matches
//   out target. 
void BillboardMaker::apply(osg::Group& group)
{

	ofmsg("group:%1%",%group.getName());

	if (group.getName().find(_name) != std::string::npos)
	{
		

		osg::ref_ptr<osg::AutoTransform> at = new osg::AutoTransform;
	    at->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);

	    //DEBUG
	    // if (group.getName().compare("TOWN_10") == 0)
	    // {
	    // 	group.setCullCallback(new CullCallback());
	    // 	omsg("CullCallback set");
	    // }


		for (int i = 0; i < group.getNumChildren(); i++)
		{
			osg::Node* child = group.getChild(i);

			if (dynamic_cast<osg::Geode*>(child) != NULL)
			{
				at->addChild(child);
				osg::Vec3d origin = shiftVertsToCalculatedOrigin(*dynamic_cast<osg::Geode*>(child));
	    		at->setPosition(origin);				
				group.removeChild(child);
			} 
			else {
				ofwarn("Warning: Child of found Group: %1% must be Geode.. skipping", %group.getName());
			}

		}

		group.addChild(at);
	}

	// Keep traversing the rest of the scene graph.
	traverse(group);
}


// The vertices have to be shifted into a relative frame from their absolut positions
// Use the center of their bounding box as a pivot for rotations and shift all vertices into a frame relative to this point
osg::Vec3d BillboardMaker::shiftVertsToCalculatedOrigin(osg::Geode& geode){
	const osg::Vec3 bbCenter = geode.getBoundingBox().center();

	// Iterate through the drawables.
	for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
	{
		osg::Geometry* geometry = geode.getDrawable(i)->asGeometry();
		if (geometry) // Make sure the object is a Geometry
		{
			osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
			if (vertices) // Make sure it was a Vec3Array
			{
				for (unsigned int j = 0; j < vertices->size(); ++j)
				{
					(*vertices)[j] -= bbCenter;
				}
			}
		}
	}

	geode.dirtyBound();

	return osg::Vec3d( bbCenter );
}

