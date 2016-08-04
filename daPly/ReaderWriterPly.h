#pragma once

#include <osgDB/fstream>
#include <osg/Image>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>


///////////////////////////////////////////////////////////////////////////////
//!
//! \class ReaderWriterPLY
//! \brief This is the Reader for the ply file format
//!
//////////////////////////////////////////////////////////////////////////////
class ReaderWriterPLY : public osgDB::ReaderWriter
{
public:
    ReaderWriterPLY()
    {
        supportsExtension("ply","Stanford Triangle Format");
    }

    virtual const char* className() { return "ReaderWriterPLY"; }
	
	// register with Registry to instantiate the above reader/writer.
	// REGISTER_OSGPLUGIN(ply, ReaderWriterPLY)

	///////////////////////////////////////////////////////////////////////////////
	//!
	//! \brief Function which is called when any ply file is requested to load in
	//! \osgDB. Load read ply file and if it successes return the osg::Node
	//!
	///////////////////////////////////////////////////////////////////////////////
    virtual osgDB::ReaderWriter::ReadResult readNode(const std::string& fileName, const osgDB::ReaderWriter::Options*) const;
protected:
};

