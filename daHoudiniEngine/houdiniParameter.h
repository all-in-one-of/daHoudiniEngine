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

            void
            setId(int _id) { id = _id; }

            int
            getParentId() { return parentId; }

            void
            setParentId(int _parentId) { parentId = _parentId; }

            int
            getType() { return type; }

            void
            setType(int _type) { type = _type; }

            int
            getSize() { return size; }

            void
            setSize(int _size) { size = _size; }

            string
            getName() { return name; }

            void
            setName(string _name) { name = _name; }

            string
            getLabel() { return label; }

            void
            setLabel(string _label) { label = _label; }

        private:

            int id;
            int parentId;
            int type;
            int size;
            string name;
            string label;
    };

    class HoudiniParameterList: public ReferenceType
    {
        public:

            HoudiniParameterList();
            
            int
            size() { return parameters.size(); }

            HoudiniParameter*
            getParameter(const int index) { return parameters.at(index); }

            void
            addParameter(HoudiniParameter* parameter) { parameters.push_back(parameter); }

            void
            print();

        private:

            vector<HoudiniParameter*> parameters;
    };
};

#endif
