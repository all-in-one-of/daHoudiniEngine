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

// TODO: generalise this so the following works:
// check for uiMin/uiMax, then use sliders
// use text boxes for vectors and non min/max things
// use submenus/containers for choices
// use the joinNext variable for displaying items
// use checkbox for HAPI_PARMTYPE_TOGGLE
// use text box for string
// do multiparms
// do menu layout

// HAPI_PARMTYPE_INT 				= 0,
// HAPI_PARMTYPE_MULTIPARMLIST,		= 1
// HAPI_PARMTYPE_TOGGLE,			= 2
// HAPI_PARMTYPE_BUTTON,			= 3
//
// HAPI_PARMTYPE_FLOAT,				= 4
// HAPI_PARMTYPE_COLOR,				= 5
//
// HAPI_PARMTYPE_STRING,			= 6
// HAPI_PARMTYPE_PATH_FILE,			= 7
// HAPI_PARMTYPE_PATH_FILE_GEO,		= 8
// HAPI_PARMTYPE_PATH_FILE_IMAGE,	= 9
// HAPI_PARMTYPE_PATH_NODE,			= 10
//
// HAPI_PARMTYPE_FOLDERLIST,		= 11
//
// HAPI_PARMTYPE_FOLDER,			= 12
// HAPI_PARMTYPE_LABEL,				= 13
// HAPI_PARMTYPE_SEPARATOR,			= 14
//
// HAPI_PARMTYPE_MAX - total supported parms = 15?
//
// creates a container for the particluar parm to edit
void HoudiniEngine::createParm(const String& asset_name, Container* cont, hapi::Parm* parm) {

	ofmsg("PARM %1%: %2% s-%3% id-%4% name: %5%", %parm->label() %parm->info().type %parm->info().size
		%parm->info().parentId
		%parm->name()
	);

	Label* label = Label::create(cont);
	label->setText(parm->label());
	label->setHorizontalAlign(Label::AlignRight);

// 	assetParamConts[asset_name].push_back(cont);

	for (int i = 0; i < parm->info().size; ++i) {
		if (parm->info().type == HAPI_PARMTYPE_INT) {
			int val = parm->getIntValue(i);
			if (parm->info().choiceCount > 0) {
				switch (parm->info().choiceListType) {
					case HAPI_CHOICELISTTYPE_NONE:
						omsg("choice list type is NONE");
						break;
					case HAPI_CHOICELISTTYPE_NORMAL:
						omsg("choice list type is NORMAL");
						break;
					case HAPI_CHOICELISTTYPE_MINI:
						omsg("choice list type is MINI");
						break;
					case HAPI_CHOICELISTTYPE_REPLACE:
						omsg("choice list type is REPLACE");
						break;
					case HAPI_CHOICELISTTYPE_TOGGLE:
						omsg("choice list type is TOGGLE");
						break;
					default:
						break;
				}
				ofmsg("  choiceindex: %1% count: %2%", %parm->info().choiceIndex  %parm->info().choiceCount);
				// display as a selection menu
				if (parm->info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
					parm->info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
					parm->info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
					
					Container* choiceCont = Container::create(Container::LayoutVertical, cont);
					for (int j = 0; j < parm->choices.size(); ++j) {
						Button* button = Button::create(choiceCont);
						button->setRadio(true);
						button->setCheckable(true);
						button->setChecked(j == val);
						button->setText(parm->choices[j].label());
						ofmsg("  choice %1%: %2% (%3%) parentId: %4%", %j %parm->choices[j].label() %parm->choices[j].value()
						%parm->choices[j].info().parentParmId
						);
						button->setUIEventHandler(this);
						button->setUserData(label);
						widgetIdToParmName[button->getId()] = parm->name();
					}
				} else {// display as a text box which can be filled in by preset selections
					// same as if there was no parmChoice count
					// TODO: add the menu as well..
					label->setText(parm->label() + " " + ostr("%1%", %val));
// 					assetParamConts[asset_name].push_back(cont);
					Slider* slider = Slider::create(cont);
					if (parm->info().hasUIMin && parm->info().hasUIMax) {
						ofmsg("min %1% max %2%", %parm->info().UIMin %parm->info().UIMax);
						slider->setTicks(parm->info().UIMax - parm->info().UIMin + 1);
						slider->setValue(val - parm->info().UIMin);
					} else {
						if (parm->info().hasMin) ofmsg("min %1%", %parm->info().min);
						if (parm->info().hasMax) ofmsg("max %1%", %parm->info().max);
						slider->setTicks(parm->info().max + 1);
						slider->setValue(val);
					}
					slider->setDeferUpdate(true);
					slider->setUIEventHandler(this);
					// TODO refactor setUserData to refer to parms and label?
					slider->setUserData(label); // reference to the label for this widget item
					
					widgetIdToParmName[slider->getId()] = parm->name();
				}
			} else {
				label->setText(parm->label() + " " + ostr("%1%", %val));
// 				assetParamConts[asset_name].push_back(cont);
				Slider* slider = Slider::create(cont);
				if (parm->info().hasUIMin && parm->info().hasUIMax) {
					ofmsg("min %1% max %2%", %parm->info().UIMin %parm->info().UIMax);
					slider->setTicks(parm->info().UIMax - parm->info().UIMin + 1);
					slider->setValue(val - parm->info().UIMin);
				} else {
					if (parm->info().hasMin) ofmsg("min %1%", %parm->info().min);
					if (parm->info().hasMax) ofmsg("max %1%", %parm->info().max);
					slider->setTicks(parm->info().max + 1);
					slider->setValue(val);
				}
				slider->setDeferUpdate(true);
				slider->setUIEventHandler(this);
				// TODO refactor setUserData to refer to parms and label?
				slider->setUserData(label); // reference to the label for this widget item

				widgetIdToParmName[slider->getId()] = parm->name();
			}
		} else if (parm->info().type == HAPI_PARMTYPE_TOGGLE) {
			int val = parm->getIntValue(i);
// 			assetParamConts[asset_name].push_back(cont);
			Button* button = Button::create(cont);
			button->setText("X");
			button->setCheckable(true);
			button->setChecked(val);
			button->setUIEventHandler(this);
			button->setUserData(label);

			widgetIdToParmName[button->getId()] = parm->name();
		} else if (parm->info().type == HAPI_PARMTYPE_BUTTON) {
			int val = parm->getIntValue(i);
// 			assetParamConts[asset_name].push_back(cont);
			Button* button = Button::create(cont);
			button->setText("X");
			button->setChecked(val);
			button->setUIEventHandler(this);
			button->setUserData(label);

			widgetIdToParmName[button->getId()] = parm->name();
		} else if (parm->info().type == HAPI_PARMTYPE_FLOAT ||
				   parm->info().type == HAPI_PARMTYPE_COLOR) {
			float val = parm->getFloatValue(i);
			label->setText(parm->label() + " " + ostr("%1%", %val));
// 			assetParamConts[asset_name].push_back(cont);
			Slider* slider = Slider::create(cont);
			if (parm->info().hasUIMin && parm->info().hasUIMax) {
				ofmsg("min %1% max %2%", %parm->info().UIMin %parm->info().UIMax);
				float sliderVal = (val - parm->info().UIMin) / (float) (parm->info().UIMax - parm->info().UIMin);
				slider->setValue(slider->getTicks() * sliderVal);
			} else {
				if (parm->info().hasMin) ofmsg("min %1%", %parm->info().min);
				if (parm->info().hasMax) ofmsg("max %1%", %parm->info().max);
				slider->setTicks(parm->info().max + 1);
				slider->setValue(val);
			}
			slider->setDeferUpdate(true);
			slider->setUIEventHandler(this);
			slider->setUserData(label);

			widgetIdToParmName[slider->getId()] = parm->name();
		} else if (parm->info().type == HAPI_PARMTYPE_STRING ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) { // TODO: update with TextBox
			std::string val = parm->getStringValue(i);
			if (parm->info().choiceCount > 0) {
				switch (parm->info().choiceListType) {
					case HAPI_CHOICELISTTYPE_NONE:
						omsg("choice list type is NONE");
						break;
					case HAPI_CHOICELISTTYPE_NORMAL:
						omsg("choice list type is NORMAL");
						break;
					case HAPI_CHOICELISTTYPE_MINI:
						omsg("choice list type is MINI");
						break;
					case HAPI_CHOICELISTTYPE_REPLACE:
						omsg("choice list type is REPLACE");
						break;
					case HAPI_CHOICELISTTYPE_TOGGLE:
						omsg("choice list type is TOGGLE");
						break;
					default:
						break;
				}
				ofmsg("  choiceindex: %1% count: %2%", 
					  %parm->info().choiceIndex  
					  %parm->info().choiceCount
				);
				// display as a selection menu
				if (parm->info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
					parm->info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
					parm->info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
// 					assetParamConts[asset_name].push_back(cont);
					Container* choiceCont = Container::create(Container::LayoutVertical, cont);
					for (int j = 0; j < parm->choices.size(); ++j) {
						Button* button = Button::create(choiceCont);
						button->setRadio(true);
						button->setCheckable(true);
						button->setChecked(parm->choices[j].value() == val);
						button->setText(parm->choices[j].label());
						ofmsg("  choice %1%: %2% (%3%) parentId: %4%", %j %parm->choices[j].label() %parm->choices[j].value()
						%parm->choices[j].info().parentParmId
						);
						button->setUIEventHandler(this);
						button->setUserData(label);
						widgetIdToParmName[button->getId()] = parm->name();
					}
				} else {// display as a text box which can be filled in by preset selections
					// same as if there was no parmChoice count
					// TODO: add the menu as well..
// 					assetParamConts[asset_name].push_back(cont);
					TextBox* box = TextBox::create(cont);
					box->setFont("fonts/segoeuimod.ttf 14");
					box->setText(val);
					box->setUIEventHandler(this);
					box->setUserData(label);
					
					widgetIdToParmName[box->getId()] = parm->name();
				}
			} else {
// 				assetParamConts[asset_name].push_back(cont);
				TextBox* box = TextBox::create(cont);
				box->setFont("fonts/segoeuimod.ttf 14");
				box->setText(val);
				box->setUIEventHandler(this);
				box->setUserData(label);

				widgetIdToParmName[box->getId()] = parm->name();
			}
		} else if (parm->info().type == HAPI_PARMTYPE_SEPARATOR) {
			label->setText("----------");
		} else if (parm->info().type == HAPI_PARMTYPE_MULTIPARMLIST) {
			ofmsg("this is a multiparm, instance length %1%, instance count %2%, instance offset %3%",
				  %parm->info().instanceLength
				  %parm->info().instanceCount
				  %parm->info().instanceStartOffset
			);
			label->setText(parm->label() + " " + ostr("%1%", %parm->info().instanceCount));
			// TODO: Add/Remove buttons, number of items, clear button?
			multiParmConts[parm->name()] = cont;
			// add another container to contain label and buttons
			Container* multiParmButtonCont = Container::create(Container::LayoutVertical, cont);
			ofmsg("Multiparm cont %1%: id-%2%",
				  %multiParmButtonCont->getName()
				  %multiParmButtonCont->getId()
			);
			cont->removeChild(label);
			multiParmButtonCont->addChild(label);
			Button* addButton = Button::create(multiParmButtonCont);
			addButton->setName(ostr("%1%_add", %parm->name()));
			addButton->setText("+");
			addButton->setUIEventHandler(this);
			Button* remButton = Button::create(multiParmButtonCont);
			remButton->setName(ostr("%1%_rem", %parm->name()));
			remButton->setText("-");
			remButton->setUIEventHandler(this);
			Button* clrButton = Button::create(multiParmButtonCont);
			clrButton->setName(ostr("%1%_clr", %parm->name()));
			clrButton->setText("Clear");
			clrButton->setUIEventHandler(this);

		}

// 		assetParamConts[asset_name].push_back(cont);
	}
}

// only run on master
// identical-looking menus get created on slaves
// a base assetCont container is created, and this is then pushed onto assetConts
void HoudiniEngine::createMenu(const int asset_id)
{
	// load only the params in the DA Folder
	// first find the index of the folder, then iterate through the list of params
	// from there onwards
	const char* daFolderName = "daFolder_0";

	hapi::Asset* myAsset = instancedHEAssets[asset_id];
	std::string asset_name = myAsset->name();

	if (myAsset == NULL) {
		ofwarn("No asset of name %1%", %asset_name);
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

	// NOTE: folder types have parentId of -1 as well.. mistake?
	while (i < parms.size()) {
		hapi::Parm* parm = &parms[i];
		if (parm->info().parentId == -1) {
			target = assetCont;
		} else {
			for (int j = 0; j < uiParms[asset_id].size(); ++j) {
				if (uiParms[asset_id][j]->getParm().info().id == parm->info().parentId) {
					target = uiParms[asset_id][j]->getContainer();
					break;
				}
			}
		}

		HoudiniUiParm* uip;

		uip = HoudiniUiParm::create(*parm, target);

		// this will not work on a HoudiniUiParm that isn't part of a folder?
		uip->getContainer()->setUIEventHandler(uip);
		uiParms[asset_id].push_back(uip);

		i++;
	}
}

HoudiniParameterList* HoudiniEngine::loadParameters(const String& asset_name)
{

	int asset_id = assetNameToIds[asset_name];
    hapi::Asset* asset = instancedHEAssets[asset_id];

    if (asset == NULL) {
        ofwarn("No asset of name %1%", %asset_name);
        return NULL;
    }

    HoudiniParameterList* parameters = 0;
    Dictionary<String, HoudiniParameterList*>::iterator it = assetParamLists.find(asset_name);

    if (it != assetParamLists.end()) {
        ofmsg("Asset already loaded: %1%", %asset_name);
        return it->second;
    } else {
        ofmsg("Proceeding to load asset: %1%", %asset_name);
        parameters = new HoudiniParameterList(); 
        assetParamLists[asset_name] = parameters;
    }

	std::vector<hapi::Parm> parms = asset->parms();

    for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); i++) {
        
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

int HoudiniEngine::getIntegerParameterValue(const String& asset_name, int param_id, int sub_index)
{
	int asset_id = assetNameToIds[asset_name];
    hapi::Asset* asset = instancedHEAssets[asset_id];

    if (asset == NULL) {
        ofwarn("No asset of name %1%", %asset_name);
        return 0;

    } else {
        std::vector<hapi::Parm> parms = asset->parms();

        for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); i++) {
            
            if (i->info().id == param_id) {
                return i->getIntValue(sub_index);
            }
        }
    }
}

void HoudiniEngine::setIntegerParameterValue(const String& asset_name, int param_id, int sub_index, int value) 
{
	int asset_id = assetNameToIds[asset_name];
    hapi::Asset* asset = instancedHEAssets[asset_id];

    if (asset == NULL) {
        ofwarn("No asset of name %1%", %asset_name);

    } else {
        std::vector<hapi::Parm> parms = asset->parms();

        for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); i++) {
            
            if (i->info().id == param_id) {
                i->setIntValue(sub_index, value);
                break;
            }
        }

        cook_one(asset);
    }
}

float HoudiniEngine::getFloatParameterValue(const String& asset_name, int param_id, int sub_index)
{
	int asset_id = assetNameToIds[asset_name];
    hapi::Asset* asset = instancedHEAssets[asset_id];

    if (asset == NULL) {
        ofwarn("No asset of name %1%", %asset_name);
        return 0;

    } else {
        std::vector<hapi::Parm> parms = asset->parms();

        for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); i++) {
            
            if (i->info().id == param_id) {
                return i->getFloatValue(sub_index);
            }
        }
    }
}

void HoudiniEngine::setFloatParameterValue(const String& asset_name, int param_id, int sub_index, float value) 
{
	int asset_id = assetNameToIds[asset_name];
    hapi::Asset* asset = instancedHEAssets[asset_id];

    if (asset == NULL) {
        ofwarn("No asset of name %1%", %asset_name);

    } else {
        std::vector<hapi::Parm> parms = asset->parms();

        for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); i++) {
            
            if (i->info().id == param_id) {
                i->setFloatValue(sub_index, value);
                break;
            }
        }

        cook_one(asset);
    }
}

String HoudiniEngine::getStringParameterValue(const String& asset_name, int param_id, int sub_index)
{
	int asset_id = assetNameToIds[asset_name];
    hapi::Asset* asset = instancedHEAssets[asset_id];

    if (asset == NULL) {
        ofwarn("No asset of name %1%", %asset_name);
        return "";

    } else {
        std::vector<hapi::Parm> parms = asset->parms();

        for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); i++) {
            
            if (i->info().id == param_id) {
                return i->getStringValue(sub_index);
            }
        }
    }
}

void HoudiniEngine::setStringParameterValue(const String& asset_name, int param_id, int sub_index, const String& value) 
{
	int asset_id = assetNameToIds[asset_name];
    hapi::Asset* asset = instancedHEAssets[asset_id];

    if (asset == NULL) {
        ofwarn("No asset of name %1%", %asset_name);

    } else {
        std::vector<hapi::Parm> parms = asset->parms();

        for (vector<hapi::Parm>::iterator i = parms.begin(); i < parms.end(); i++) {
            
            if (i->info().id == param_id) {
                i->setStringValue(sub_index, value.c_str());
                break;
            }
        }

        cook_one(asset);
    }
}
