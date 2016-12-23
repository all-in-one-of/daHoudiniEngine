#ifndef __HE_HOUDINI_PARAMETER__
#define __HE_HOUDINI_PARAMETER__

#define OMEGA_NO_GL_HEADERS
#include <omega.h>

#include <vector>

namespace houdiniEngine {
    using namespace std;
    using namespace omega;

    class HoudiniParameter: public ReferenceType
    {
        public:

            HoudiniParameter(const int id);

            int
            getId() { return id; }

        private:

            int id;
    };

    class HoudiniParameterList: public ReferenceType
    {
        public:

            HoudiniParameterList();

            int
            size() { return parameters.size(); }

            HoudiniParameter
            getParameter(const int index) { return parameters.at(index); }

            void
            addParameter(const HoudiniParameter& parameter) { parameters.push_back(parameter); }

        private:

            vector<HoudiniParameter> parameters;
    };
};

#endif
