#include <daHoudiniEngine/UI/houdiniUiParm.h>

using namespace houdiniEngine;

Dictionary<int, HoudiniUiParm* > HoudiniUiParm::myUiParms;

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
// HAPI_PARMTYPE_NODE,			= 10
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
	baseContainer(NULL),
	myLabel(NULL),
	myParm(parm),
	newCont(NULL),
	choiceCont(NULL),
	multiParmButtonCont(NULL),
	mySlider(NULL)
{
	// ofmsg("Constructing uiParm %1%", %myParm.info().id);
	Container::Layout 			layout = Container::LayoutHorizontal;
	Container::HorizontalAlign 	hAlign = Container::AlignCenter;
	Container::VerticalAlign 	vAlign = Container::AlignTop;
	// if (myParm.info().type == HAPI_PARMTYPE_FOLDERLIST) {
	// 	layout = Container::LayoutHorizontal;
	// 	hAlign = Container::AlignRight;
	// 	vAlign = Container::AlignTop;
	// }
	// if (myParm.info().type == HAPI_PARMTYPE_FOLDER) {
	// 	layout = Container::LayoutVertical;
	// 	hAlign = Container::AlignRight;
	// 	vAlign = Container::AlignTop;
	// }

	// folders aligned vertically
	// everything else horizontally
	baseContainer = Container::create(layout, cont);
	baseContainer->setStyleValue("border", "1 #ff0000");
	baseContainer->setHorizontalAlign(hAlign);
	baseContainer->setVerticalAlign(vAlign);

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
	// debug
	ofmsg("PARM %1% (%2%) %3%x%4% name: %5% (%6%)", 
		%myParm.info().id
		%myParm.info().parentId
		%t 
		%myParm.info().size
		%myParm.name()
		%myParm.label() 
	);

	myLabel = Label::create(baseContainer);
	myLabel->setHorizontalAlign(Label::AlignRight);

	switch (myParm.info().type) {
		// these shouldnt get renderd
		case HAPI_PARMTYPE_FOLDERLIST:
			myLabel->setText(ostr("FL:'%1%'/%2%", %myParm.label() %myParm.info().size));
			baseContainer->setFillColor(Color("#404040"));
			break;
		case HAPI_PARMTYPE_FOLDER:
			myLabel->setText(ostr("F:'%1%'/%2%", %myParm.label() %myParm.info().size));
			baseContainer->setFillColor(Color("#606060"));
			// start to add windows for containers to reduce screen space
			if (false) {
			mySlider = Slider::create(baseContainer);
			mySlider->setName(ostr("%1%_Slider", %myParm.name()));
			mySlider->setTicks(101);
			baseContainer->setMinimumSize(Vector2f(600, 20));
			baseContainer->setMaximumSize(Vector2f(900, 900));
			baseContainer->setSize(Vector2f(900, 600));
			baseContainer->setClippingEnabled(true);
			baseContainer->setAutosize(false);
			}
			break;
		default:
			myLabel->setText(ostr("%1%", %myParm.label()));
			baseContainer->setFillColor(Color("#808080"));
			break;
	}
	baseContainer->setFillEnabled(true);

	if (myParm.info().type == HAPI_PARMTYPE_FOLDER ||
		myParm.info().type == HAPI_PARMTYPE_MULTIPARMLIST) {
		layout = Container::LayoutVertical;
		hAlign = Container::AlignRight;
		vAlign = Container::AlignTop;
	} else {
		layout = Container::LayoutHorizontal;
		hAlign = Container::AlignLeft;
		vAlign = Container::AlignTop;
	}
	newCont = Container::create(layout, baseContainer);

	// newCont->setSizeAnchor(Vector2f(0, -1)); // -1 to disable
	// newCont->setSizeAnchorEnabled(true);

	newCont->setHorizontalAlign(hAlign);
	newCont->setVerticalAlign(vAlign);
	// newCont->setHorizontalAlign(hAlign);
	// newCont->setVerticalAlign(vAlign);
	newCont->setStyleValue("border", "1 #0000ff");
	Container* myContainer = newCont;

	// is there a nicer way to do this?
	if (myParm.info().type == HAPI_PARMTYPE_INT ||
		myParm.info().type == HAPI_PARMTYPE_FLOAT ||
		myParm.info().type == HAPI_PARMTYPE_COLOR) {
		myContainer->setLayout(Container::LayoutGridHorizontal);
		myContainer->setGridRows(myParm.info().size);
		myContainer->setGridColumns(2);
	}

	myContainer->setFillColor(Color("#A0A0A0"));
	myContainer->setFillEnabled(true);

	for (int i = 0; i < myParm.info().size; ++i) {
		if (myParm.info().type == HAPI_PARMTYPE_INT) {
			int val = myParm.getIntValue(i);
			if (myParm.info().choiceCount > 0) {
				switch (myParm.info().choiceListType) {
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
				ofmsg("  choiceindex: %1% count: %2%", %myParm.info().choiceIndex  %myParm.info().choiceCount);
				// display as a selection menu
				// display button of currently selected item, then when pressed, a menu of
				// selectable items is shown
				// then when chosen, original button is updated
				if (myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
					myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
					myParm.info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
					choiceCont = Container::create(Container::LayoutHorizontal, myContainer);
					choiceCont->setStyleValue("border", "1 #00ff00");
					choiceCont->setHorizontalAlign(Container::AlignLeft);
					for (int j = 0; j < myParm.choices.size(); ++j) {
						Button* button = Button::create(choiceCont);
						button->setRadio(true);
						button->setCheckable(true);
						button->setChecked(j == val);
						button->setText(myParm.choices[j].label());
						button->getLabel()->setHorizontalAlign(Label::AlignLeft);
						ofmsg("  choice %1% (%2%): %3% (%4%)", 
							%j 
							%myParm.choices[j].info().parentParmId
							%myParm.choices[j].value()
							%myParm.choices[j].label() 
						);
					}
				} else {// display as a text box which can be filled in by preset selections
					// same as if there was no parmChoice count
					// TODO: add the menu as well..
					myLabel->setText(myParm.label() + " " + ostr("%1%", %val));
					Slider* slider = Slider::create(myContainer);
					if (myParm.info().hasUIMin && myParm.info().hasUIMax) {
						ofmsg("  UImin %1% UImax %2%", %myParm.info().UIMin %myParm.info().UIMax);
						slider->setTicks(myParm.info().UIMax - myParm.info().UIMin + 1);
						slider->setValue(val - myParm.info().UIMin);
					} else {
						if (myParm.info().hasMin) ofmsg("  min %1%", %myParm.info().min);
						if (myParm.info().hasMax) ofmsg("  max %1%", %myParm.info().max);
						slider->setTicks(myParm.info().max + 1);
						slider->setValue(val);
					}
					slider->setDeferUpdate(true);
				}
			} else {
				// myLabel->setText(myParm.label());
				Label* label = Label::create(myContainer);
				label->setText(ostr("%1%", %val));
				parmLabels.push_back(label);
				Slider* slider = Slider::create(myContainer);
				slider->setUserData((void*)(parmLabels.size() - 1));
				if (myParm.info().hasUIMin && myParm.info().hasUIMax) {
					ofmsg("  UImin %1% UImax %2%", %myParm.info().UIMin %myParm.info().UIMax);
					slider->setTicks(myParm.info().UIMax - myParm.info().UIMin + 1);
					slider->setValue(val - myParm.info().UIMin);
				} else {
					if (myParm.info().hasMin) ofmsg("  min %1%", %myParm.info().min);
					if (myParm.info().hasMax) ofmsg("  max %1%", %myParm.info().max);
					slider->setTicks(myParm.info().max + 1);
					slider->setValue(val);
				}
				slider->setDeferUpdate(true);
			}
		} else if (myParm.info().type == HAPI_PARMTYPE_TOGGLE) {
			int val = myParm.getIntValue(i);
			Button* button = Button::create(myContainer);
			button->setText("X");
			button->setCheckable(true);
			button->setChecked(val);

		} else if (myParm.info().type == HAPI_PARMTYPE_BUTTON) {
			int val = myParm.getIntValue(i);
			Button* button = Button::create(myContainer);
			button->setText("X");
			button->setChecked(val);

		} else if (myParm.info().type == HAPI_PARMTYPE_FLOAT ||
				   myParm.info().type == HAPI_PARMTYPE_COLOR) {
			float val = myParm.getFloatValue(i);
			myLabel->setText(myParm.label());
			Label* label = Label::create(myContainer);
			label->setText(ostr("%1%", %val));
			parmLabels.push_back(label);
			Slider* slider = Slider::create(myContainer);
			slider->setUserData((void*)(parmLabels.size() - 1));
			if (myParm.info().hasUIMin && myParm.info().hasUIMax) {
				ofmsg("  UImin %1% UImax %2%", %myParm.info().UIMin %myParm.info().UIMax);
				float sliderVal = (val - myParm.info().UIMin) / (float) (myParm.info().UIMax - myParm.info().UIMin);
				slider->setValue(slider->getTicks() * sliderVal);
			} else {
				if (myParm.info().hasMin) ofmsg("  min %1%", %myParm.info().min);
				if (myParm.info().hasMax) ofmsg("  max %1%", %myParm.info().max);
				slider->setTicks(myParm.info().max + 1);
				slider->setValue(val);
			}
			slider->setDeferUpdate(true);
		} else if (myParm.info().type == HAPI_PARMTYPE_STRING ||
				   myParm.info().type == HAPI_PARMTYPE_PATH_FILE ||
				   myParm.info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
				   myParm.info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) { // TODO: update with TextBox
			std::string val = myParm.getStringValue(i);
			if (myParm.info().choiceCount > 0) {
				switch (myParm.info().choiceListType) {
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
					  %myParm.info().choiceIndex  
					  %myParm.info().choiceCount
				);
				// display as a selection menu
				if (myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
					myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
					myParm.info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
					choiceCont = Container::create(Container::LayoutVertical, myContainer);
					choiceCont->setStyleValue("border", "1 #00ff00");
					choiceCont->setHorizontalAlign(Container::AlignLeft);
					for (int j = 0; j < myParm.choices.size(); ++j) {
						Button* button = Button::create(choiceCont);
						button->setRadio(true);
						button->setCheckable(true);
						// segfaulting on this line.. myParm.choices[j] doesn't exist?
						button->setChecked(myParm.choices[j].value() == val);
						button->setText(myParm.choices[j].label());
						ofmsg("  choice %1% (%2%): %3% (%4%)", 
							%j 
							%myParm.choices[j].info().parentParmId
							%myParm.choices[j].value()
							%myParm.choices[j].label() 
						);
					}
				} else {// display as a text box which can be filled in by preset selections
					// same as if there was no parmChoice count
					// TODO: add the menu as well..
					TextBox* box = TextBox::create(myContainer);
					box->setWidth(200.0);
					box->setFont("fonts/segoeuimod.ttf 14");
					box->setText(val);
				}
			} else {
				TextBox* box = TextBox::create(myContainer);
				box->setWidth(200.0);
				box->setFont("fonts/segoeuimod.ttf 14");
				box->setText(val);
			}
		} else if (myParm.info().type == HAPI_PARMTYPE_SEPARATOR) {
			myLabel->setText("----------");
		} else if (myParm.info().type == HAPI_PARMTYPE_MULTIPARMLIST) {
			ofmsg("this is a multiparm, instance length %1%, instance count %2%, instance offset %3%",
				  %myParm.info().instanceLength
				  %myParm.info().instanceCount
				  %myParm.info().instanceStartOffset
			);
			myLabel->setText(myParm.label() + " " + ostr("%1%", %myParm.info().instanceCount));
			// TODO: Add/Remove buttons, number of items, clear button?
			// multiParmConts[myParm.name()] = cont;
			// add another container to contain label and buttons
			multiParmButtonCont = Container::create(Container::LayoutVertical, baseContainer);
			ofmsg("Multiparm cont %1%: id-%2%",
				  %multiParmButtonCont->getName()
				  %multiParmButtonCont->getId()
			);
			myContainer->removeChild(myLabel);
			multiParmButtonCont->addChild(myLabel);
			Button* addButton = Button::create(multiParmButtonCont);
			addButton->setName(ostr("%1%_add", %myParm.name()));
			addButton->setText("+");
			Button* remButton = Button::create(multiParmButtonCont);
			remButton->setName(ostr("%1%_rem", %myParm.name()));
			remButton->setText("-");
			Button* clrButton = Button::create(multiParmButtonCont);
			clrButton->setName(ostr("%1%_clr", %myParm.name()));
			clrButton->setText("Clear");
		}
	}

	// add parmid to label
	myLabel->setText(ostr("%1%: %2%", %myParm.info().id %myLabel->getText()));

	myUiParms[myParm.info().id] = this;
 }

HoudiniUiParm::~HoudiniUiParm() {
	// myContainer->setContainer(NULL);
	// Debug output
	// ofmsg("~Destructing uiParm %1%", %myParm.info().id);
	// ofmsg("    refcounts: 	baseContainer %1% multiParmButtonCont %2% choiceCont %3% newCont %4% myLabel %5%",
	// 	%(baseContainer != NULL ? ostr("%1%", %baseContainer->refCount()) : "-")
	// 	%(multiParmButtonCont != NULL ? ostr("%1%", %multiParmButtonCont->refCount()) : "-")
	// 	%(choiceCont != NULL ? ostr("%1%", %choiceCont->refCount()) : "-")
	// 	%(newCont != NULL ? ostr("%1%", %newCont->refCount()) : "-")
	// 	%(myLabel != NULL ? ostr("%1%", %myLabel->refCount()) : "-")
	// );

	// for (int i = 0; i < myContainer->getNumChildren(); ++i) {
	// 	Container* cont = dynamic_cast<Container*>(myContainer->getChildByIndex(i));
	// 	if (cont != NULL) {
	// 		for (int j = 0; j < cont->getNumChildren(); ++j) {
	// 			Container* cont2 = dynamic_cast<Container*>(cont->getChildByIndex(j));
	// 			if (cont2 != NULL) {
	// 				for (int k = 0; k < cont2->getNumChildren(); ++k) {
	// 					cont2->removeChild(cont2->getChildByIndex(k));
	// 				}
	// 			}
	// 			cont->removeChild(cont->getChildByIndex(j));
	// 		}
	// 	}
	// 	myContainer->removeChild(myContainer->getChildByIndex(i));
	// }
	// foreach(Label* label, parmLabels) {
	// 	newCont->removeChild(label);
	// 	label = NULL;
	// }
	// parmLabels.clear();
	// omsg("parmlabels cleared");
	// nicely remove from parent?
	if (myParm.info().parentId >=0) {
		if (myUiParms[myParm.info().parentId] != NULL) {
			if (myUiParms[myParm.info().parentId]->getContents() != NULL) {
				myUiParms[myParm.info().parentId]->getContents()->removeChild(baseContainer);
			}
		}
	} else {
		if (baseContainer->getContainer() != NULL) {
			baseContainer->getContainer()->removeChild(baseContainer);
		}
	}

	if (myLabel != NULL) baseContainer->removeChild(myLabel);
	if (newCont != NULL) baseContainer->removeChild(newCont);
	if (choiceCont != NULL) {
		baseContainer->removeChild(choiceCont);
		for (int i = 0; i < choiceCont->getNumChildren(); ++i) {
			Widget* w = choiceCont->getChildByIndex(i);
			w->setNavigationEnabled(false);
			choiceCont->removeChild(w);
		}
	}
	if (multiParmButtonCont != NULL) baseContainer->removeChild(multiParmButtonCont);
	baseContainer = NULL;
	multiParmButtonCont = NULL;
	choiceCont = NULL;
	for (int i = 0; i < newCont->getNumChildren(); ++i) {
		Widget* w = newCont->getChildByIndex(i);
		w->setNavigationEnabled(false);
		newCont->removeChild(w);
	}
	// omsg("newCont children removed");
	foreach(Label* label, parmLabels) {
		Container* c = label->getContainer();
		if (c != NULL) { c->removeChild(label); }
	}
	parmLabels.clear();
	// omsg("parmlabels cleared");

	newCont = NULL;
	myLabel = NULL;
	myUiParms.erase(myParm.info().id);
	// ReferenceType::printObjCounts();
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
		Button* button = dynamic_cast<Button*>(myWidget);

		if (button != NULL) {
			ofmsg("Button value: %1%", %button->isChecked());
		}
		if (slider != NULL) {
			ofmsg("Slider value: %1%", %slider->getValue());
		}
		TextBox* textBox = dynamic_cast<TextBox*>(myWidget);
		if (textBox != NULL) {
			ofmsg("TextBox text: %1%", %textBox->getText());
		}

		ofmsg("Parm type %1%: %2%", %myParm.label() %myParm.info().type);

		if (slider != NULL && StringUtils::endsWith(myWidget->getName(), "_slider")) {
			ofmsg("slider val: %1%", %slider->getValue());
			evt.setProcessed();
			return;
		}

		if (myParm.info().type == HAPI_PARMTYPE_INT) {
			if (myParm.info().choiceCount > 0) {
				Container* choiceCont = button->getContainer();
				if (choiceCont == NULL) {
					ofmsg("missing parent container for %1%!", %myParm.name());
					return;
				}
				for (int i = 0; i < choiceCont->getNumChildren(); ++i) {
					if (choiceCont->getChildByIndex(i)->getName() == button->getName()) {
						myParm.setIntValue(0, i);
						break;
					}
				}
			} else {
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
			}
		} else if (myParm.info().type == HAPI_PARMTYPE_TOGGLE) {
			bool val = button->isChecked();
			void * data = button->getUserData();
			int index = *((int *)&data);
			myParm.setIntValue(0, (int) val);
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
		} else if (myParm.info().type == HAPI_PARMTYPE_STRING ||
				   myParm.info().type == HAPI_PARMTYPE_PATH_FILE ||
				   myParm.info().type == HAPI_PARMTYPE_PATH_FILE_GEO	||
				   myParm.info().type == HAPI_PARMTYPE_PATH_FILE_IMAGE) { // TODO: update with TextBox
			if (myParm.info().choiceCount > 0) {
				switch (myParm.info().choiceListType) {
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
					  %myParm.info().choiceIndex
					  %myParm.info().choiceCount
				);
				// TODO: code for selecting an item from a list

				// what sort of menu display is it?
				if (myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NONE ||
					myParm.info().choiceListType == HAPI_CHOICELISTTYPE_NORMAL ||
					myParm.info().choiceListType == HAPI_CHOICELISTTYPE_MINI) {
				} else {// display as a text box which can be filled in by preset selections
				}
				Container* choiceCont = button->getContainer();
				if (choiceCont == NULL) {
					ofmsg("missing parent container for %1%!", %myParm.name());
					return;
				}
				for (int i = 0; i < choiceCont->getNumChildren(); ++i) {
					if (choiceCont->getChildByIndex(i)->getName() == button->getName()) {
						myParm.setStringValue(0, myParm.choices[i].value().c_str());
						break;
					}
				}
			// just a text box..
			} else {
				std::string val = textBox->getText();
				ofmsg("String value set to %1%", %val);
				myParm.setStringValue(0, val.c_str());
			}
		}
		
		he->cook();
		he->wait_for_cook();
	}
}
