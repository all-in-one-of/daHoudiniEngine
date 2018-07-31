import os

from cyclops import *
from daHEngine import *
from omegaToolkit import *

ui = UiModule.createAndInitialize().getUi()

things = [
    "Nothing",
    "some thing",
    "Some other Thing"
    "Thing 3",
    "Last thing"
]

cont = Container.create(ContainerLayout.LayoutHorizontal, ui)
choiceCont = Container.create(ContainerLayout.LayoutVertical, ui)
choiceCont.setVisible(False)
for thing in things:
    tbut = Button.create(choiceCont)
    tbut.setText(thing)
    tbut.setRadio(True)
    tbut.setCheckable(True)

lab = Label.create(cont)
lab.setText("My Parm thing")
but = Button.create(cont)
but.setText(things[0])
but.setUIEventCommand("choiceCont.setVisible({})".format("%value%"))
but.setCheckable(True)
choicePos = Vector2(but.getPosition()[0] + but.getSize()[0], but.getPosition()[1])
choiceCont.setPosition(choicePos)
cont.setPosition(Vector2(100, 100))
