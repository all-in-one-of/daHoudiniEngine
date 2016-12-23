#ifndef __HE_HOUDINI_PARAMETER__
#define __HE_HOUDINI_PARAMETER__

#include <vector>

namespace houdiniEngine {
    using namespace std;

    class HoudiniParameter
    {
        public:

            HoudiniParameter(const int id);

            int
            getId() { return id; }

        private:

            int id;
    };

    class HoudiniParameterList
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
