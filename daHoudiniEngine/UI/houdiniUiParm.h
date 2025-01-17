#ifndef __HEPARM_H__
#define __HEPARM_H__

// double check these includes
#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>
#include <daHoudiniEngine/houdiniParameter.h>
// #define OMEGA_NO_GL_HEADERS
// #include <omega.h>
// #include <omegaToolkit.h>

namespace houdiniEngine {
    using namespace omega;
    using namespace omegaToolkit;
    using namespace omegaToolkit::ui;

    class HoudiniUiParm : public ReferenceType, public IEventListener
    {

        public:
        HoudiniUiParm(hapi::Parm parm, Container* cont);
        ~HoudiniUiParm();

        virtual void handleEvent(const Event& evt);

        static HoudiniUiParm* create(hapi::Parm parm, Container* cont);

        Container* getContainer() { return baseContainer; };
        void setContainer(Container* cont ) { baseContainer = cont; };
        Container* getContents() { return newCont; };
        void setContents(Container* cont ) { newCont = cont; };

        Label* getLabel() { return myLabel; };
        Label* getParmLabel(const int i) { return parmLabels[i]; };

        hapi::Parm getParm() { return myParm; };
        void setParm(hapi::Parm parm) { myParm = parm; };

        private:

        Container* baseContainer;
        Vector<Label*> parmLabels;
        Label* myLabel;
        // store a copy of the parm
        hapi::Parm myParm;

        // for choice parms
        Container* newCont;
        Container* choiceCont;
        Container* multiParmButtonCont;

        Slider* mySlider;

        static Dictionary<int,HoudiniUiParm* > myUiParms;

    };
    
};
#endif