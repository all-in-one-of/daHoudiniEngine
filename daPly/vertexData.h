/*
    vertexData.h
    Copyright (c) 2007, Tobias Wolf <twolf@access.unizh.ch>
    All rights reserved.

    Header file of the VertexData class.
*/

/** note, derived from Equalizer LGPL source.*/


#ifndef MESH_VERTEXDATA_H
#define MESH_VERTEXDATA_H


#include <osg/Node>
#include <osg/PrimitiveSet>

#include <vector>

///////////////////////////////////////////////////////////////////////////////
//!
//! \class VertexData
//! \brief helps to read ply file and converts in to osg::Node format
//!
///////////////////////////////////////////////////////////////////////////////

// defined elsewhere
struct PlyFile;

namespace ply
{
    /*  Holds the flat data and offers routines to read, scale and sort it.  */
    class VertexData
    {
    public:
        // Default constructor
        VertexData(bool shiftVerts = false, bool usePivot = false, bool faceScreen = false);


        // Reads ply file and convert in to osg::Node and returns the same
        osg::Node* readPlyFile( const char* file, const bool ignoreColors = false );

        // to set the flag for using inverted face
        void useInvertedFaces() { _invertFaces = true; }

    private:

        enum VertexFields
        {
          NONE = 0,
          XYZ = 1,
          NORMALS = 2,
          RGB = 4,
          AMBIENT = 8,
          DIFFUSE = 16,
          SPECULAR = 32,
          RGBA = 64,
          OBJECTID = 128, //daHoudiniEngine specific
          PIVOT = 256
        };

        // Function which reads all the vertices and colors if color info is
        // given and also if the user wants that information
        void readVertices( PlyFile* file, const int nVertices,
                           const int vertexFields );

        // Reads the triangle indices from the ply file
        void readTriangles( PlyFile* file, const int nFaces );

        int getNumGroups();

        bool        _invertFaces;


        // Vertex array in osg format
        osg::ref_ptr<osg::Vec3Array>   _vertices;
        // Color array in osg format
        osg::ref_ptr<osg::Vec4Array>   _colors;
        osg::ref_ptr<osg::Vec4Array>   _ambient;
        osg::ref_ptr<osg::Vec4Array>   _diffuse;
        osg::ref_ptr<osg::Vec4Array>   _specular;


        // Normals in osg format
        osg::ref_ptr<osg::Vec3Array> _normals;
        // The indices of the faces in premitive set
        std::vector< osg::ref_ptr<osg::DrawElementsUInt> > _triangles;
        std::vector< osg::ref_ptr<osg::DrawElementsUInt> > _quads;


        ///// DA custom grouping types ///
        //id of the group a vertex is belonging to
        std::vector<int>                _object_ids;
        //offset of group in global vertex list
        std::vector<int>                _group_base_offset;
        // pivot point of a group
        osg::ref_ptr<osg::Vec3Array>   _pivotPerGeode;

        int  _numGroups;
        // indicates, if vertices should be centered around there origin point
        bool _shiftVerts;
        // inidicates if a pivot point, if supplied, should be used
        bool _useSuppliedPivotPoint;

        // make all groups face the screen (follow the camera)
        bool _faceScreen;
    };
}


#endif // MESH_VERTEXDATA_H
