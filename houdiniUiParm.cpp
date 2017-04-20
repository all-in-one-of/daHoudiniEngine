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

// help to parse params:
// 1 - Parameters such as HAPI_PARMTYPE_FOLDERLIST's and HAPI_PARMTYPE_FOLDER's
// 	have a child count associated with them. The child count is stored as the
// 	int value of these parameters.
// 2 - HAPI_PARMTYPE_FOLDERLIST's can only contain folders. HAPI_PARMTYPE_FOLDER's
// 	cannot contain other HAPI_PARMTYPE_FOLDER's, only HAPI_PARMTYPE_FOLDERLIST's
// 	and regular parameters.
// 3 - When a HAPI_PARMTYPE_FOLDERLIST is encountered, we should dive into its
// 	contents immediately, while everything else is traversed in a breadth first manner.

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
HoudiniUiParm* HoudiniUiParm::create(hapi::Parm parm, Container* cont) {
	HoudiniUiParm* hp = new HoudiniUiParm(parm, cont);
	return hp;
};

// TODO: make the alignemnt of parms nicer than it is now
// can this be refactored better?
HoudiniUiParm::HoudiniUiParm(hapi::Parm parm, Container* cont):
	myContainer(NULL),
	myLabel(NULL),
	myParm(parm)
{
	Container::Layout layout = Container::LayoutHorizontal;
	if (parm.info().type == HAPI_PARMTYPE_FOLDER) {
		layout = Container::LayoutVertical;
	}

	myContainer = Container::create(layout, cont);
	myContainer->setHorizontalAlign(Container::AlignRight);
	myContainer->setVerticalAlign(Container::AlignTop);

	ofmsg("PARM %1%: %2% s-%3% id-%4% name: %5%", %parm.label() %parm.info().type %parm.info().size
		%parm.info().parentId
		%parm.name()
	);

	myLabel = Label::create(myContainer);
	myLabel->setHorizontalAlign(Label::AlignRight);

	switch (parm.info().type) {
		// these shouldnt get renderd
		case HAPI_PARMTYPE_FOLDERLIST:
			myLabel->setText(ostr("FL:'%1%'/%2%", %parm.label() %parm.info().size));
			myContainer->setFillColor(Color("#404040"));
			break;
		case HAPI_PARMTYPE_FOLDER:
			myLabel->setText(ostr("F:'%1%'/%2%", %parm.label() %parm.info().size));
			myContainer->setFillColor(Color("#606060"));
			break;
		default:
			myLabel->setText(ostr("%1%", %parm.label()));
			myContainer->setFillColor(Color("#808080"));
			break;
	}
	myContainer->setFillEnabled(true);

	Container* newCont = Container::create(Container::LayoutVertical, myContainer);
	myContainer = newCont;

	// is there a nicer way to do this?
	if (parm.info().type == HAPI_PARMTYPE_INT ||
		parm.info().type == HAPI_PARMTYPE_FLOAT ||
		parm.info().type == HAPI_PARMTYPE_COLOR) {
		myContainer->setLayout(Container::LayoutGridHorizontal);
		myContainer->setGridRows(parm.info().size);
		myContainer->setGridColumns(2);
	}

	myContainer->setFillColor(Color("#A0A0A0"));
	myContainer->setFillEnabled(true);

	for (int i = 0; i < parm.info().size; ++i) {
		if (parm.info().type == HAPI_PARMTYPE_INT) {
			int val = parm.getIntValue(i);
			if (parm.info().choiceCount > 0) {
				switch (parm.info().choiceListType) {
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
				ofmsg("  choiceindex: %1% count: %2%", %parm.info().choiceIndex  %parm.info().choiceCount);
				// display as a selection menu
				if (parm.info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
					parm.info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
					parm.info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
					
					Container* choiceCont = Container::create(Container::LayoutVertical, myContainer);
					choiceCont->setHorizontalAlign(Container::AlignLeft);
					for (int j = 0; j < parm.choices.size(); ++j) {
						Button* button = Button::create(choiceCont);
						button->setRadio(true);
						button->setCheckable(true);
						button->setChecked(j == val);
						button->setText(parm.choices[j].label());
						button->getLabel()->setHorizontalAlign(Label::AlignLeft);
						ofmsg("  choice %1%: %2% (%3%) parentId: %4%", %j %parm.choices[j].label() %parm.choices[j].value()
						%parm.choices[j].info().parentParmId
						);
					}
				} else {// display as a text box which can be filled in by preset selections
					// same as if there was no parmChoice count
					// TODO: add the menu as well..
					myLabel->setText(parm.label() + " " + ostr("%1%", %val));
					Slider* slider = Slider::create(myContainer);
					if (parm.info().hasUIMin && parm.info().hasUIMax) {
						ofmsg("min %1% max %2%", %parm.info().UIMin %parm.info().UIMax);
						slider->setTicks(parm.info().UIMax - parm.info().UIMin + 1);
						slider->setValue(val - parm.info().UIMin);
					} else {
						if (parm.info().hasMin) ofmsg("min %1%", %parm.info().min);
						if (parm.info().hasMax) ofmsg("max %1%", %parm.info().max);
						slider->setTicks(parm.info().max + 1);
						slider->setValue(val);
					}
					slider->setDeferUpdate(true);
				}
			} else {
				// myLabel->setText(parm.label());
				Label* label = Label::create(myContainer);
				label->setText(ostr("%1%", %val));
				parmLabels.push_back(label);
				Slider* slider = Slider::create(myContainer);
				slider->setUserData((void*)(parmLabels.size() - 1));
				if (parm.info().hasUIMin && parm.info().hasUIMax) {
					ofmsg("min %1% max %2%", %parm.info().UIMin %parm.info().UIMax);
					slider->setTicks(parm.info().UIMax - parm.info().UIMin + 1);
					slider->setValue(val - parm.info().UIMin);
				} else {
					if (parm.info().hasMin) ofmsg("min %1%", %parm.info().min);
					if (parm.info().hasMax) ofmsg("max %1%", %parm.info().max);
					slider->setTicks(parm.info().max + 1);
					slider->setValue(val);
				}
				slider->setDeferUpdate(true);
			}
		} else if (parm.info().type == HAPI_PARMTYPE_TOGGLE) {
			int val = parm.getIntValue(i);
			Button* button = Button::create(myContainer);
			button->setText("X");
			button->setCheckable(true);
			button->setChecked(val);

		} else if (parm.info().type == HAPI_PARMTYPE_BUTTON) {
			int val = parm.getIntValue(i);
			Button* button = Button::create(myContainer);
			button->setText("X");
			button->setChecked(val);

		} else if (parm.info().type == HAPI_PARMTYPE_FLOAT ||
				   parm.info().type == HAPI_PARMTYPE_COLOR) {
			float val = parm.getFloatValue(i);
			myLabel->setText(parm.label());
			Label* label = Label::create(myContainer);
			label->setText(ostr("%1%", %val));
			parmLabels.push_back(label);
			Slider* slider = Slider::create(myContainer);
			slider->setUserData((void*)(parmLabels.size() - 1));
			if (parm.info().hasUIMin && parm.info().hasUIMax) {
				ofmsg("min %1% max %2%", %parm.info().UIMin %parm.info().UIMax);
				float sliderVal = (val - parm.info().UIMin) / (float) (parm.info().UIMax - parm.info().UIMin);
				slider->setValue(slider->getTicks() * sliderVal);
			} else {
				if (parm.info().hasMin) ofmsg("min %1%", %parm.info().min);
				if (parm.info().hasMax) ofmsg("max %1%", %parm.info().max);
				slider->setTicks(parm.info().max + 1);
				slider->setValue(val);
			}
			slider->setDeferUpdate(true);
		} else if (parm.info().type == HAPI_PARMTYPE_STRING ||
				   parm.info().type == HAPI_PARMTYPE_PATH_FILE ||
				   parm.info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
				   parm.info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) { // TODO: update with TextBox
			std::string val = parm.getStringValue(i);
			if (parm.info().choiceCount > 0) {
				switch (parm.info().choiceListType) {
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
					  %parm.info().choiceIndex  
					  %parm.info().choiceCount
				);
				// display as a selection menu
				if (parm.info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
					parm.info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
					parm.info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
					Container* choiceCont = Container::create(Container::LayoutVertical, myContainer);
					for (int j = 0; j < parm.choices.size(); ++j) {
						Button* button = Button::create(choiceCont);
						button->setRadio(true);
						button->setCheckable(true);
						button->setChecked(parm.choices[j].value() == val);
						button->setText(parm.choices[j].label());
						ofmsg("  choice %1%: %2% (%3%) parentId: %4%", %j %parm.choices[j].label() %parm.choices[j].value()
						%parm.choices[j].info().parentParmId
						);
					}
				} else {// display as a text box which can be filled in by preset selections
					// same as if there was no parmChoice count
					// TODO: add the menu as well..
					TextBox* box = TextBox::create(myContainer);
					box->setFont("fonts/segoeuimod.ttf 14");
					box->setText(val);
				}
			} else {
				TextBox* box = TextBox::create(myContainer);
				box->setFont("fonts/segoeuimod.ttf 14");
				box->setText(val);
			}
		} else if (parm.info().type == HAPI_PARMTYPE_SEPARATOR) {
			myLabel->setText("----------");
		} else if (parm.info().type == HAPI_PARMTYPE_MULTIPARMLIST) {
			ofmsg("this is a multiparm, instance length %1%, instance count %2%, instance offset %3%",
				  %parm.info().instanceLength
				  %parm.info().instanceCount
				  %parm.info().instanceStartOffset
			);
			myLabel->setText(parm.label() + " " + ostr("%1%", %parm.info().instanceCount));
			// TODO: Add/Remove buttons, number of items, clear button?
			// multiParmConts[parm.name()] = cont;
			// add another container to contain label and buttons
			Container* multiParmButtonCont = Container::create(Container::LayoutVertical, myContainer);
			ofmsg("Multiparm cont %1%: id-%2%",
				  %multiParmButtonCont->getName()
				  %multiParmButtonCont->getId()
			);
			myContainer->removeChild(myLabel);
			multiParmButtonCont->addChild(myLabel);
			Button* addButton = Button::create(multiParmButtonCont);
			addButton->setName(ostr("%1%_add", %parm.name()));
			addButton->setText("+");
			Button* remButton = Button::create(multiParmButtonCont);
			remButton->setName(ostr("%1%_rem", %parm.name()));
			remButton->setText("-");
			Button* clrButton = Button::create(multiParmButtonCont);
			clrButton->setName(ostr("%1%_clr", %parm.name()));
			clrButton->setText("Clear");
		}
	}
 }

HoudiniUiParm::~HoudiniUiParm() {
	// myContainer->setContainer(NULL);
}

void HoudiniUiParm::handleEvent(const Event& evt) {
	if(evt.getServiceType() != Service::Ui) return;
	if(evt.getType() != Event::Click && 
	   evt.getType() != Event::Toggle && 
	   evt.getType() != Event::ChangeValue) return;

	Widget* myWidget = Widget::getSource<Widget>(evt);
	String t;

	HoudiniEngine* he = HoudiniEngine::instance();

	if (evt.getType() == Event::Click) {
		t = "Click";
	}
	if (evt.getType() == Event::Toggle) {
		t = "Toggle";
	}
	if (evt.getType() == Event::ChangeValue) {
		t = "ChangeValue";
	}
	if (evt.getType() == Event::Click ||
		evt.getType() == Event::Toggle ||
		evt.getType() == Event::ChangeValue) {
		ofmsg("HUIP Event! %1% source:%2%", %t %myWidget->getName());
		Slider* slider = dynamic_cast<Slider*>(myWidget);

		if (slider == NULL) return;

		ofmsg("Slider value: %1%", %slider->getValue());
		ofmsg("Parm type %1%: %2%", %myParm.label() %myParm.info().type);

		if (myParm.info().type == HAPI_PARMTYPE_INT) {
			int val = slider->getValue();
			if (myParm.info().hasUIMin && myParm.info().hasUIMax) { // offset value based on the UI range
				val = myParm.info().UIMin + val;
			}
			ofmsg("Int value set to %1%", %val);
			void * data = slider->getUserData();
			int index = *((int *)&data);
			parmLabels[index]->setText(ostr("%1%", %val));
			// myLabel->setText(ostr("%1%: %2%", %myParm.label() %val));

			// static_cast<Label*>(slider->getUserData())->setText(ostr("%1% %2%", %myParm.label() %val));
			myParm.setIntValue(0, val);

		} else if (myParm.info().type == HAPI_PARMTYPE_FLOAT ||
					myParm.info().type == HAPI_PARMTYPE_COLOR) {
			float val = slider->getValue();
			if (myParm.info().hasUIMin && myParm.info().hasUIMax) {
				val = ((float) myParm.info().UIMin) + (val * (float) (myParm.info().UIMax - myParm.info().UIMin));
			}
			ofmsg("Float value set to %1%", %val);
			void * data = slider->getUserData();
			int index = *((int *)&data);
			parmLabels[index]->setText(ostr("%1%", %(val / ((float)slider->getTicks()))));
			// myLabel->setText(ostr("%1%: %2%", %myParm.label() %(val / ((float)slider->getTicks()))));
			// static_cast<Label*>(slider->getUserData())->setText(ostr("%1% %2%", %myParm.label() %(val / ((float)slider->getTicks()))));
			myParm.setFloatValue(0, val / ((float)slider->getTicks()));
		}
		
		he->cook();
	}

}
