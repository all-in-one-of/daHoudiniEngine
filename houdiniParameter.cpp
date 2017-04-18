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

void
HoudiniParameterList::print()
{
    for (vector<HoudiniParameter*>::iterator i = parameters.begin(); i != parameters.end(); ++i) {
        ofmsg("%1%: %2%", %(*i)->getId() 
                          %(*i)->getName());
    }
}
