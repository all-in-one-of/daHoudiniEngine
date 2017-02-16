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
	this file contains event handling methods for parameter changes and asset interaction

******************************************************************************/

#include <daHoudiniEngine/daHEngine.h>
#include <daHoudiniEngine/houdiniGeometry.h>

using namespace houdiniEngine;

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::update(const UpdateContext& context)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::onMenuItemEvent(MenuItem* mi)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::onSelectedChanged(SceneNode* source, bool value)
{
}

// TODO: expose to python to write my own event handler
// TODO: widgetIdToParmId needs to account for multiple assets, especially the current one
///////////////////////////////////////////////////////////////////////////////
void HoudiniEngine::handleEvent(const Event& evt)
{

	bool doUpdate = false;
	// only handle these events on the master
	if (!SystemManager::instance()->isMaster()) return;

	// a few UI Events to consider:
	// Service::UI types:
	// Down, Up, ChangeValue for Sliders, I only want ChangeValue events
	// Toggle for Checkable
	// Click for Button
	// stop if it's not a Ui Service event
	if(evt.getServiceType() != Service::Ui || evt.isProcessed()) {
		return;
	}


	Widget* myWidget = Widget::getSource<Widget>(evt);

	ofmsg("widget name: '%1%'", %myWidget->getName());
	ofmsg("current asset is %1%", %currentAssetName);

	// do it if source is button and event is toggle or click
	if (myWidget != NULL) {

		// choice of asset?
		if (strlen(myWidget->getName().c_str()) > 5 &&
			ostr("%1%", %myWidget->getName().substr(0, 5)) == "Asset") {
			currentAssetName = ostr("%1%", %myWidget->getName().substr(6));
			void * data = myWidget->getUserData();
			int assetIndex = *((int *)&data);
			// children actually removed on update.. so do this to mark for removal
			for (int i = 1; i < houdiniCont->getNumChildren(); ++i) {
				Widget* endRef = houdiniCont->getChildByIndex(i);
				if (endRef != NULL) {
					houdiniCont->removeChild(endRef);
				}
			}
			houdiniCont->addChild(assetConts[assetIndex]);
			evt.setProcessed();
			return;
		} else

		// folder toggling?
		if (strlen(myWidget->getName().c_str()) > 12 &&
			ostr("%1%", %myWidget->getName().substr(0, 12)) == "FolderButton") {
			// NOTE: myFolderCont may not have a container parent
			Container* myFolderCont = static_cast<Container*>(myWidget->getUserData());
			if (myFolderCont != NULL) {
				ofmsg("Target Folder of widget %1%: %2%", %myWidget->getName() %myFolderCont->getName());
				if (myFolderCont->getContainer() != NULL) {
					ofmsg("num children in folderCont: %1%", %myFolderCont->getContainer()->getNumChildren());
				}

				// stored by button id!
				Container* folderCont = folderListContents[myWidget->getId()];
				// Container* folderCont = myFolderCont->getContainer()->getContainer();

				for (int i = 0; i < folderCont->getNumChildren(); ++i) {
					ofmsg("conts in folderCont: %1%: %2%", %folderCont->getNumChildren() %folderCont->getChildByIndex(0)->getName());
					folderCont->getChildByIndex(folderCont->getNumChildren() - 1)->setVisible(false);
					stagingCont->addChild(folderCont->getChildByIndex(folderCont->getNumChildren() - 1));
					folderCont->removeChild(folderCont->getChildByIndex(folderCont->getNumChildren() - 1));
				}

				myFolderCont->setVisible(true);
				stagingCont->removeChild(myFolderCont);
				folderCont->addChild(myFolderCont);
			} else {
				ofmsg("no parent container for %1%", %myWidget->getName());
			}
			evt.setProcessed();
			return;
		}


		doUpdate = evt.getType() == Event::Toggle || evt.getType() == Event::Click;
	}

	omsg("about to get source");
	myWidget = Widget::getSource<Widget>(evt);
	ofmsg("got source %1%", %myWidget->getName());

	// problem was bug in Slider.cpp, myValueChanged was never set back to false. Fixed
	// stop if widget source is not a slider or event type is not changeValue
	if (myWidget != NULL && evt.getType() == Event::ChangeValue) {
		myWidget = myWidget;
		doUpdate = true;
	}

	// do this anyway.. for now
// 	if (!doUpdate) {
// 		return;
// 	}

	// otherwise, do something!
// 	ofmsg("Widget source: %1%, id: %2%. Do a cook call here..",
// 		  %Widget::getSource<Widget>(evt)->getName()
// 		  %parmId
// 	);
	// TODO: do incremental checks for what i need to update:
	// HAPI_ObjectInfo::hasTransformChanged
	// HAPI_ObjectInfo::haveGeosChanged
	// HAPI_GeoInfo::hasGeoChanged
	// HAPI_GeoInfo::hasMaterialChanged

	omsg("got here");

	String asset_name = currentAssetName;
	hapi::Asset* myAsset = instancedHEAssetsByName[asset_name];

	if (myAsset == NULL) {
		ofwarn("No instanced asset %1%", %asset_name);
		return;
	} else {
		ofmsg("Asset is %1%", %asset_name);
	}

	UiModule::instance()->activateWidget(myWidget);
	
	ofmsg("Active widget %1%? %2%", %myWidget->getName() %myWidget->isActive());
	
	// the link between widget and parmId
	int myParmId = widgetIdToParmId[myWidget->getId()];

	// this seems unreliable when referring to parm->choices. 
	// pointer memory location differs and I get corrupt data
	// therefore use absolute references for it
	hapi::Parm* parm = &(myAsset->parms()[myParmId]);

	ofmsg("PARM %1%: %2% s-%3% id-%4%",
		  %parm->label()
		  %parm->info().type
		  %parm->info().size
		  %parm->name() );

	if (parm->info().choiceCount > 0) {
		ofmsg("I'm a choice %1%: %2%", %parm->info().choiceCount %parm->name());
		Button* button = dynamic_cast<Button*>(myWidget);
		if (parm->info().type == HAPI_PARMTYPE_INT) {
			int val = 0;
			Container* parentCont = button->getContainer();
			for (int i = 0; i < parentCont->getNumChildren(); ++i) {
				if (parentCont->getChildByIndex(i)->getName() == button->getName()) {
					val = i;
					break;
				}
			}
			ofmsg("Value set to %1%", %val);
			parm->setIntValue(0, val);
		} else if (parm->info().type == HAPI_PARMTYPE_STRING) {
			Container* parentCont = button->getContainer();
			int myIndex = 0;
			// find the index by matching container name to content
			for (int i = 0; i < parentCont->getNumChildren(); ++i) {
				if (parentCont->getChildByIndex(i)->getName() == button->getName()) {
					myIndex = i;
					break;
				}
			}

			// BUG *parm changes! I'm doing something that causes the pointer location to change..
			// direct reference to choice item works, (ie myAsset->parms()[myParmId])
			// but not through *parm.
			String val = ostr("%1%", %myAsset->parms()[myParmId].choices[myIndex].value());
			ofmsg("Value set to %1%", %val);
			parm->setStringValue(0, val.c_str());
		}

	} else
	if (parm->info().type == HAPI_PARMTYPE_INT) {
		Slider* slider = dynamic_cast<Slider*>(myWidget);
		if (slider != NULL) {
			int val = slider->getValue();
			if (parm->info().hasUIMin && parm->info().hasUIMax) { // offset value based on the UI range
				val = parm->info().UIMin + val;
			}
			ofmsg("Value set to %1%", %val);
			static_cast<Label*>(slider->getUserData())->setText(ostr("%1% %2%", %parm->label() %val));
			parm->setIntValue(0, val);
		}

	} else if (parm->info().type == HAPI_PARMTYPE_FLOAT ||
			   parm->info().type == HAPI_PARMTYPE_COLOR) {
		Slider* slider = dynamic_cast<Slider*>(myWidget);
		if (slider != NULL) {
			float val = slider->getValue();
			if (parm->info().hasUIMin && parm->info().hasUIMax) {
				val = ((float) parm->info().UIMin) + (val * (float) (parm->info().UIMax - parm->info().UIMin));
			}
			ofmsg("Value set to %1%", %val);
			static_cast<Label*>(slider->getUserData())->setText(ostr("%1% %2%", %parm->label() %(val / ((float)slider->getTicks()))));
			parm->setFloatValue(0, val / ((float)slider->getTicks()));
		}

	} else if (parm->info().type == HAPI_PARMTYPE_TOGGLE ||
			   parm->info().type == HAPI_PARMTYPE_BUTTON) {
		Button* button = dynamic_cast<Button*>(myWidget);
		if (button != NULL) {
			ofmsg("Value set to %1%", %button->isChecked());
			// don't add value to label, already visible with button checked-ness
			parm->setIntValue(0, button->isChecked());
		}
	} else if (parm->info().type == HAPI_PARMTYPE_STRING ||
			parm->info().type == HAPI_PARMTYPE_PATH_FILE ||
			parm->info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
			parm->info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) {
		TextBox* tb = dynamic_cast<TextBox*>(myWidget);
		if (tb != NULL) {
			ofmsg("Value set to %1%", %tb->getText());
			parm->setStringValue(0, tb->getText().c_str());
		}

	}

	evt.setProcessed();

	myAsset->cook();
	wait_for_cook();

	process_assets(*myAsset);

	updateGeos = true;


// this is stuff to apply changes to parms
/*
	if (!SystemManager::instance()->isMaster())
	{
		return;
	}

	// TODO: change this to suit running on the master only, but callable from slave..
	bool update = false;


	// user tag is of form '<parmName> <parmIndex>'
	// eg: 'size 2.1' for a size vector
	int z = mi->getUserTag().find_last_of(" "); // index of space

	// TODO: do a null check here.. may break
	String asset_name = ((MenuItem*)mi->getUserData())->getUserTag();
	hapi::Asset* myAsset = instancedHEAssetsByName[asset_name];

	std::map<std::string, hapi::Parm> parmMap = myAsset->parmMap();

	hapi::Parm* parm = &(parmMap[mi->getUserTag().substr(0, z)]);
	int index = atoi(mi->getUserTag().substr(z + 1).c_str());

	if (myAsset == NULL) {
		ofwarn("No instanced asset %1%", %asset_name);
		return;
	}

	if (parm->info().type == HAPI_PARMTYPE_INT) {
		parm->setIntValue(index, mi->getSlider()->getValue());

		((MenuItem*)mi->getUserData())->setText(parm->label() + ": " +
		ostr("%1%", %mi->getSlider()->getValue()));

		update = true;

	} else if (parm->info().type == HAPI_PARMTYPE_TOGGLE) {
		parm->setIntValue(index, mi->getButton()->isChecked());

		((MenuItem*)mi->getUserData())->setText(parm->label());

		update = true;

	} else if (parm->info().type == HAPI_PARMTYPE_FLOAT) {
		parm->setFloatValue(index, 0.01 * mi->getSlider()->getValue());

		((MenuItem*)mi->getUserData())->setText(parm->label() + ": " +
		ostr("%1%", %(mi->getSlider()->getValue() * 0.01)));

		update = true;

	}

	if (update) {

		// TODO: do incremental checks for what i need to update:
		// HAPI_ObjectInfo::hasTransformChanged
		// HAPI_ObjectInfo::haveGeosChanged
		// HAPI_GeoInfo::hasGeoChanged
		// HAPI_GeoInfo::hasMaterialChanged


		myAsset->cook();
		wait_for_cook();

		process_assets(*myAsset);

		updateGeos = true;
	}

*/
}
