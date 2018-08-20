#ifndef __HE_HOUDINI_ASSET__
#define __HE_HOUDINI_ASSET__

#include <cyclops/cyclops.h>
#define OMEGA_NO_GL_HEADERS
#include <omega.h>
#include <omegaOsg/omegaOsg.h>
#include <omegaToolkit.h>

namespace houdiniEngine {
    using namespace cyclops;
	using namespace omega;
	using namespace omegaOsg;

	///////////////////////////////////////////////////////////////////////////////////////////////
	//! PYAPI
	class HoudiniAsset : public Entity
	{
	public:
		//! PYAPI Convenience method for creating SphereShape
		static HoudiniAsset* create(const String& modelName);

	public:
		HoudiniAsset(SceneManager* scene, const String& modelName);

		ModelAsset* getModel();

	private:
		ModelAsset* myModel;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////
	inline ModelAsset* HoudiniAsset::getModel()
	{ return myModel; }
};

#endif
