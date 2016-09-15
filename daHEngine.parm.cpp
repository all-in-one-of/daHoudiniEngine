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

	assetParamConts[asset_name].push_back(cont);

	for (int i = 0; i < parm->info().size; ++i) {
		if (parm->info().type == HAPI_PARMTYPE_INT) {
			int val = parm->getIntValue(i);
			if (parm->info().choiceCount > 0) {
				ofmsg("  choiceindex: %1%", %parm->info().choiceIndex);
				ofmsg("  choiceCount: %1%", %parm->info().choiceCount);
				Container* choiceCont = Container::create(Container::LayoutVertical, cont);
				for (int j = 0; j < parm->choices.size(); ++j) {
					Button* button = Button::create(choiceCont);
					button->setRadio(true);
					button->setCheckable(true);
					button->setChecked(j == val);
					button->setText(parm->choices[j].label());
					ofmsg("  choice %1%: %2% %3%", %j %parm->choices[j].label() %parm->choices[j].value());
					button->setUIEventHandler(this);
					button->setUserData(label);
					widgetIdToParmId[button->getId()] = parm->info().id;
				}
			} else {
				label->setText(parm->label() + " " + ostr("%1%", %val));
	// 			label->setUserTag(asset_name); // reference to the asset this param belongs to
				assetParamConts[asset_name].push_back(cont);
				Slider* slider = Slider::create(cont);
				if (parm->info().hasUIMin && parm->info().hasUIMax) {
					slider->setTicks(parm->info().UIMax - parm->info().UIMin + 1);
					slider->setValue(val - parm->info().UIMin);
				} else {
					slider->setTicks(parm->info().max + 1);
					slider->setValue(val);
				}
				slider->setDeferUpdate(true);
				slider->setUIEventHandler(this);
				// TODO refactor setUserData to refer to parms and label?
				slider->setUserData(label); // reference to the label for this widget item

				widgetIdToParmId[slider->getId()] = parm->info().id;
			}
		} else if (parm->info().type == HAPI_PARMTYPE_TOGGLE) {
			int val = parm->getIntValue(i);
// 			label->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParamConts[asset_name].push_back(cont);
			Button* button = Button::create(cont);
			button->setText("X");
			button->setCheckable(true);
			button->setChecked(val);
// 			label->setUserData(miLabel); // reference to the label, for updating values
			button->setUIEventHandler(this);
			button->setUserData(label);

			widgetIdToParmId[button->getId()] = parm->info().id;
		} else if (parm->info().type == HAPI_PARMTYPE_BUTTON) {
			int val = parm->getIntValue(i);
// 			label->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParamConts[asset_name].push_back(cont);
			Button* button = Button::create(cont);
			button->setText("X");
// 			button->setCheckable(true);
			button->setChecked(val);
// 			label->setUserData(miLabel); // reference to the label, for updating values
			button->setUIEventHandler(this);
			button->setUserData(label);

			widgetIdToParmId[button->getId()] = parm->info().id;
		} else if (parm->info().type == HAPI_PARMTYPE_FLOAT ||
				   parm->info().type == HAPI_PARMTYPE_COLOR) {
			float val = parm->getFloatValue(i);
			label->setText(parm->label() + " " + ostr("%1%", %val));
// 			label->setUserTag(asset_name); // reference to the asset this param belongs to
			assetParamConts[asset_name].push_back(cont);
			Slider* slider = Slider::create(cont);
			if (parm->info().hasUIMin && parm->info().hasUIMax) {
				float sliderVal = (val - parm->info().UIMin) / (float) (parm->info().UIMax - parm->info().UIMin);
				slider->setValue(slider->getTicks() * sliderVal);
			} else {
				slider->setTicks(parm->info().max + 1);
				slider->setValue(val);
			}
			slider->setDeferUpdate(true);
			slider->setUIEventHandler(this);
			slider->setUserData(label);

			widgetIdToParmId[slider->getId()] = parm->info().id;
		} else if (parm->info().type == HAPI_PARMTYPE_STRING ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE ||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
				   parm->info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) { // TODO: update with TextBox
			std::string val = parm->getStringValue(i);
			if (parm->info().choiceCount > 0) {
				ofmsg("  choiceindex: %1%", %parm->info().choiceIndex);
				ofmsg("  choiceCount: %1%", %parm->info().choiceCount);
	// 			label->setUserTag(asset_name); // reference to the asset this param belongs to
				assetParamConts[asset_name].push_back(cont);
				Container* choiceCont = Container::create(Container::LayoutVertical, cont);
				for (int j = 0; j < parm->choices.size(); ++j) {
					Button* button = Button::create(choiceCont);
					button->setRadio(true);
					button->setCheckable(true);
					button->setChecked(parm->choices[j].value() == val);
					button->setText(parm->choices[j].label());
					ofmsg("  choice %1%: %2% %3%", %j %parm->choices[j].label() %parm->choices[j].value());
					button->setUIEventHandler(this);
					button->setUserData(label);
					widgetIdToParmId[button->getId()] = parm->info().id;
				}
			} else {
	// 			label->setUserTag(asset_name); // reference to the asset this param belongs to
				assetParamConts[asset_name].push_back(cont);
				TextBox* box = TextBox::create(cont);
				box->setText("X");
	// 			label->setUserData(miLabel); // reference to the label, for updating values
				box->setUIEventHandler(this);
				box->setUserData(label);

				widgetIdToParmId[box->getId()] = parm->info().id;
			}
		} else if (parm->info().type == HAPI_PARMTYPE_SEPARATOR) {
			label->setText("----------");
		}

		assetParamConts[asset_name].push_back(cont);
	}

}


// only run on master
// identical-looking menus get created on slaves
void HoudiniEngine::createMenu(const String& asset_name)
{
	// load only the params in the DA Folder
	// first find the index of the folder, then iterate through the list of params
	// from there onwards
	const char* daFolderName = "daFolder_0";

	hapi::Asset* myAsset = instancedHEAssetsByName[asset_name];

	if (myAsset == NULL) {
		ofwarn("No asset of name %1%", %asset_name);
	}

	Button* assetButton = Button::create(assetCont);
	assetButton->setRadio(true);
	assetButton->setChecked(true);
	assetButton->setText(asset_name);
	assetButton->setName(ostr("Asset %1%", %asset_name)); // TODO: use number here instead..
	assetButton->setUIEventHandler(this);

	std::map<std::string, hapi::Parm> parmMap = myAsset->parmMap();
	std::vector<hapi::Parm> parms = myAsset->parms();

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

	ui::Menu* menu = houdiniMenu;

// 	int i = daFolderIndex;
	int i = 0;

	Dictionary<int, Menu* > subMenus; // keep refs to submenus

	// help to parse params:
	// 1 - Parameters such as HAPI_PARMTYPE_FOLDERLIST's and HAPI_PARMTYPE_FOLDER's
	// 	have a child count associated with them. The child count is stored as the
	// 	int value of these parameters.
	// 2 - HAPI_PARMTYPE_FOLDERLIST's can only contain folders. HAPI_PARMTYPE_FOLDER's
	// 	cannot contain other HAPI_PARMTYPE_FOLDER's, only HAPI_PARMTYPE_FOLDERLIST's
	// 	and regular parameters.
	// 3 - When a HAPI_PARMTYPE_FOLDERLIST is encountered, we should dive into its
	// 	contents immediately, while everything else is traversed in a breadth first manner.

// 	houdiniCont->setUIEventHandler(this);

	while (i < parms.size()) {
		hapi::Parm* parm = &parms[i];
		ofmsg("PARM %1%: %2% %3% %4% %5%", %parm->label()
		                                   %parm->info().id
		                                   %parm->info().type
		                                   %parm->info().size
		                                   %parm->info().parentId
		);
		if (parm->info().parentId >= 0) {
			ofmsg("  Parent PARM %1%: %2% ", %(&parms[parm->info().parentId])->label()
			                                 %(&parms[parm->info().parentId])->info().id
			);
		}

		if (parm->info().type == HAPI_PARMTYPE_FOLDERLIST) { // only contains folders
// 			cout << "doing folderList " << parm->info().id << ", " << parm->info().size << endl;
// 			Container* myCont = Container::create(Container::LayoutVertical, houdiniCont);
			Container* myCont = Container::create(Container::LayoutHorizontal, houdiniCont); // for testing
			myCont->setVerticalAlign(Container::AlignTop);
			if (parm->info().parentId >= 0) {
				houdiniCont->removeChild(myCont);
				baseConts[parm->info().parentId]->addChild(myCont);
			}
			Label* label = Label::create(myCont);
			label->setName(ostr("%1%_label", %parm->name()));
			label->setText(parm->label());
			baseConts[parm->info().id] = myCont;
			myCont->setFillColor(Color("#404040"));
			myCont->setFillEnabled(true);

			i++;

			for (int j = 0; j < parm->info().size; ++j) {
				hapi::Parm* parm = &parms[i + j];
				// this is redundant, should always be folder type
				if (parm->info().type == HAPI_PARMTYPE_FOLDER) { // can contain folderLists and parms
// 					cout << "doing folder " << parm->info().id << ", " << parm->info().size << endl;
					Container* folderCont = Container::create(Container::LayoutVertical, myCont);
					baseConts[parm->info().id] = folderCont;

					Label* label = Label::create(folderCont);
					label->setName(ostr("%1%_label", %parm->name()));
					label->setText(parm->label());
					folderCont->setFillColor(Color("#808080"));
					folderCont->setFillEnabled(true);
				}
			}

			i += parm->info().size;

		} else { // its a parm
// 			cout << "doing param " << parm->info().id << endl;
			Container* myCont = Container::create(Container::LayoutHorizontal, houdiniCont);
			if (parm->info().parentId >= 0) {
				houdiniCont->removeChild(myCont);
				baseConts[parm->info().parentId]->addChild(myCont);
			}
			myCont->setFillColor(Color("#B0B040"));
			myCont->setFillEnabled(true);

			createParm(asset_name, myCont, parm);
		}

		i++;
	}
}
