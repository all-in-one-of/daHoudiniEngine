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
	this file contains menu creation and linking to Houdini Asset Parameters

******************************************************************************/

#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>
#include <daHoudiniEngine/houdiniParameter.h>
#include <daHoudiniEngine/UI/houdiniUiParm.h>

using namespace houdiniEngine;

// only run on master
// identical-looking menus get created on slaves
// a base assetCont container is created, and this is then pushed onto assetConts
void HoudiniEngine::createMenu(const int asset_id)
{
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);
	std::string asset_name = myAsset->name();

	if (myAsset == NULL) {
		ofwarn("[HoudiniEngine::createMenu] No asset of name %1%", %asset_name);
	}

	// button to choose this asset
	Button* assetButton = Button::create(assetChoiceCont);
	assetButton->setRadio(true);
	assetButton->setChecked(true);
	assetButton->setText(asset_name);
	assetButton->setName(ostr("Asset %1%", %asset_name)); // TODO: use number here instead..
	assetButton->setUIEventHandler(this);

	// all params go on an assetCont container
	// layout should be justified somehow
	Container* assetCont = Container::create(Container::LayoutHorizontal, stagingCont);
	assetCont->setHorizontalAlign(Container::AlignCenter);
	assetCont->setVerticalAlign(Container::AlignTop);
	assetCont->setFillColor(Color("#806040"));
	assetCont->setPosition(Vector2f(50, 50));
	Label* assetLabel = Label::create(assetCont);
	assetLabel->setText(ostr("AssetCont %1%", %asset_name));
	assetConts.push_back(assetCont);

	int assetContSize = assetConts.size() - 1;
	// cast to an int that is the same size as void*
	assetButton->setUserData((void *)(intptr_t)assetContSize); // point to the container to use in assetConts

    // no longer create parms here
    // TODO: add distributing parms code
    // createParms(asset_id, assetCont);
}

void HoudiniEngine::createParms(const int asset_id, Container* assetCont)
{

	// load only the params in the DA Folder
	// first find the index of the folder, then iterate through the list of params
	// from there onwards
	const char* daFolderName = "daFolder_0";

    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);
	std::map<std::string, hapi::Parm> parmMap = myAsset->parmMap();
	std::vector<hapi::Parm> parms = myAsset->parms();

	// find the DA_Param section
	int daFolderIndex = 0;
	if (parmMap.count(daFolderName) > 0) {
		HAPI_ParmId daFolderId = parmMap[daFolderName].info().id;
		while (daFolderIndex < parms.size()) {
			hapi::Parm* parm = &parms[daFolderIndex];
			ofmsg("(before) PARM %1%: %2%", %parm->label() %parm->info().type);
			if (daFolderId == parms[daFolderIndex].info().id) {
				break;
			}
			daFolderIndex ++;
		}
	}

// 	int i = daFolderIndex; // skipping this for now
	int i = 0;

	// help to parse params:
	// 1 - Parameters such as HAPI_PARMTYPE_FOLDERLIST's and HAPI_PARMTYPE_FOLDER's
	// 	have a child count associated with them. The child count is stored as the
	// 	int value of these parameters.
	// 2 - HAPI_PARMTYPE_FOLDERLIST's can only contain folders. HAPI_PARMTYPE_FOLDER's
	// 	cannot contain other HAPI_PARMTYPE_FOLDER's, only HAPI_PARMTYPE_FOLDERLIST's
	// 	and regular parameters.
	// 3 - When a HAPI_PARMTYPE_FOLDERLIST is encountered, we should dive into its
	// 	contents immediately, while everything else is traversed in a breadth first manner.

	Container* target = assetCont;

	Widget* prevWidget = NULL;

	// NOTE: HAPI_PARMTYPE_FOLDERLIST defines the parent for the following HAPI_PARMTYPE_FOLDER parms
    int lastFolderListIndex = 0;
	while (i < parms.size()) {
		hapi::Parm* parm = &parms[i];
		HoudiniUiParm* uip;

        // make folders point to this folderlist
        if (parm->info().type == HAPI_PARMTYPE_FOLDERLIST) {
            lastFolderListIndex = i;
        }

        // find parent parameter
        if (parm->info().type == HAPI_PARMTYPE_FOLDER) {
            uip = (HoudiniUiParm*) uiParms[asset_id][lastFolderListIndex].get();
            target = uip->getContents();
        } else if (parm->info().parentId == -1) {
			target = assetCont;
		} else {
			for (int j = 0; j < uiParms[asset_id].size(); ++j) {
                // can't forward reference nested class HoudiniUiParm, so this is current
                // workaround
                uip = (HoudiniUiParm*) uiParms[asset_id][j].get();
				if (uip->getParm().info().id == parm->info().parentId) {
					target = uip->getContents();
					break;
				}
			}
		}

        // ui navigation
		uip = HoudiniUiParm::create(*parm, target);
        Widget* thisWidget = uip->getContents()->getChildByIndex(uip->getContents()->getNumChildren() - 1);
        if (thisWidget != NULL) {
            thisWidget->setVerticalPrevWidget(prevWidget);
        }
        if (prevWidget != NULL) {
            prevWidget->setVerticalNextWidget(thisWidget);
        }

		// this will not work on a HoudiniUiParm that isn't part of a folder?
		uip->getContents()->setUIEventHandler(uip);
		uiParms[asset_id].push_back(uip);

		prevWidget = thisWidget;

		i++;
	}
}

HoudiniParameterList* HoudiniEngine::loadParameters(const String& asset_name)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::loadParameters] Not running on %1%", %SystemManager::instance()->getHostname());
		return NULL;
	}

	int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::loadParameters] No asset of name %1%", %asset_name);
        return NULL;
    }

    HoudiniParameterList* parameters = 0;
    Dictionary<String, HoudiniParameterList*>::iterator it = assetParamLists.find(asset_name);

    if (it != assetParamLists.end()) {
        ofmsg("[HoudiniEngine::loadParameters] already loaded: %1%", %asset_name);
        return it->second;
    } else {
        ofmsg("[HoudiniEngine::loadParameters] loading asset: %1%", %asset_name);
        parameters = new HoudiniParameterList();
        assetParamLists[asset_name] = parameters;
    }

	std::vector<hapi::Parm> parms = myAsset->parms();

    for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); ++i) {

        if (i->info().type != HAPI_PARMTYPE_FOLDERLIST) {

            HoudiniParameter* parameter = new HoudiniParameter(i->info().id);

            parameter->setParentId(i->info().parentId);
            parameter->setType(i->info().type);
            parameter->setSize(i->info().size);
            parameter->setName(i->name());
            parameter->setLabel(i->label());

            parameters->addParameter(parameter);
        }
    }

    return parameters;
}

// fetches parameters in a dict of name:value
boost::python::dict HoudiniEngine::getParameters(const String& asset_name)
{
    boost::python::dict d;

    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::getParameters] Not running on %1%", %SystemManager::instance()->getHostname());
		return d;
	}
	int asset_id = assetNameToIds[asset_name];


	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::getParameters] No asset of name %1%", %asset_name);
        return d;

    }
    std::vector<hapi::Parm> parms = myAsset->parms();

    for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {
            switch (it->info().type) {
            case HAPI_PARMTYPE_INT:
            case HAPI_PARMTYPE_MULTIPARMLIST:
            case HAPI_PARMTYPE_TOGGLE:
            case HAPI_PARMTYPE_BUTTON:
                if (it->info().size > 1) {
                    boost::python::list list;
                    for (int i =0; i < it->info().size; ++i) {
                        list.append(it->getIntValue(i));
                    }
                    d[it->name()] = list;
                } else {
                    d[it->name()] = it->getIntValue(0);
                }
                break;
            case HAPI_PARMTYPE_FLOAT:
            case HAPI_PARMTYPE_COLOR:
                if (it->info().size > 1) {
                    boost::python::list list;
                    for (int i =0; i < it->info().size; ++i) {
                        list.append(it->getFloatValue(i));
                    }
                    d[it->name()] = list;
                } else {
                    d[it->name()] = it->getFloatValue(0);
                }
                break;
            case HAPI_PARMTYPE_STRING:
            case HAPI_PARMTYPE_PATH_FILE:
            case HAPI_PARMTYPE_PATH_FILE_GEO:
            case HAPI_PARMTYPE_PATH_FILE_IMAGE:
            case HAPI_PARMTYPE_NODE:
                if (it->info().size > 1) {
                    boost::python::list list;
                    for (int i =0; i < it->info().size; ++i) {
                        list.append(it->getStringValue(i).c_str());
                    }
                    d[it->name()] = list;
                } else {
                    d[it->name()] = it->getStringValue(0).c_str();
                }

                break;
            case HAPI_PARMTYPE_FOLDERLIST:
                // show folders in the list
            case HAPI_PARMTYPE_FOLDERLIST_RADIO:
                // show selected folder from list
            case HAPI_PARMTYPE_FOLDER:
                // show vars in folder
            case HAPI_PARMTYPE_LABEL:
                // show label contents
            case HAPI_PARMTYPE_SEPARATOR:
            default:
                d[it->name()] = it->info().type;
                break;
            }
    }

    return d;
}

boost::python::object HoudiniEngine::getParameterValue(const String& asset_name, const String& parm_name)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::getParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return boost::python::object();
	}

    int asset_id = assetNameToIds[asset_name];

    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::getParameterValue] No asset of name %1%", %asset_name);
        return boost::python::object();

    }

    std::vector<hapi::Parm> parms = myAsset->parms();

    for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {
        if (it->name() == parm_name) {

            switch (it->info().type) {
            case HAPI_PARMTYPE_INT:
                if (it->info().choiceCount != 0) {
                    return boost::python::object(it->choices[it->getIntValue(0)].label());
                    break;
                }
                // continue through, it's ok!
            case HAPI_PARMTYPE_MULTIPARMLIST:
            case HAPI_PARMTYPE_TOGGLE:
            case HAPI_PARMTYPE_BUTTON:
                if (it->info().size > 1) {
                    boost::python::list list;
                    for (int i =0; i < it->info().size; ++i) {
                        list.append(it->getIntValue(i));
                    }
                    return boost::python::object(list);
                } else {
                    return boost::python::object(it->getIntValue(0));
                }
                break;
            case HAPI_PARMTYPE_FLOAT:
            case HAPI_PARMTYPE_COLOR:
                if (it->info().size > 1) {
                    boost::python::list list;
                    for (int i =0; i < it->info().size; ++i) {
                        list.append(it->getFloatValue(i));
                    }
                    return boost::python::object(list);
                } else {
                    return boost::python::object(it->getFloatValue(0));
                }
                break;
            case HAPI_PARMTYPE_STRING:
                if (it->info().choiceCount != 0) {
                    return boost::python::object(it->choices[it->getIntValue(0)].label());
                    break;
                }
                // continue through, it's ok!
            case HAPI_PARMTYPE_PATH_FILE:
            case HAPI_PARMTYPE_PATH_FILE_GEO:
            case HAPI_PARMTYPE_PATH_FILE_IMAGE:
            case HAPI_PARMTYPE_NODE:
                if (it->info().size > 1) {
                    boost::python::list list;
                    for (int i =0; i < it->info().size; ++i) {
                        list.append(it->getStringValue(i).c_str());
                    }
                    return boost::python::object(list);
                } else {
                    return boost::python::object(it->getStringValue(0).c_str());
                }

                break;
            case HAPI_PARMTYPE_FOLDERLIST:
            case HAPI_PARMTYPE_FOLDERLIST_RADIO:
            case HAPI_PARMTYPE_FOLDER:
            case HAPI_PARMTYPE_LABEL:
            case HAPI_PARMTYPE_SEPARATOR:
            default:
                break;
            }
        }
    }

    return boost::python::object();
}

void HoudiniEngine::setParameterValue(const String& asset_name, const String& parm_name, boost::python::object value)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::setParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return;
	}

    int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::setParameterValue] No asset of name %1%", %asset_name);
        return;

    }

    std::vector<hapi::Parm> parms = myAsset->parms();

    for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {
        if (it->name() == parm_name) {

            bool doCook = true;

            switch (it->info().type) {
            case HAPI_PARMTYPE_INT:
                if (it->info().choiceCount != 0) {
                    boost::python::extract<const char*> stringVal(value);
                    if (stringVal.check()) {
                        boost::python::list myChoices = getParameterChoices(asset_name, parm_name);
                        bool found = false;
                        for (int i = 0; i < boost::python::len(myChoices); ++i) {
                            boost::python::extract<const char *> myVal(myChoices[i]);
                            if (strcmp(stringVal, myVal) == 0) {
                                it->setIntValue(0, i);
                                found = true;
                                break;
                            }
                        }

                        if (found) {
                            break;
                        }
                    } // otherwise, fall through and try the int way of doing it
                }
                // continue through, it's ok!
            case HAPI_PARMTYPE_MULTIPARMLIST:
            case HAPI_PARMTYPE_TOGGLE:
            case HAPI_PARMTYPE_BUTTON:
            {
                if (it->info().size > 1) {
                    boost::python::list myList = (boost::python::list) value;
                    if (boost::python::len(myList) != it->info().size) {
                        ofwarn("[HoudiniEngine::setParameterValue] incorrect number of args, got %1%, expected %2%",
                            %boost::python::len(myList)
                            %it->info().size
                        );
                        return;
                    }
                    for (int i =0; i < it->info().size; ++i) {
                        boost::python::extract<int> myVal(myList[i]);
                        it->setIntValue(i, myVal);
                    }
                    break;
                } else {
                    boost::python::extract<int> intVal(value);
                    if (intVal.check()) {
                        it->setIntValue(0, intVal);
                    } else {
                        ofwarn("[HoudiniEngine::setParameterValue] '%1%' not an integer value", %value);
                    }
                    break;
                }
            }
            case HAPI_PARMTYPE_FLOAT:
            case HAPI_PARMTYPE_COLOR:
            {
                if (it->info().size > 1) {
                    boost::python::list myList = (boost::python::list) value;
                    if (boost::python::len(myList) != it->info().size) {
                        ofwarn("[HoudiniEngine::setParameterValue] incorrect number of args, got %1%, expected %2%",
                            %boost::python::len(myList)
                            %it->info().size
                        );
                        return;
                    }
                    for (int i =0; i < it->info().size; ++i) {
                        boost::python::extract<float> myVal(myList[i]);
                        it->setFloatValue(i, myVal);
                    }
                    break;
                } else {
                    boost::python::extract<float> floatVal(value);
                    if (floatVal.check()) {
                        it->setFloatValue(0, floatVal);
                    } else {
                        ofwarn("[HoudiniEngine::setParameterValue] '%1%' not a float value", %value);
                    }
                    break;
                }
            }
            case HAPI_PARMTYPE_STRING:
                if (it->info().choiceCount != 0) {
                    boost::python::extract<const char*> stringVal(value);
                    if (stringVal.check()) {
                        boost::python::list myChoices = getParameterChoices(asset_name, parm_name);
                        bool found = false;
                        for (int i = 0; i < boost::python::len(myChoices); ++i) {
                            boost::python::extract<const char *> myVal(myChoices[i]);
                            if (strcmp(stringVal, myVal) == 0) {
                                it->setStringValue(0, myVal);
                                found = true;
                                break;
                            }
                        }
                        if (found) {
                            break;
                        }
                    } // otherwise, fall through and try the int way of doing it
                }
                // continue through, it's ok!
            case HAPI_PARMTYPE_PATH_FILE:
            case HAPI_PARMTYPE_PATH_FILE_GEO:
            case HAPI_PARMTYPE_PATH_FILE_IMAGE:
            case HAPI_PARMTYPE_NODE:
            {
                if (it->info().size > 1) {
                    boost::python::list myList = (boost::python::list) value;
                    if (boost::python::len(myList) != it->info().size) {
                        ofwarn("[HoudiniEngine::setParameterValue] incorrect number of args, got %1%, expected %2%",
                            %boost::python::len(myList)
                            %it->info().size
                        );
                        return;
                    }
                    for (int i =0; i < it->info().size; ++i) {
                        boost::python::extract<const char*> myVal(myList[i]);
                        it->setStringValue(i, myVal);
                    }
                    break;
                } else {
                    boost::python::extract<const char*> stringVal(value);
                    if (stringVal.check()) {
                        it->setStringValue(0, stringVal);
                    } else {
                        ofwarn("[HoudiniEngine::setParameterValue] '%1%' not a string value", %value);
                    }
                    break;
                }
            }
            // {
            //     boost::python::extract<const char* > stringVal(value);
            //     if (stringVal.check()) {
            //         it->setStringValue(0, stringVal);
            //     } else {
            //         ofwarn("'%1%' not an string value", %value);
            //     }
            //     break;
            // }
            case HAPI_PARMTYPE_FOLDERLIST:
            case HAPI_PARMTYPE_FOLDERLIST_RADIO:
            case HAPI_PARMTYPE_FOLDER:
            case HAPI_PARMTYPE_LABEL:
            case HAPI_PARMTYPE_SEPARATOR:
            default:
                doCook = false;
                break;
            }

            if (doCook) {
                cook_one(myAsset);
            }

            return;
        }
    }

}

void HoudiniEngine::insertMultiparmInstance(const String& asset_name, const String& parm_name, int pos) {

    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::insertMultiparmInstance] Not running on %1%", %SystemManager::instance()->getHostname());
		return;
	}

    int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::insertMultiparmInstance] No asset of name %1%", %asset_name);
        return;

    }

    std::vector<hapi::Parm> parms = myAsset->parms();
    for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {
        if (it->name() == parm_name) {
            it->insertMultiparmInstance(pos);
            cook_one(myAsset);
            return;
        }
    }
}
void HoudiniEngine::removeMultiparmInstance(const String& asset_name, const String& parm_name, int pos) {

    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::removeMultiparmInstance] Not running on %1%", %SystemManager::instance()->getHostname());
		return;
	}

    int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::removeMultiparmInstance] No asset of name %1%", %asset_name);
        return;

    }

    std::vector<hapi::Parm> parms = myAsset->parms();
    for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {
        if (it->name() == parm_name) {
            it->removeMultiparmInstance(pos);
            cook_one(myAsset);
            return;
        }
    }

}

boost::python::list HoudiniEngine::getParameterChoices(const String& asset_name, const String& parm_name) {
    boost::python::list myList;

    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::getParameterChoices] Not running on %1%", %SystemManager::instance()->getHostname());
		return myList;
	}

    int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::getParameterChoices] No asset of name %1%", %asset_name);
        return myList;

    }

    std::vector<hapi::Parm> parms = myAsset->parms();
    for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {
        if (it->name() == parm_name) {
            for (int i=0; i < it->choices.size(); ++i) {
                myList.append(it->choices[i].label());
            }
            break;
        }
    }

    return myList;
}


int HoudiniEngine::getIntegerParameterValue(const String& asset_name, int param_id, int sub_index)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::getIntegerParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return 0;
	}

	int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::getIntegerParameterValue] No asset of name %1%", %asset_name);
        return 0;

    } else {
        std::vector<hapi::Parm> parms = myAsset->parms();

        for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {

            if (it->info().id == param_id) {
                return it->getIntValue(sub_index);
            }
        }
    }
}

void HoudiniEngine::setIntegerParameterValue(const String& asset_name, int param_id, int sub_index, int value)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::setIntegerParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return;
	}

	int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::setIntegerParameterValue] No asset of name %1%", %asset_name);

    } else {
        std::vector<hapi::Parm> parms = myAsset->parms();

        for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {

            if (it->info().id == param_id) {
                it->setIntValue(sub_index, value);
                break;
            }
        }

        cook_one(myAsset);
    }
}

float HoudiniEngine::getFloatParameterValue(const String& asset_name, int param_id, int sub_index)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::getFloatParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return 0;
	}

	int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::getFloatParameterValue] No asset of name %1%", %asset_name);
        return 0;

    } else {
        std::vector<hapi::Parm> parms = myAsset->parms();

        for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {

            if (it->info().id == param_id) {
                return it->getFloatValue(sub_index);
            }
        }
    }
}

void HoudiniEngine::setFloatParameterValue(const String& asset_name, int param_id, int sub_index, float value)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::setFloatParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return;
	}

	int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::setFloatParameterValue] No asset of name %1%", %asset_name);

    } else {
        std::vector<hapi::Parm> parms = myAsset->parms();

        for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {

            if (it->info().id == param_id) {
                it->setFloatValue(sub_index, value);
                break;
            }
        }

        cook_one(myAsset);
    }
}

String HoudiniEngine::getStringParameterValue(const String& asset_name, int param_id, int sub_index)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::getStringParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return "";
	}

	int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::getStringParameterValue] No asset of name %1%", %asset_name);
        return "";

    } else {
        std::vector<hapi::Parm> parms = myAsset->parms();

        for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {

            if (it->info().id == param_id) {
                return it->getStringValue(sub_index);
            }
        }
    }
}

void HoudiniEngine::setStringParameterValue(const String& asset_name, int param_id, int sub_index, const String& value)
{
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::setStringParameterValue] Not running on %1%", %SystemManager::instance()->getHostname());
		return;
	}

	int asset_id = assetNameToIds[asset_name];
    // shouldn't be cached, should fetch new each time
	hapi::Asset* myAsset = new hapi::Asset(asset_id, session);

    if (myAsset == NULL) {
        ofwarn("[HoudiniEngine::setStringParameterValue] No asset of name %1%", %asset_name);

    } else {
        std::vector<hapi::Parm> parms = myAsset->parms();

        for (vector<hapi::Parm>::iterator it = parms.begin(); it < parms.end(); ++it) {

            if (it->info().id == param_id) {
                it->setStringValue(sub_index, value.c_str());
                break;
            }
        }

        cook_one(myAsset);
    }
}

Container* HoudiniEngine::getParmCont(int i, int asset_id) {
    return ((HoudiniUiParm*) uiParms[asset_id][i].get())->getContents();
}

Container* HoudiniEngine::getParmBase(int i, int asset_id) {
    return ((HoudiniUiParm*) uiParms[asset_id][i].get())->getContainer();
}

void HoudiniEngine::doIt(int asset_id) {
    Container* base = ((HoudiniUiParm*) uiParms[asset_id][uiParms[asset_id].size() - 1].get())->getContainer();
    uiParms[asset_id].pop_back();
    ofmsg("%1% refcount %2%", %base->getName() %base->refCount());
    base = NULL;
}

void HoudiniEngine::printParms(int asset_id) {
    // only run on master
	if (!SystemManager::instance()->isMaster()) {
		hflog("[HoudiniEngine::printParms] Not running on %1%", %SystemManager::instance()->getHostname());
		return;
	}

    hapi::Asset* asset = new hapi::Asset(asset_id, session);
    if (asset == NULL) {
        ofwarn("[HoudiniEngine::printParms] No asset with id %1%", %asset_id);
        return;
    }
    ofmsg("[HoudiniEngine::printParms] asset has %1% parms", %asset->nodeInfo().parmCount);
    std::vector<hapi::Parm> parms = asset->parms();
    foreach (hapi::Parm myParm, parms) {
        String t = "";
        switch (myParm.info().type) {
            case HAPI_PARMTYPE_INT: t = "INT"; break;
            case HAPI_PARMTYPE_MULTIPARMLIST: t = "MPL";  break;
            case HAPI_PARMTYPE_TOGGLE: t = "TOG";  break;
            case HAPI_PARMTYPE_BUTTON: t = "BUT";  break;
            case HAPI_PARMTYPE_FLOAT: t = "FLT";  break;
            case HAPI_PARMTYPE_COLOR: t = "COL";  break;
            case HAPI_PARMTYPE_STRING: t = "STR";  break;
            case HAPI_PARMTYPE_PATH_FILE: t = "FIL";  break;
            case HAPI_PARMTYPE_PATH_FILE_GEO: t = "GEO";  break;
            case HAPI_PARMTYPE_PATH_FILE_IMAGE: t = "IMG";  break;
            case HAPI_PARMTYPE_NODE: t = "NOD";  break;
            case HAPI_PARMTYPE_FOLDERLIST: t = "FLS";  break;
            case HAPI_PARMTYPE_FOLDER: t = "FLD";  break;
            case HAPI_PARMTYPE_LABEL: t = "LAB";  break;
            case HAPI_PARMTYPE_SEPARATOR: t = "SEP";  break;
        }
        ofmsg("[HoudiniEngine::printParms] PARM %1% (%2%) %3%x%4% name: %5% (%6%)",
            %myParm.info().id
            %myParm.info().parentId
            %t
            %myParm.info().size
            %myParm.name()
            %myParm.label()
        );
        for (int i = 0; i < myParm.info().size; ++i) {
            if (myParm.info().type == HAPI_PARMTYPE_INT) {
                int val = myParm.getIntValue(i);
                if (myParm.info().choiceCount > 0) {
                    switch (myParm.info().choiceListType) {
                        case HAPI_CHOICELISTTYPE_NONE:
                            omsg("[HoudiniEngine::printParms] choice list type is NONE");
                            break;
                        case HAPI_CHOICELISTTYPE_NORMAL:
                            omsg("[HoudiniEngine::printParms] choice list type is NORMAL");
                            break;
                        case HAPI_CHOICELISTTYPE_MINI:
                            omsg("[HoudiniEngine::printParms] choice list type is MINI");
                            break;
                        case HAPI_CHOICELISTTYPE_REPLACE:
                            omsg("[HoudiniEngine::printParms] choice list type is REPLACE");
                            break;
                        case HAPI_CHOICELISTTYPE_TOGGLE:
                            omsg("[HoudiniEngine::printParms] choice list type is TOGGLE");
                            break;
                        default:
                            break;
                    }
                    ofmsg("[HoudiniEngine::printParms]   choiceindex: %1% count: %2%", %myParm.info().choiceIndex  %myParm.info().choiceCount);
                    // display as a selection menu
                    if (myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
                        myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
                        myParm.info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
                        for (int j = 0; j < myParm.choices.size(); ++j) {
                            ofmsg("[HoudiniEngine::printParms]   choice %1% (%2%): %3% (%4%)",
                                %j
                                %myParm.choices[j].info().parentParmId
                                %myParm.choices[j].value()
                                %myParm.choices[j].label()
                            );
                        }
                    } else {
                        // display as a text box which can be filled in by preset selections
                        // same as if there was no parmChoice count
                        // TODO: add the menu as well..
                        if (myParm.info().hasUIMin && myParm.info().hasUIMax) {
                            ofmsg("[HoudiniEngine::printParms]   UImin %1% UImax %2%", %myParm.info().UIMin %myParm.info().UIMax);
                        } else {
                            if (myParm.info().hasMin) ofmsg("[HoudiniEngine::printParms]   min %1%", %myParm.info().min);
                            if (myParm.info().hasMax) ofmsg("[HoudiniEngine::printParms]   max %1%", %myParm.info().max);
                        }
                    }
                } else {
                    if (myParm.info().hasUIMin && myParm.info().hasUIMax) {
                        ofmsg("[HoudiniEngine::printParms]   UImin %1% UImax %2%", %myParm.info().UIMin %myParm.info().UIMax);
                    } else {
                        if (myParm.info().hasMin) ofmsg("[HoudiniEngine::printParms]   min %1%", %myParm.info().min);
                        if (myParm.info().hasMax) ofmsg("[HoudiniEngine::printParms]   max %1%", %myParm.info().max);
                    }
                }

            } else if (myParm.info().type == HAPI_PARMTYPE_TOGGLE) {
                int val = myParm.getIntValue(i);

            } else if (myParm.info().type == HAPI_PARMTYPE_BUTTON) {
                int val = myParm.getIntValue(i);

            } else if (myParm.info().type == HAPI_PARMTYPE_FLOAT ||
                       myParm.info().type == HAPI_PARMTYPE_COLOR) {
                float val = myParm.getFloatValue(i);
                if (myParm.info().hasUIMin && myParm.info().hasUIMax) {
                    ofmsg("[HoudiniEngine::printParms]   UImin %1% UImax %2%", %myParm.info().UIMin %myParm.info().UIMax);
                } else {
                    if (myParm.info().hasMin) ofmsg("[HoudiniEngine::printParms]   min %1%", %myParm.info().min);
                    if (myParm.info().hasMax) ofmsg("[HoudiniEngine::printParms]   max %1%", %myParm.info().max);
                }

            } else if (myParm.info().type == HAPI_PARMTYPE_STRING ||
                       myParm.info().type == HAPI_PARMTYPE_PATH_FILE ||
                       myParm.info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
                       myParm.info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) { // TODO: update with TextBox
                std::string val = myParm.getStringValue(i);
                if (myParm.info().choiceCount > 0) {
                    switch (myParm.info().choiceListType) {
                        case HAPI_CHOICELISTTYPE_NONE:
                            omsg("[HoudiniEngine::printParms] choice list type is NONE");
                            break;
                        case HAPI_CHOICELISTTYPE_NORMAL:
                            omsg("[HoudiniEngine::printParms] choice list type is NORMAL");
                            break;
                        case HAPI_CHOICELISTTYPE_MINI:
                            omsg("[HoudiniEngine::printParms] choice list type is MINI");
                            break;
                        case HAPI_CHOICELISTTYPE_REPLACE:
                            omsg("[HoudiniEngine::printParms] choice list type is REPLACE");
                            break;
                        case HAPI_CHOICELISTTYPE_TOGGLE:
                            omsg("[HoudiniEngine::printParms] choice list type is TOGGLE");
                            break;
                        default:
                            break;
                    }
                    ofmsg("[HoudiniEngine::printParms]   choiceindex: %1% count: %2%",
                        %myParm.info().choiceIndex
                        %myParm.info().choiceCount
                    );
                    // display as a selection menu
                    if (myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
                        myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
                        myParm.info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
                        for (int j = 0; j < myParm.choices.size(); ++j) {
                            ofmsg("[HoudiniEngine::printParms]   choice %1% (%2%): %3% (%4%)",
                                %j
                                %myParm.choices[j].info().parentParmId
                                %myParm.choices[j].value()
                                %myParm.choices[j].label()
                            );
                        }
                    } else {// display as a text box which can be filled in by preset selections
                        // same as if there was no parmChoice count
                        // TODO: add the menu as well..
                        omsg ("[HoudiniEngine::printParms]   skipped");
                    }
                } else {
                    ofmsg("[HoudiniEngine::printParms]   textbox text: %1%", %val);
                }
            } else if (myParm.info().type == HAPI_PARMTYPE_SEPARATOR) {
                omsg("[HoudiniEngine::printParms] ----------");
            } else if (myParm.info().type == HAPI_PARMTYPE_MULTIPARMLIST) {
                ofmsg("[HoudiniEngine::printParms] multiparm, instance length %1%, instance count %2%, instance offset %3%",
                    %myParm.info().instanceLength
                    %myParm.info().instanceCount
                    %myParm.info().instanceStartOffset
                );
            }
        }

    }
}
