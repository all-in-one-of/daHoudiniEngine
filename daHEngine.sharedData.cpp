/******************************************************************************
Houdini Engine Module for Omegalib

Authors:
  Darren Lee             darren.lee@uts.edu.au

Copyright 2015-2016,     Data Arena, University of Technology Sydney
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and authors, and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the Data Arena Project.

-------------------------------------------------------------------------------

daHEngine
	module to display geometry from Houdini Engine in omegalib
	this file contains the sharing of the HEngine content across the cluster

******************************************************************************/

#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>

using namespace houdiniEngine;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// only run on master
// for each part of each geo of each object of each asset:
// send all the verts, faces, normals, colours, etc
void HoudiniEngine::commitSharedData(SharedOStream& out)
{
	out << updateGeos; // may not be necessary to send this..

	// continue only if there is something to send
	if (!updateGeos) {
		return;
	}

 	updateGeos = false;

 	ofmsg("MASTER: sending %1% assets", %myHoudiniGeometrys.size());

	// TODO change this to count the number of changed objs, geos, parts
	out << int(myHoudiniGeometrys.size());

	int i = 0;

    foreach(HGDictionary::Item hg, myHoudiniGeometrys)
    {
		bool haveObjectsChanged = hg->objectsChanged;
		ofmsg("MASTER: Objects have changed:  %1%", %haveObjectsChanged);
		out << haveObjectsChanged;

		if (!haveObjectsChanged) {
			continue;
		}

		ofmsg("MASTER: Name %1%", %hg->getName());
		out << hg->getName();
		out << hg->getObjectCount();

		// objects
		for (int obj = 0; obj < hg->getObjectCount(); ++obj) {
			bool hasTransformChanged = hg->getTransformChanged(obj);
			ofmsg("MASTER: Transforms have changed:  %1%", %hasTransformChanged);

			out << hasTransformChanged;

			if (hasTransformChanged) {
				osg::Vec3d pos = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getPosition();
				osg::Quat rot = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getAttitude();
				osg::Vec3d scale = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getScale();

				out << pos;
				out << rot;
				out << scale;
			}

			bool haveGeosChanged = hg->getGeosChanged(obj);
			ofmsg("MASTER: Geos have changed:  %1%", %haveGeosChanged);
			out << haveGeosChanged;

			if (!haveGeosChanged) {
				continue;
			}

			ofmsg("Geodes: %1%", %hg->getGeodeCount(obj));
			out << hg->getGeodeCount(obj);

			// geoms
			for (int g = 0; g < hg->getGeodeCount(obj); ++g) {
				bool hasGeoChanged = hg->getGeoChanged(g, obj);
				ofmsg("MASTER: Geo has changed:  %1%", %hasGeoChanged);
				out << hasGeoChanged;

				if (!hasGeoChanged) {
					continue;
				}

				ofmsg("Drawables: %1%", %hg->getDrawableCount(g, obj));
				out << hg->getDrawableCount(g, obj);

				// parts
				for (int d = 0; d < hg->getDrawableCount(g, obj); ++d) {
					ofmsg("Vertex count: %1%", %hg->getVertexCount(d, g, obj));
					out << hg->getVertexCount(d, g, obj);
					for (int i = 0; i < hg->getVertexCount(d, g, obj); ++i) {
						out << hg->getVertex(i, d, g, obj);
					}
					ofmsg("Normal count: %1%", %hg->getNormalCount(d, g, obj));
					out << hg->getNormalCount(d, g, obj);
					for (int i = 0; i < hg->getNormalCount(d, g, obj); ++i) {
						out << hg->getNormal(i, d, g, obj);
					}
					ofmsg("Color count: %1%", %hg->getColorCount(d, g, obj));
					out << hg->getColorCount(d, g, obj);
					for (int i = 0; i < hg->getColorCount(d, g, obj); ++i) {
						out << hg->getColor(i, d, g, obj);
					}
					// faces are done in that primitive set way
					// TODO: simplification: assume all faces are triangles?
					osg::Geometry* geo = hg->getOsgNode()->asGroup()->getChild(obj)->asGroup()->getChild(g)->asGeode()->getDrawable(d)->asGeometry();
					osg::Geometry::PrimitiveSetList psl = geo->getPrimitiveSetList();

					out << int(psl.size());
					for (int i = 0; i < psl.size(); ++i) {
						osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(psl[i].get());

						out << da->getMode() << int(da->getFirst()) << int(da->getCount());
					}
				}
			}
		}
	}

	// distribute the parameter list
	// int parmCount = assetParams.size();
	// disable parm distribution until menu is improved (WIP)
	int parmCount = 0;
	out << parmCount;

	// TODO: reimplement sending of ui
//     foreach(ParmConts::Item pcs, assetParamConts) {
// 		out << pcs.first;
// 		out << int(pcs.second.size());
// 		for (int i = 0; i < pcs.second.size(); ++i) {
// 			MenuItem* mi = &pcs.second[i];
// 			out << mi->getType();
// 			// MenuItem.Type: { Button, Checkbox, Slider, Label, SubMenu, Image, Container }
// 			if (mi->getType() == MenuItem::Label) {
// 				out << mi->getText();
// 			} else if (mi->getType() == MenuItem::Slider) {
// 				out << mi->getSlider()->getTicks();
// 				out << mi->getSlider()->getValue();
// 			} else if (mi->getType() == MenuItem::Button) { // checkbox is actually a button
// 				out << mi->isChecked();
// 			}
// 		}
// 	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// only run on slaves!
void HoudiniEngine::updateSharedData(SharedIStream& in)
{
	in >> updateGeos;

	if (!updateGeos) {
		return;
	}

	// houdiniGeometry count
	int numItems = 0;

	in >> numItems;

	if (!SystemManager::instance()->isMaster()) {
		ofmsg("SLAVE %1%: getting data: %2%", %SystemManager::instance()->getHostname() %numItems);
	}

    for(int i = 0; i < numItems; i++) {

		bool haveObjectsChanged;
		in >> haveObjectsChanged;

		ofmsg("SLAVE: objs have changed: %1%", %haveObjectsChanged);

		if (!haveObjectsChanged) {
			continue;
		}

        String name;
        in >> name;

		ofmsg("SLAVE: name: '%1%'", %name);

 		int objectCount = 0;
        in >> objectCount;

 		ofmsg("SLAVE: obj count: '%1%'", %objectCount);

       HoudiniGeometry* hg = myHoudiniGeometrys[name];
        if(hg == NULL) {
 			ofmsg("SLAVE: no hg: '%1%'", %name);
			hg = HoudiniGeometry::create(name);
			hg->addObject(objectCount);
			myHoudiniGeometrys[name] = hg;
			SceneManager::instance()->addModel(hg);
// 			continue;
// 		} else {
// 			ofmsg("SLAVE: i have the hg: '%1%'", %name);
// 			getHGInfo(name);
// 			ofmsg("SLAVE: end info for '%1%'", %name);
		}

// 		hg->clear();

		ofmsg("SLAVE: current obj count: '%1%'", %hg->getObjectCount());

		if (hg->getObjectCount() < objectCount) {
			hg->addObject(objectCount - hg->getObjectCount());
		}

		ofmsg("SLAVE: new obj count: '%1%'", %hg->getObjectCount());

 		for (int obj = 0; obj < objectCount; ++obj) {
			bool hasTransformChanged;
			in >> hasTransformChanged;

			ofmsg("SLAVE: transforms have changed: '%1%'", %hasTransformChanged);

			if (hasTransformChanged) {
				osg::Vec3d pos;
				in >> pos;

				osg::Quat rot;
				in >> rot;

				osg::Vec3d scale;
				in >> scale;

				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setPosition(pos);
				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setAttitude(rot);
				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setScale(scale);
			}

			bool haveGeosChanged;
			in >> haveGeosChanged;

			ofmsg("SLAVE: geos have changed: '%1%'", %haveGeosChanged);

			if (!haveGeosChanged) {
				continue;
			}

			int geodeCount = 0;
	        in >> geodeCount;

 			ofmsg("SLAVE: geode count: '%1%'", %geodeCount);

			if (hg->getGeodeCount(obj) < geodeCount) {
				hg->addGeode(geodeCount - hg->getGeodeCount(obj), obj);
			}

			for (int g = 0; g < geodeCount; ++g) {

				bool hasGeoChanged;
				in >> hasGeoChanged;

				if (!hasGeoChanged) {
					continue;
				}

				int drawableCount = 0;
		        in >> drawableCount;

 				ofmsg("SLAVE: drawable count: '%1%'", %drawableCount);

				// add drawables if needed
				if (hg->getDrawableCount(g, obj) < drawableCount) {
					hg->addDrawable(drawableCount - hg->getDrawableCount(g, obj), g, obj);
				}

				hg->clearGeode(g, obj);

				for (int d = 0; d < drawableCount; ++d) {

					int vertCount = 0;
					in >> vertCount;

					ofmsg("SLAVE: vertex count: '%1%'", %vertCount);
					for (int j = 0; j < vertCount; ++j) {
						Vector3f v;
						in >> v;
						hg->addVertex(v, d, g, obj);
					}

					int normalCount = 0;
					in >> normalCount;

					ofmsg("SLAVE: normal count: '%1%'", %normalCount);
					for (int j = 0; j < normalCount; ++j) {
						Vector3f n;
						in >> n;
						hg->addNormal(n, d, g, obj);
					}

					int colorCount = 0;
					in >> colorCount;

					ofmsg("SLAVE: color count: '%1%'", %colorCount);
					for (int j = 0; j < colorCount; ++j) {
						Color c;
						in >> c;
						hg->addColor(c, d, g, obj);
					}

					// primitive set count
					int psCount = 0;
					in >> psCount;

					ofmsg("SLAVE: primitive set count: '%1%'", %psCount);

					for (int j = 0; j < psCount; ++j) {
						osg::PrimitiveSet::Mode mode;
						int startIndex, count;
						in >> mode >> startIndex >> count;
						hg->addPrimitiveOsg(mode, startIndex, count, d, g, obj);
					}
				}
			}
		}

		hg->dirty();
    }

	// read in menu parms
	int parmCount = 0;
	in >> parmCount;

	//  parameter list
	// TODO: reimplement reading of new ui layout

	// ofmsg("SLAVE: currently have %1% asset parameter lists", %assetParamConts.size());
    // foreach(ParmConts::Item mis, assetParamConts)
	// {
	// 	ofmsg("SLAVE: name %1%", %mis.first);
	// 	ofmsg("SLAVE: number %1%", %mis.second.size());
	// 	for (int i = 0; i < mis.second.size(); ++i) {
	// 		ofmsg("SLAVE: menu item %1%", %mis.second[i]);
	// 	}
	// }
	// ofmsg("SLAVE: received %1% asset parameter lists", %parmCount);

}

