

#include "reaper_plugin_functions.h"
#include "sower_editor.h"
using namespace sower;


sower::Editor::Splitter::Splitter(bool vertical, Callback callback) noexcept:
  isVertical(vertical), dragCommand(callback) {
    setRepaintsOnMouseActivity (true);
  setMouseCursor(vertical ? MouseCursor::LeftRightResizeCursor : MouseCursor::UpDownResizeCursor);
}


void sower::Editor::Splitter::paint(Graphics& g) {
  g.drawImageWithin(Editor::instance->images[isVertical ? ImageIndex::splitterV :
    ImageIndex::splitterH], 0, 0, getWidth(), getHeight(),
    RectanglePlacement::stretchToFit);
}

void sower::Editor::Splitter::mouseDown(const MouseEvent & e) {
  if (!e.mods.isLeftButtonDown()) return;
  startPos = isVertical ? getX() : getY();
}


void sower::Editor::Splitter::mouseDrag(const MouseEvent & e) {
  if (!e.mods.isLeftButtonDown()) return;
  int pos = startPos + (isVertical ? e.getDistanceFromDragStartX() : e.getDistanceFromDragStartY());
  (Editor::instance->*dragCommand)(pos);
}




