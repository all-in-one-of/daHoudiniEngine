from daHEngine import *
from cyclops import *
from omegaToolkit import *

# this will make a container seperate from the menu to display houdini parameter controls

# Create a sub-menu that will contain the multiple columns
mm = MenuManager.createAndInitialize()
menu2 = mm.getMainMenu().addSubMenu("AnAsset")

# Get the menu container, set its layout to horizontal. Set all elements
# aligned at the top of the container
#mc = menu2.getContainer()
#mc.setLayout(ContainerLayout.LayoutHorizontal)
#mc.setLayout(ContainerLayout.LayoutFree)
#mc.setLayout(ContainerLayout.LayoutVertical)
#mc.setLayout(ContainerLayout.LayoutGridHorizontal)
#mc.setVerticalAlign(VAlign.AlignBottom)

uim = UiModule.createAndInitialize()

# new container
he = Container.create(ContainerLayout.LayoutFree, uim.getUi())
he.setStyle('fill:#404040') # 25% grey
he.setStyle('border:1 #808080') # 50% grey
he.setPadding(0)
he.setMargin(0)

he.setPosition(Vector2(100, 100))
mc = he

# Create 3 vertical columns, add them to the menu
#c1 = Container.create(ContainerLayout.LayoutVertical, mc)
c1 = Container.create(ContainerLayout.LayoutHorizontal, mc)
c1.setHorizontalAlign(HAlign.AlignLeft)
c1.setPosition(Vector2(5.0, 5.0))
c1.setPadding(0)
c1.setMargin(0)

#c2 = Container.create(ContainerLayout.LayoutVertical, mc)
c2 = Container.create(ContainerLayout.LayoutVertical, mc)
c2.setVerticalAlign(VAlign.AlignTop)
c2a = Container.create(ContainerLayout.LayoutHorizontal, c2)
c2a.setHorizontalAlign(HAlign.AlignLeft)
c2b = Container.create(ContainerLayout.LayoutHorizontal, c2)
c2b.setHorizontalAlign(HAlign.AlignLeft)

myPos = c2.getPosition()
c2.setPosition(Vector2(5.0, 25.0))
c2.setPadding(0)
c2.setMargin(0)
c2.setStyle('border:1 #808080') # 50% grey

c3 = Container.create(ContainerLayout.LayoutVertical, mc)
c3.setVerticalAlign(VAlign.AlignTop)
c3a = Container.create(ContainerLayout.LayoutHorizontal, c3)
c3a.setHorizontalAlign(HAlign.AlignLeft)
c3b = Container.create(ContainerLayout.LayoutHorizontal, c3)
c3b.setHorizontalAlign(HAlign.AlignLeft)

c3.setPosition(Vector2(5.0, 30.0)	)
# Add a bunch of buttons to the columns
# Column 1
b1c1 = Button.create(c1)
b2c1 = Button.create(c1)
b1c1.setStyle('border:1 #808080') # 50% grey
b1c1.setStyle('border-bottom:1 #404040') # 25% grey, matching background
print b1c1.getPosition()
print b2c1.getPosition()
#b1c1.setAutosize(False)
#b2c1.setAutosize(False)
b1c1.setHeight(25)
b2c1.setHeight(25)
print b1c1.getPosition()
print b2c1.getPosition()

# Column 2
L1c2a = Label.create(c2a)
L1c2a.setAutosize(False)
L1c2a.setWidth(100)
L1c2a.setText("Some Value")

t1c2a = TextBox.create(c2a)
t1c2a.setText("0.12")
t1c2a.setFont('fonts/segoeuimod.ttf 14')
t1c2a.setAutosize(False)
t1c2a.setWidth(65)
t1c2a.setUIEventCommand('blahb()')

sliderWidth = 320

s1c2a = Slider.create(c2a)
s1c2a.setUIEventCommand('blah()')
s1c2a.setAutosize(False)
s1c2a.setWidth(sliderWidth)

L1c2b = Label.create(c2b)
L1c2b.setText("Other Value")
L1c2b.setAutosize(False)
L1c2b.setWidth(100)

t1c2b = TextBox.create(c2b)
t1c2b.setText("0.12")
t1c2b.setFont('fonts/segoeuimod.ttf 14')
t1c2b.setAutosize(False)
t1c2b.setWidth(65)

s1c2b = Slider.create(c2b)
s1c2b.setUIEventCommand('blah2()')
s1c2b.setAutosize(False)
s1c2b.setWidth(sliderWidth)

# Column 3
L1c3a = Label.create(c3a)
L1c3a.setText("Value 2 ")
L1c3a.setAutosize(False)
L1c3a.setWidth(100)

t1c3a = TextBox.create(c3a)
t1c3a.setText("0.12")
t1c3a.setFont('fonts/segoeuimod.ttf 14')
t1c3a.setAutosize(False)
t1c3a.setWidth(65)

s1c3a = Slider.create(c3a)
s1c3a.setUIEventCommand('blah3()')
s1c3a.setAutosize(False)
s1c3a.setWidth(sliderWidth)

L1c3b = Label.create(c3b)
L1c3b.setText("Value 3rd")
L1c3b.setAutosize(False)
L1c3b.setWidth(100)

t1c3b = TextBox.create(c3b)
t1c3b.setText("0.12")
t1c3b.setFont('fonts/segoeuimod.ttf 14')
t1c3b.setAutosize(False)
t1c3b.setWidth(65)

s1c3b = Slider.create(c3b)
s1c3b.setUIEventCommand('blah4()')
s1c3b.setAutosize(False)
s1c3b.setWidth(sliderWidth)

def blah():
	t1c2a.setText(str(s1c2a.getValue() * 10))

def blahb():
	s1c2a.setValue(int(int(t1c2a.getText()) / 10.0))

def blah2():
	t1c2b.setText(str(s1c2b.getValue() * 10))

def blah3():
	t1c3a.setText(str(s1c3a.getValue() * 10))

def blah4():
	t1c3b.setText(str(s1c3b.getValue() * 10))


b1c1.setUIEventCommand('erf()')

def erf():
	c3.setVisible(False)
	c2.setVisible(True)

b2c1.setUIEventCommand('erf2()')

def erf2():
	c2.setVisible(False)
	c3.setVisible(True)


# Column 3
#b1c3 = Button.create(c3)
#b2c3 = Button.create(c3)
#b3c3 = Button.create(c3)

# setup navigation
# vertical navigation for buttons will be set up automatically by the container.
# we set horizontal navigation manually

# Buttons on the first row
#b1c1.setHorizontalNextWidget(b1c2)
#b1c2.setHorizontalPrevWidget(b1c1)

#b1c2.setHorizontalNextWidget(b1c3)
#b1c3.setHorizontalPrevWidget(b1c2)

## Buttons on the second row
#b2c1.setHorizontalNextWidget(b2c2)
#b2c2.setHorizontalPrevWidget(b2c1)

#b2c2.setHorizontalNextWidget(b2c3)
#b2c3.setHorizontalPrevWidget(b2c2)

# Buttons on the third row
# since the first column does not have a third row, prev navigation
# from column 2 goes to the last button of column 1
#b3c2.setHorizontalPrevWidget(b2c1)

#b3c2.setHorizontalNextWidget(b3c3)
#b3c3.setHorizontalPrevWidget(b3c2)

#lab = Label.create(c1)
#lab.setColor(Color('blue'))


########### prev code ##########

## Get the menu container, set its layout to horizontal. Set all elements
## aligned at the top of the container
#mc = menu2.getContainer()
#mc.setLayout(ContainerLayout.LayoutHorizontal)
#mc.setVerticalAlign(VAlign.AlignTop)

## Create 3 vertical columns, add them to the menu
#c1 = Container.create(ContainerLayout.LayoutVertical, mc)
#c2 = Container.create(ContainerLayout.LayoutVertical, mc)
#c3 = Container.create(ContainerLayout.LayoutVertical, mc)

## Add a bunch of buttons to the columns
## Column 1
#b1c1 = Button.create(c1)
#b2c1 = Button.create(c1)

## Column 2
#b1c2 = Button.create(c2)
#b2c2 = Button.create(c2)
#b3c2 = Button.create(c2)

## Column 3
#b1c3 = Button.create(c3)
#b2c3 = Button.create(c3)
#b3c3 = Button.create(c3)

## setup navigation
## vertical navigation for buttons will be set up automatically by the container.
## we set horizontal navigation manually

## Buttons on the first row
#b1c1.setHorizontalNextWidget(b1c2)
#b1c2.setHorizontalPrevWidget(b1c1)

#b1c2.setHorizontalNextWidget(b1c3)
#b1c3.setHorizontalPrevWidget(b1c2)

## Buttons on the second row
#b2c1.setHorizontalNextWidget(b2c2)
#b2c2.setHorizontalPrevWidget(b2c1)

#b2c2.setHorizontalNextWidget(b2c3)
#b2c3.setHorizontalPrevWidget(b2c2)

## Buttons on the third row
## since the first column does not have a third row, prev navigation
## from column 2 goes to the last button of column 1
#b3c2.setHorizontalPrevWidget(b2c1)

#b3c2.setHorizontalNextWidget(b3c3)
#b3c3.setHorizontalPrevWidget(b3c2)

#lab = Label.create(c1)
#lab.setColor(Color('blue'))

mc.requestLayoutRefresh()
print b1c1.getPosition()
print b2c1.getPosition()
