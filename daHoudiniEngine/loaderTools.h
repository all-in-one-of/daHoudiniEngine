#include <cyclops/cyclops.h>

#define OMEGA_NO_GL_HEADERS
#include <omega.h>
#include <omegaOsg/omegaOsg.h>

#include <osg/AutoTransform>



namespace houdiniEngine {
    using namespace omega;
    using namespace cyclops;
    using namespace omegaToolkit;


    class LoaderTools : public ReferenceType
    {
    public:
        static void createBillboardNodes(Entity* node, std::string nameSubstr);

    protected:
        // static void addCameraFacingTransform(osg::Geode& geode);
    };


    class BillboardMaker : public osg::NodeVisitor
    {
    public:
        BillboardMaker(std::string name);
        
        virtual void apply(osg::Group& node);
        virtual void apply(osg::Transform& node){
            // ofmsg("Transform:%1%",%node.getName());
            traverse(node);
        }
        virtual void apply(osg::Geode& node){
            // ofmsg("Group:%1%",%node.getName());
            traverse(node);
        }


        osg::Vec3d shiftVertsToCalculatedOrigin(osg::Geode& geode);
        std::vector<osg::Geode*>& getBillboardGeodes() { return mybillboardGeodes; }

        
    protected:
        std::string _name;
        std::vector<osg::Geode*> mybillboardGeodes;
    };

}