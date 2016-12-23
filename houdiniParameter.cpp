#include <daHoudiniEngine/houdiniParameter.h>

using namespace houdiniEngine;

HoudiniParameter::HoudiniParameter(const int id): ReferenceType(), id(id)
{
    // pass
}

HoudiniParameterList::HoudiniParameterList(): ReferenceType(), parameters()
{
    // pass
}
