#include "sower_editor.h"

using namespace sower;
Editor::ToolButton::ToolButton(CommandCallback cmd, Image* img) noexcept :
  command(cmd),
  image(img),
  isOver(false),
  isDown(false) {
}


void Editor::ToolButton::updateState(bool over, bool down) {
  isOver = over; isDown = down; repaint();
}


void Editor::ToolButton::mouseEnter(const MouseEvent&) { updateState(true, false); }
void  Editor::ToolButton::mouseExit(const MouseEvent&) { updateState(false, false); }
void  Editor::ToolButton::mouseDown(const MouseEvent& e) { 
  if (isEnabled()) updateState(true, e.mods.isLeftButtonDown());
}

void  Editor::ToolButton::mouseUp(const MouseEvent& e) {
  if ((!e.mods.isLeftButtonDown()) || (!isDown)) return;
  auto pos = e.getPosition();
  if(!getLocalBounds().contains(pos))return;
  (Editor::instance->*command)();
  updateState(false, false);
}




void Editor::ToolButton::paint(Graphics& g) {
  g.drawImageAt(*image, 0, 0);
  if (isDown || isOver) {
    if (isDown) g.drawImageAt(Editor::instance->images[ImageIndex::btnDownMask], 0, 0);
    else if (isOver) g.drawImageAt(Editor::instance->images[ImageIndex::btnHoverMask], 0, 0);
  }
}

Editor::ScrollText::ScrollText(Callback cmdDrag, Callback cmdEndDrag) :
  dragCommand(cmdDrag),
  endDragCommand(cmdEndDrag),
  lastDelta(0){
  setMouseCursor(MouseCursor::UpDownResizeCursor);
}


void Editor::ScrollText::paint(Graphics& g) {
  g.setFont(Editor::instance->fontBig);
  g.setColour(Editor::instance->textColor);
  g.drawText(text, getLocalBounds(), Justification::centred, false);
}



void Editor::ScrollText::mouseDrag(const MouseEvent& e) {
  if (!e.mods.isLeftButtonDown()) return;
  int delta = -e.getDistanceFromDragStartY() / 8;
  if (lastDelta == delta) return;
  lastDelta = delta;
  (Editor::instance->*dragCommand)(delta);
}

void  Editor::ScrollText::mouseUp(const MouseEvent& e) {
  if (!e.mods.isLeftButtonDown()) return;
  int delta = -e.getDistanceFromDragStartY() / 8;
  lastDelta = 0;
  (Editor::instance->*endDragCommand)(delta);
}

void Editor::ScrollText::mouseWheelMove(const MouseEvent & event, const MouseWheelDetails & wheel) {
  if (wheel.isInertial) return;
  int dy = 1; if ((wheel.deltaY < 0) || (wheel.isReversed)) dy = -dy;
  (Editor::instance->*endDragCommand)(dy);
}


Editor::Header::Header(Editor& editor) noexcept:
btnMenu(&Editor::openHeaderMenu, &editor.images[ImageIndex::tbrMenu]) {
  addAndMakeVisible(btnMenu);
}


void Editor::Header::resized() {
  btnMenu.setBounds(getWidth() - Editor::kChannelRightIndent, 0, Editor::kChannelRightIndent, getHeight());
}


void Editor::Header::paint(Graphics& g) {
  if (g.getClipBounds().getX() >= btnMenu.getX()) return;
  g.drawImageWithin(Editor::instance->images[ImageIndex::tbr], 0, 0, btnMenu.getX(), getHeight(),
    RectanglePlacement::stretchToFit);
  g.setFont(Editor::instance->fontBig);
  g.setColour(Editor::instance->textColor);
  g.drawText(text, Editor::kChannelLeftIndent, 
    0, btnMenu.getX() - Editor::kChannelLeftIndent - 4, getHeight(), Justification::centredLeft, true);
}

void Editor::Header::mouseUp(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) Editor::instance->openMainMenu(this);
}

Editor::Toolbar::Toolbar(Editor& editor) noexcept:
  btnBarPrev(&Editor::setBarToPrevious, &editor.images[ImageIndex::arrLeft]),
  txtBar(&Editor::barScrolled, &Editor::advanceBar),
  btnBarNext(&Editor::setBarToNext, &editor.images[ImageIndex::arrRight]),
  btnSyncBar(&Editor::toggleSyncBar, (project.flags & project.kSyncBar) ? 
    &editor.images[ImageIndex::syncOn] : &editor.images[ImageIndex::syncOff]),
  btnLoop (&Editor::toggleLoopState, &editor.images[ImageIndex::loopOff]),
  txtDenominator(&Editor::resolutionScrolled, &Editor::advanceResolution),
  btnItemStretchMode(&Editor::toggleItemStretchMode, (project.flags & project.kStretchItem)?
    &editor.images[ImageIndex::modeStretch]: &editor.images[ImageIndex::modeInsert]),
  btnControlPanel(&Editor::toggleControlPanelVisible, (project.flags & project.kCPanelVisible) != 0?
    &editor.images[ImageIndex::cpanelOn] :&editor.images[ImageIndex::cpanelOff]),

  btnMidiProcessor(&Editor::openMidiProcessor, &editor.images[ImageIndex::midiProcessor]),
  btnOptions(&Editor::openOptions, &editor.images[ImageIndex::options]) {
  int x = 0;
  addChild(btnBarPrev, x, 24);
  addChild(txtBar, x, 32);
  addChild(btnBarNext, x, 24);
  addChild(btnSyncBar, x, 24);
  addChild(btnLoop, x, 24);
  addChild(txtDenominator, x, 32);
  addChild(btnItemStretchMode, x, 24);
  addChild(btnControlPanel, x, 24);
  addChild(btnMidiProcessor, x, 24);
  addChild(btnOptions, x, 24);

  btnBarPrev.setTooltip("Go to the prev. bar");
  txtBar.setTooltip("Current bar");
  btnBarNext.setTooltip("Go to the next bar");
  btnSyncBar.setTooltip(String("Sync bar: ") +  (useSyncBar()? "on" : "off"));
  btnLoop.setTooltip(String("Loop: ") +  "off");
  txtDenominator.setTooltip("Grid resolution");
  btnItemStretchMode.setTooltip(String("If there's no MIDI item, ")
    + (useStretchItem() ? "stretch the prev. one" : "insert a new one"));
  btnControlPanel.setTooltip(String("Control panel: ") +  (isControlPanelVisible() ? "on" : "off"));
  btnMidiProcessor.setTooltip("Open MIDI Processor");
  btnOptions.setTooltip("Open settings");
}



  int sower::Editor::Toolbar::getMinimumWidth() const noexcept {
    int wt = 0; auto pComp = getChildren().getLast();
    if (pComp) wt = pComp->getRight();
    return wt;
  }

  void sower::Editor::Toolbar::addChild(Component& c, int& pos, int wt) {
    c.setBounds(pos, 0, wt, Editor::kToolbarHeight); pos += wt;
    addAndMakeVisible(c);
  }



 void sower::Editor::Toolbar::mouseUp(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    if (btnMidiProcessor.getBounds().contains(e.getPosition())) Editor::instance->openMidiProcessor();
  }
 }

void Editor::Toolbar::paint(Graphics& g) {
    g.drawImageWithin(Editor::instance->images[ImageIndex::tbr], 0, 0, getWidth(), getHeight(),
      RectanglePlacement::stretchToFit);

}



