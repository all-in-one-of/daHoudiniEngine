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
	out << updateGeos; // TODO: may not be necessary to send this..

	// continue only if there is something to send
	if (!updateGeos) {
		return;
	}

 	updateGeos = false;

 	hflog("[HoudiniEngine::MASTER] sending %1% assets", %myHoudiniGeometrys.size());

	out << int(myHoudiniGeometrys.size());

    foreach(HGDictionary::Item hg, myHoudiniGeometrys)
    {
		bool haveObjectsChanged = hg->objectsChanged;
		hflog("[HoudiniEngine::MASTER] Objects have changed:  %1%", %haveObjectsChanged);
		out << haveObjectsChanged;

		// still need to traverse this, as an object may not change, but geos in it can change
		// if (!haveObjectsChanged) {
		// 	continue;
		// }

		hflog("[HoudiniEngine::MASTER] Name %1%", %hg->getName());
		out << hg->getName();
		hflog("[HoudiniEngine::MASTER] Object Count %1%", %hg->getObjectCount());
		out << hg->getObjectCount();

		// objects
		for (int obj = 0; obj < hg->getObjectCount(); ++obj) {
			// bool hasTransformChanged = hg->getTransformChanged(obj);
			// always do this..
			bool hasTransformChanged = true;
			hflog("[HoudiniEngine::MASTER] Object %1%: Transforms have changed:  %2%", %obj %hasTransformChanged);

			out << hasTransformChanged;

			if (hasTransformChanged) {
				osg::Vec3d pos = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getPosition();
				osg::Quat rot = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getAttitude();
				osg::Vec3d scale = hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->getScale();

				out << pos[0] << pos[1] << pos[2];
				out << rot[0] << rot[1] << rot[2] << rot[3];
				out << scale[0] << scale[1] << scale[2];
			}

			bool haveGeosChanged = hg->getGeosChanged(obj);
			hflog("[HoudiniEngine::MASTER] Object %1% Geos have changed:  %2%", %obj %haveGeosChanged);
			out << haveGeosChanged;

			if (!haveGeosChanged) {
				continue;
			}

			hflog("[HoudiniEngine::MASTER] Object %1% Geodes: %2%", %obj %hg->getGeodeCount(obj));
			out << hg->getGeodeCount(obj);

			// geoms
			for (int g = 0; g < hg->getGeodeCount(obj); ++g) {
				bool hasGeoChanged = hg->getGeoChanged(g, obj);
				hflog("[HoudiniEngine::MASTER] Object %1% Geo %2% has changed:  %3%", %obj %g %hasGeoChanged);
				out << hasGeoChanged;

				if (!hasGeoChanged) {
					continue;
				}

				hflog("[HoudiniEngine::MASTER] Object %1% Geo %2% Drawables: %3%", %obj %g %hg->getDrawableCount(g, obj));
				out << hg->getDrawableCount(g, obj);

				// parts
				for (int d = 0; d < hg->getDrawableCount(g, obj); ++d) {
					hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% Vertex count: %4%",
						%obj %g %d
						%hg->getVertexCount(d, g, obj));
					out << hg->getVertexCount(d, g, obj);
					for (int i = 0; i < hg->getVertexCount(d, g, obj); ++i) {
						out << hg->getVertex(i, d, g, obj);
					}
					hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% Normal count: %4%",
						%obj %g %d
						%hg->getNormalCount(d, g, obj));
					out << hg->getNormalCount(d, g, obj);
					for (int i = 0; i < hg->getNormalCount(d, g, obj); ++i) {
						out << hg->getNormal(i, d, g, obj);
					}
					hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% Color count: %4%",
						%obj %g %d
						%hg->getColorCount(d, g, obj));
					out << hg->getColorCount(d, g, obj);
					for (int i = 0; i < hg->getColorCount(d, g, obj); ++i) {
						out << hg->getColor(i, d, g, obj);
					}
					hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% UV count: %4%",
						%obj %g %d
						%hg->getUVCount(d, g, obj));
					out << hg->getUVCount(d, g, obj);
					for (int i = 0; i < hg->getUVCount(d, g, obj); ++i) {
						out << hg->getUV(i, d, g, obj);
					}
					// faces are done in that primitive set way
					// TODO: simplification: assume all faces are triangles?
					osg::Geometry* geo = hg->getOsgNode()->asGroup()->getChild(obj)->asGroup()->getChild(g)->asGeode()->getDrawable(d)->asGeometry();
					osg::Geometry::PrimitiveSetList psl = geo->getPrimitiveSetList();

					hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% Primitive Set count: %4%",
						%obj %g %d %psl.size());
					out << int(psl.size());
					for (int i = 0; i < psl.size(); ++i) {
						osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(psl[i].get());

						hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% ps%4%: %5% %6% %7%",
							%obj %g %d %i
							%da->getMode()
							%da->getFirst()
							%da->getCount()
						);
						out << da->getMode() << int(da->getFirst()) << int(da->getCount());
					}
					hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% Mat Id: %4%",
						%obj %g %d
						%hg->getMatId(d, g, obj));
					out << hg->getMatId(d, g, obj);

					hflog("[HoudiniEngine::MASTER] O%1%G%2% D%3% %4%",
						%obj %g %d
						%(hg->isTransparent(d, g, obj) ? "Transparent" : "Opaque"));
					out << hg->isTransparent(d, g, obj);
				}
			}
		}
	}

	typedef Dictionary <String, ParmStruct > PS;
	typedef Dictionary < String, Vector< MatStruct > > Amps;

	// distribute materials
 	hlog("[HoudiniEngine::MASTER] sending material info");

	int assetCount = int(assetMaterialParms.size());

 	hflog("[HoudiniEngine::MASTER] sending materials of %1% assets", %assetCount);
	out << assetCount;

    foreach(Amps::Item amp, assetMaterialParms) {
		hflog("[HoudiniEngine::MASTER] Material parms for asset Name %1%", %amp.first);
		out << amp.first;
		int assetMatCount = int(amp.second.size());
		out << assetMatCount;
		hflog("[HoudiniEngine::MASTER] sending %1% materials under %2%", %assetMatCount %amp.first);
		for (int i = 0; i < assetMatCount; ++i) {
			MatStruct* ms = &amp.second[i];
			hflog("[HoudiniEngine::MASTER] Material id for %1%: %2%", %amp.first %ms->matId);
			out << ms->matId;
			hflog("[HoudiniEngine::MASTER] Part id for %1%: %2%", %amp.first %ms->partId);
			out << ms->partId;
			hflog("[HoudiniEngine::MASTER] Geo id for %1%: %2%", %amp.first %ms->geoId);
			out << ms->geoId;
			hflog("[HoudiniEngine::MASTER] Object id for %1%: %2%", %amp.first %ms->objId);
			out << ms->objId;
			int myParmCount = ms->parms.size();
			hflog("[HoudiniEngine::MASTER] Parm count for %1%: %2%", %amp.first %myParmCount);
			out << myParmCount;
			foreach(PS::Item mp, amp.second[i].parms) {
				hflog("[HoudiniEngine::MASTER] Material parm name %1%", %mp.first);
				out << mp.first;
				hflog("[HoudiniEngine::MASTER]   ParmStruct type %1%", %mp.second.type);
				out << mp.second.type;

				hflog("[HoudiniEngine::MASTER]   ParmStruct type int value count: %1%", %mp.second.intValues.size());
				out << int(mp.second.intValues.size());
				for (int i = 0; i < mp.second.intValues.size(); ++i) {
					hflog("[HoudiniEngine::MASTER]   int value %1%: %2%", %i %mp.second.intValues[i]);
					out << mp.second.intValues[i];
				}

				hflog("[HoudiniEngine::MASTER]   ParmStruct type float value count: %1%", %mp.second.floatValues.size());
				out << int(mp.second.floatValues.size());
				for (int i = 0; i < mp.second.floatValues.size(); ++i) {
					hflog("[HoudiniEngine::MASTER]   float value %1%: %2%", %i %mp.second.floatValues[i]);
					out << mp.second.floatValues[i];
				}

				hflog("[HoudiniEngine::MASTER]   ParmStruct type str value count: %1%", %mp.second.stringValues.size());
				out << int(mp.second.stringValues.size());
				for (int i = 0; i < mp.second.stringValues.size(); ++i) {
					hflog("[HoudiniEngine::MASTER]   str value %1%: %2%", %i %mp.second.stringValues[i]);
					out << mp.second.stringValues[i];
				}


				// textures are shared based on value 'diffuseMapName' and 'normalMapName'
				// change this to something more sensible.
				// maybe just send all the parm values directly?!
				// ie foreach parm, send the type, string, int and float values.
				// then, reassemble on other side, picking out only what i need?
				// yeah, lets try that..

				// share the textures properly in another loop
				// fetch the texture from the sceneManager and send this across
				/*
				Ref<PixelData> pd = PixelData::create(32, 32, PixelData::FormatRgba);
				pd->copyFrom(mySceneManager->getPixelData(mp.second));
				hflog("[HoudiniEngine::MASTER] Sending pixel data info: w %1% h %2%", %pd->getWidth() %pd->getHeight());
				out << pd->getWidth() << pd->getHeight();
				hflog("[HoudiniEngine::MASTER] Sending pixel data size %1%", %pd->getSize());
				out << pd->getSize();
				hflog("[HoudiniEngine::MASTER] Sending pixel data for %1%", %mp.second);
				out.write(pd->map(), pd->getSize());
				pd->unmap();
				*/
			}
		}
	}
	hlog("[HoudiniEngine::MASTER] end commitSharedData");

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
		hflog("[HoudiniEngine::SLAVE] %1%: getting data: %2%", %SystemManager::instance()->getHostname() %numItems);
	}

    for(int i = 0; i < numItems; i++) {

		bool haveObjectsChanged;
		in >> haveObjectsChanged;

		hflog("[HoudiniEngine::SLAVE] objs have changed: %1%", %haveObjectsChanged);

		// still need to traverse this, as an object may not change, but geos in it can change
		// if (!haveObjectsChanged) {
		// 	continue;
		// }

        String name;
        in >> name;

		hflog("[HoudiniEngine::SLAVE] name: '%1%'", %name);

 		int objectCount = 0;
        in >> objectCount;

 		hflog("[HoudiniEngine::SLAVE] obj count: '%1%'", %objectCount);

       HoudiniGeometry* hg = myHoudiniGeometrys[name];
        if(hg == NULL) {
 			hflog("[HoudiniEngine::SLAVE] no hg: '%1%'", %name);
			hg = HoudiniGeometry::create(name);
			hg->addObject(objectCount);
			myHoudiniGeometrys[name] = hg;
			mySceneManager->addModel(hg);
		// 			continue;
		// 		} else {
		// 			hflog("[HoudiniEngine::SLAVE] i have the hg: '%1%'", %name);
		// 			getHGInfo(name);
		// 			hflog("[HoudiniEngine::SLAVE] end info for '%1%'", %name);
				}

		// 		hg->clear();

		hflog("[HoudiniEngine::SLAVE] current obj count: '%1%'", %hg->getObjectCount());

		if (hg->getObjectCount() < objectCount) {
			hg->addObject(objectCount - hg->getObjectCount());
		}

		hflog("[HoudiniEngine::SLAVE] new obj count: '%1%'", %hg->getObjectCount());

 		for (int obj = 0; obj < objectCount; ++obj) {
			bool hasTransformChanged;
			in >> hasTransformChanged;

			hflog("[HoudiniEngine::SLAVE] transforms have changed: '%1%'", %hasTransformChanged);

			if (hasTransformChanged) {
				osg::Vec3d pos;
				in >> pos[0] >> pos[1] >> pos[2];

				osg::Quat rot;
				in >> rot[0] >> rot[1] >> rot[2] >> rot[3];

				osg::Vec3d scale;
				in >> scale[0] >> scale[1] >> scale[2];

				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setPosition(pos);
				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setAttitude(rot);
				hg->getOsgNode()->asGroup()->getChild(obj)->asTransform()->
					asPositionAttitudeTransform()->setScale(scale);
			}

			bool haveGeosChanged;
			in >> haveGeosChanged;

			hflog("[HoudiniEngine::SLAVE] geos have changed: '%1%'", %haveGeosChanged);

			if (!haveGeosChanged) {
				continue;
			}

			int geodeCount = 0;
	        in >> geodeCount;

 			hflog("[HoudiniEngine::SLAVE] geode count: '%1%'", %geodeCount);

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

 				hflog("[HoudiniEngine::SLAVE] drawable count: '%1%'", %drawableCount);

				// add drawables if needed
				if (hg->getDrawableCount(g, obj) < drawableCount) {
					hg->addDrawable(drawableCount - hg->getDrawableCount(g, obj), g, obj);
				}

				hg->clearGeode(g, obj);

				for (int d = 0; d < drawableCount; ++d) {

					int vertCount = 0;
					in >> vertCount;

					hflog("[HoudiniEngine::SLAVE] vertex count: '%1%'", %vertCount);
					for (int j = 0; j < vertCount; ++j) {
						Vector3f v;
						in >> v;
						hg->addVertex(v, d, g, obj);
					}

					int normalCount = 0;
					in >> normalCount;

					hflog("[HoudiniEngine::SLAVE] normal count: '%1%'", %normalCount);
					for (int j = 0; j < normalCount; ++j) {
						Vector3f n;
						in >> n;
						hg->addNormal(n, d, g, obj);
					}

					int colorCount = 0;
					in >> colorCount;

					hflog("[HoudiniEngine::SLAVE] color count: '%1%'", %colorCount);
					for (int j = 0; j < colorCount; ++j) {
						Color c;
						in >> c;
						hg->addColor(c, d, g, obj);
					}

					int uvCount = 0;
					in >> uvCount;

					hflog("[HoudiniEngine::SLAVE] uv count: '%1%'", %uvCount);
					for (int j = 0; j < uvCount; ++j) {
						Vector3f uv;
						in >> uv;
						hg->addUV(uv, d, g, obj);
					}

					// primitive set count
					int psCount = 0;
					in >> psCount;

					hflog("[HoudiniEngine::SLAVE] primitive set count: '%1%'", %psCount);

					for (int j = 0; j < psCount; ++j) {
						osg::PrimitiveSet::Mode mode;
						int startIndex, count;
						in >> mode >> startIndex >> count;
						hflog("[HoudiniEngine::SLAVE]   ps%1%: %2% %3% %4%", %j %mode %startIndex %count);
						hg->addPrimitiveOsg(mode, startIndex, count, d, g, obj);
					}

					int matId = 0;
					in >> matId;
					hflog("[HoudiniEngine::SLAVE] setting matId to  %1% for D%2% G%3% O%4%", %matId %d %g %obj);
					hg->setMatId(matId, d, g, obj);

					bool transparent = false;
					in >> transparent;
					hflog("[HoudiniEngine::SLAVE] setting part D%2% G%3% O%4% to %1%",
						%(transparent ? "TRANSPARENT" : "OPAQUE") %d %g %obj);
					hg->setTransparent(transparent, d, g, obj);

					// set transparency rendering hint
					osg::StateSet* ss = hg->getOsgNode(g, obj)->getDrawable(d)->getOrCreateStateSet();
					if (transparent) {
						ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
						ss->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED |
						osg::StateAttribute::OVERRIDE);
					} else {
						ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
						ss->setMode(GL_BLEND, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED |
						osg::StateAttribute::OVERRIDE);
					}


				}
			}
		}

		hg->dirty();
    }

	hlog("[HoudiniEngine::SLAVE] done reading geometry, about to read materials");

	// read in materials
	int matCount = 0;
	in >> matCount;
	hflog("[HoudiniEngine::SLAVE] reading %1% materials", %matCount);

	// lets clobber it if there any mats
	if (matCount > 0) {
		hflog("[HoudiniEngine::SLAVE] clearing assetMaterialParms as %1% materials incoming", %matCount);
		assetMaterialParms.clear();
	}

	for (int i = 0; i < matCount; ++i) {
		String matName;
		in >> matName;
		hflog("[HoudiniEngine::SLAVE] reading material parms for %1%", %matName);
		int assetMatCount;
		in >> assetMatCount;
		hflog("[HoudiniEngine::SLAVE] reading %1% materials.. under asset %2%", %assetMatCount %matName);
		assetMaterialParms[matName] = Vector< MatStruct >();
		for (int j = 0; j < assetMatCount; ++j) {
			MatStruct ms;
			int matId;
			in >> matId;
			ms.matId = matId;
			hflog("[HoudiniEngine::SLAVE] read matId: %1%", %matId);
			int partId;
			in >> partId;
			ms.partId = partId;
			hflog("[HoudiniEngine::SLAVE] read partId: %1%", %partId);
			int geoId;
			in >> geoId;
			ms.geoId = geoId;
			hflog("[HoudiniEngine::SLAVE] read geoId: %1%", %geoId);
			int objId;
			in >> objId;
			ms.objId = objId;
			hflog("[HoudiniEngine::SLAVE] read objId: %1%", %objId);

			int matParmCount;
			in >> matParmCount;
			hflog("[HoudiniEngine::SLAVE] reading %1% parms for %2%:%3%", %matParmCount %matName %matId);
			for (int k = 0; k < matParmCount; ++k) {
				String parm, val;
				int parmType;
				in >> parm >> parmType;
				hflog("[HoudiniEngine::SLAVE] read in parm name %1% type '%2%'", %parm %parmType);
				int intValsCount;
				in >> intValsCount;
				for (int v = 0; v < intValsCount; ++v) {
					int val;
					in >> val;
					ms.parms[parm].intValues.push_back(val);
					hflog("[HoudiniEngine::SLAVE] read int: %1%", %val);
				}

				int floatValsCount;
				in >> floatValsCount;
				for (int v = 0; v < floatValsCount; ++v) {
					float val;
					in >> val;
					ms.parms[parm].floatValues.push_back(val);
					hflog("[HoudiniEngine::SLAVE] read float: %1%", %val);
				}

				int stringValsCount;
				in >> stringValsCount;
				for (int v = 0; v < stringValsCount; ++v) {
					String val;
					in >> val;
					ms.parms[parm].stringValues.push_back(val);
					hflog("[HoudiniEngine::SLAVE] read string: %1%", %val);
				}

				/*
				hflog("[HoudiniEngine::SLAVE] read in Pixel Data for %1%", %val);
				int w, h;
				size_t size;
				in >> w >> h;
				in >> size;
				hflog("[HoudiniEngine::SLAVE] read in Pixel Data w %1% h %2% and size %3%", %w %h %size);
				Ref<PixelData> pd = PixelData::create(w, h, PixelData::FormatRgba);
				byte* imgptr = pd->map();
				in.read(imgptr, size);
				pd->unmap();
				pd->setDirty(true);
				osg::Texture2D* texture = mySceneManager->createTexture(val, pd);
				osg::Texture::WrapMode textureWrapMode;
				textureWrapMode = osg::Texture::REPEAT;

				texture->setWrap(osg::Texture2D::WRAP_R, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_S, textureWrapMode);
				texture->setWrap(osg::Texture2D::WRAP_T, textureWrapMode);

				hflog("[HoudiniEngine::SLAVE] (Hopefully) setting material parm %1% to %2% for asset %3%",
					%parm
					%val
					%matName
				);
				// todo: move this somewhere more sensible
				if (parm == "diffuseMapName") {
					hlog("[HoudiniEngine::SLAVE] setting diffuse map");
					assetInstances[matName]->setEffect("houdini -d white");
					assetInstances[matName]->getMaterial()->setDiffuseTexture(val);
				}
				if (parm == "normalMapName") {
					hlog("[HoudiniEngine::SLAVE] setting normal map");
					assetInstances[matName]->setEffect("houdini -d white");
					assetInstances[matName]->getMaterial()->setNormalTexture(val);
				}
				*/
			}
			hflog("[HoudiniEngine::SLAVE] about to add %1% to assetMaterialParms", %matName);
			assetMaterialParms[matName].push_back(ms);
			hflog("[HoudiniEngine::SLAVE] added %1% to assetMaterialParms", %matName);
		}

		hflog("[HoudiniEngine::SLAVE] about to apply material parms on %1%", %matName);
		// TODO: this should be its own method..
		if (assetInstances.count(matName)) {
			hflog("[HoudiniEngine::SLAVE] applying parms to asset instance %1%", %matName);

			// this should work as there is already an assetInstance
			HoudiniGeometry* hg = myHoudiniGeometrys[matName];

			// apply materials
			for (int j = 0; j < assetMaterialParms[matName].size(); ++j) {
				MatStruct* ms = &(assetMaterialParms[matName][j]);

				// look through all drawables in the hg and match against matIds
				for (int o = 0; o < hg->getObjectCount(); ++o) {
					for (int g = 0; g < hg->getGeodeCount(o); ++g) {
						for (int d = 0; d < hg->getDrawableCount(g, o); ++d) {
							hflog("[HoudiniEngine::SLAVE] compare ms->matId %1% D%2% G%3% O%4% matId %5%",
								%ms->matId %d %g %o %hg->getMatId(d, g, o));
							if (hg->getMatId(d, g, o) == ms->matId) {
								hflog("[HoudiniEngine::SLAVE] found partId D%1% G%2% O%3%",
									%d %g %o
								);

								osg::StateSet* ss =  hg->getOsgNode(g, o)->getDrawable(d)->getOrCreateStateSet();
								// NB: This overwrites shader info set elsewhere so ignoring it for now..
								// if i have ambient..
								// if (ms->parms.count("ogl_amb")) {
								// 	Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
								// 	if (mat == NULL) {
								// 		mat = new osg::Material();
								// 	}
								// 	mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(
								// 		ms->parms["ogl_amb"].floatValues[0],
								// 		ms->parms["ogl_amb"].floatValues[1],
								// 		ms->parms["ogl_amb"].floatValues[2],
								// 		1.0
								// 	));
								// 	ss->setAttributeAndModes(mat,
								// 		osg::StateAttribute::ON | osg::StateAttribute::PROTECTED |
								// 		osg::StateAttribute::OVERRIDE);

								// 	hflog("[HoudiniEngine::SLAVE] applied ogl_amb to %1%:part %2%", %matName %ms->partId);
								// }
								// if i have emit..
								if (ms->parms.count("ogl_emit")) {
									Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
									if (mat == NULL) {
										mat = new osg::Material();
									}
									mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(
										ms->parms["ogl_emit"].floatValues[0],
										ms->parms["ogl_emit"].floatValues[1],
										ms->parms["ogl_emit"].floatValues[2],
										1.0
									));
									ss->setAttributeAndModes(mat,
										osg::StateAttribute::ON | osg::StateAttribute::PROTECTED |
										osg::StateAttribute::OVERRIDE);

									hflog("[HoudiniEngine::SLAVE] applied ogl_emit to %1%:part %2%", %matName %ms->partId);
								}
								// if i have diffuse..
								if (ms->parms.count("ogl_diff")) {
									Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
									if (mat == NULL) {
										mat = new osg::Material();
									}
									mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(
										ms->parms["ogl_diff"].floatValues[0],
										ms->parms["ogl_diff"].floatValues[1],
										ms->parms["ogl_diff"].floatValues[2],
										1.0
									));
									ss->setAttributeAndModes(mat,
										osg::StateAttribute::ON | osg::StateAttribute::PROTECTED |
										osg::StateAttribute::OVERRIDE);

									hflog("[HoudiniEngine::SLAVE] applied ogl_diff to %1%:part %2%", %matName %ms->partId);
								}
								// NB: This overwrites shader info set elsewhere so ignoring it for now..
								// // if i have spec..
								// if (ms->parms.count("ogl_spec")) {
								// 	Ref<osg::Material> mat = static_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
								// 	if (mat == NULL) {
								// 		mat = new osg::Material();
								// 	}
								// 	mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(
								// 		ms->parms["ogl_spec"].floatValues[0],
								// 		ms->parms["ogl_spec"].floatValues[1],
								// 		ms->parms["ogl_spec"].floatValues[2],
								// 		1.0
								// 	));
								// 	ss->setAttributeAndModes(mat,
								// 		osg::StateAttribute::ON | osg::StateAttribute::PROTECTED |
								// 		osg::StateAttribute::OVERRIDE);

								// 	hflog("[HoudiniEngine::SLAVE] applied ogl_spec to %1%:part %2%", %matName %ms->partId);
								// }
								// if i have diffuse..
								if (ms->parms.count("ogl_alpha")) {
									// update the state set for this attribute
									if (assetInstances.count(hg->getName()) > 0) {
										const string name = "unif_alpha";
										osg::Uniform* u =  ss->getOrCreateUniform(name, osg::Uniform::FLOAT, 1);
										u->set(ms->parms["ogl_apha"].floatValues[0]);

										ss->getUniformList()[name].second = osg::StateAttribute::ON | osg::StateAttribute::PROTECTED |
											osg::StateAttribute::OVERRIDE;

										// set as transparent
										if (ms->parms["ogl_apha"].floatValues[0] < 0.95) {
											ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
											ss->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED |
											osg::StateAttribute::OVERRIDE);
										} else {
											ss->setRenderingHint(osg::StateSet::OPAQUE_BIN);
											ss->setMode(GL_BLEND, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED |
											osg::StateAttribute::OVERRIDE);
										}
									}

									hflog("[HoudiniEngine::SLAVE] applied ogl_alpha to %1%:part %2%", %matName %ms->partId);
								}
							}
						}
					}
				}
			}
			hflog("[HoudiniEngine::SLAVE] finished applying material parms on %1%", %matName);
		} else {
			hflog("[HoudiniEngine::SLAVE] no %1% asset instance", %matName);
		}

	}
	hlog("[HoudiniEngine::SLAVE] end updateSharedData");
}

