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
// Helper function to remove a container that contains child containers 
void HoudiniEngine::removeConts(Container* cont) {
	if (cont == NULL) {
		return;
	}
	for (int i = cont->getNumChildren() - 1; i >= 0 ; --i) {
		Widget* blah = cont->getChildByIndex(i);
		Container *blahCont = dynamic_cast<Container*> (blah);
		blah->setNavigationEnabled(false);
		cont->removeChild(blah);
		if (blahCont != NULL) {
			ofmsg("container? %1% id: %2%", 
				  %blahCont->getName()
				  %blahCont->getId()
				 );
			
			UiModule::instance()->getUi()->addChild(blahCont);
			removeTheseWidgets.push_back(blahCont);
		}
		ofmsg("refcount for %1%: %2%, id: %3%", %blah->getName() %blah->refCount() %blah->getId());
	}
}

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

	while (!removeTheseWidgets.empty()) {
		removeTheseWidgets.back()->getContainer()->removeChild(removeTheseWidgets.back());
		removeTheseWidgets.pop_back();
	}
	
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

	// do it if source is button and event is toggle or click
	if (myWidget != NULL) {

		// choice of asset?
		if (strlen(myWidget->getName().c_str()) > 5 &&
			StringUtils::startsWith(myWidget->getName(), "Asset", false)) {
			// currentAssetName = ostr("%1%", %myWidget->getName().substr(6));
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
		}
	}

}
