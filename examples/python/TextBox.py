ui = UiModule.createAndInitialize().getUi()

font = 'fonts/segoeuimod.ttf 42'

c = Container.create(ContainerLayout.LayoutVertical, ui)
c.setSize(Vector2(900, 600))
c.setPosition(Vector2(15, 96))
c.setFillEnabled(True)
c.setFillColor(Color('#202020'))

l1 = Label.create(c)
l1.setFont(font)
l1.setText("First Name:")
t1 = TextBox.create(c)
t1.setFont(font)
l2 = Label.create(c)
l2.setFont(font)
l2.setText("Color:")
t2 = TextBox.create(c)
t2.setFont(font)
t2.setUIEventCommand("c2.setFillColor(Color('#%value%'))")

c2 = Container.create(ContainerLayout.LayoutFree, c)
c2.setSize(Vector2(300,300))
c2.setAutosize(False)
c2.setFillEnabled(True)
c2.setFillColor(Color('black'))

c.requestLayoutRefresh()
