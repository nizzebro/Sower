

#include "sower_editor.h"
#include "reaper_plugin_functions.h"

using namespace sower;

void Editor::GridHeader::paint(Graphics& g) {
  Editor& editor = *Editor::instance;
  auto rc = g.getClipBounds();
  int y = rc.getY(); 
  int iRow = y / kRowHeight + project.getTopmostRowVisible();
  y -= y % kRowHeight;


  for (; y < rc.getBottom(); y += kRowHeight, ++iRow) {
    auto track = isSingleMode() ? project.chnSingle.track : project.chnsMulti[iRow].track;
    int midiChn = isSingleMode() ? project.chnSingle.midiChn : project.chnsMulti[iRow].midiChn;
    int pitch = isSingleMode() ? (127 - iRow) : project.chnsMulti[iRow].pitch;

    bool isRowActive = iRow == editor.iRowActive;

    g.drawImageAt(
      editor.images[(isRowActive ? ImageIndex::chnLeftA : ImageIndex::chnLeft)],
      0, y);

    g.drawImageWithin(
      editor.images[ImageIndex::chn + (isRowActive? 1 : 0)],
      kChannelLeftIndent, y, getWidth() - kChannelLeftIndent - kChannelRightIndent, kRowHeight,
      RectanglePlacement::stretchToFit);
    g.setFont(editor.fontBig);
    g.setColour(editor.textColor);
    g.drawText(getTrackAndNoteName(track, pitch, midiChn), kChannelLeftIndent, y,
      getWidth() - kChannelLeftIndent - kChannelRightIndent, kRowHeight, Justification::centredLeft, true);
    bool isRowSelected = editor.getSelectedRows().contains(iRow);
    g.drawImageAt( editor.images[ImageIndex::chnSelOff + (isRowActive ? 1 : 0) + (isRowSelected? 2 :0)],
      getWidth() - kChannelRightIndent, y);
  }
}

void sower::Editor::GridHeader::mouseDown(const MouseEvent & e) {
  int x = e.getMouseDownX();
  int y = e.getMouseDownY();
  Editor& editor = *Editor::instance;
  int iRow = y / kRowHeight + project.getTopmostRowVisible();
  
  if (x < getWidth() - kChannelRightIndent) {
    // label
    if (config.flags & config.kPreviewHeader) Editor::instance->playPreview(iRow);
    Editor::instance->setRowActive(iRow);
    return;
  }
  if (!e.mods.isLeftButtonDown()) return;
  // selector

  y -= y % kRowHeight; // for repaint

  auto& selectedRows = editor.getSelectedRows();
  int idxSelected = selectedRows.indexOf(iRow);

  if (e.mods.isShiftDown()) { 
    // if shift is pressed, add to / remove this row from group
    if (idxSelected != -1) selectedRows.remove(idxSelected);
    else selectedRows.add(iRow); 
    repaint(getWidth() - kChannelRightIndent, y, kChannelRightIndent, kRowHeight);
  }
  else {
    // selected this one or select all
    int nRows = editor.getNumRowsMax();
    if ((idxSelected != -1) && (selectedRows.size() != nRows)) {
      selectedRows.resize(nRows);
      int* p = selectedRows.begin();
      for (int i = 0; i != nRows; ++i, ++p) { *p = i; }
    }
    else {
      selectedRows.clearQuick();
      selectedRows.add(iRow);
    }
    repaint(getWidth() - kChannelRightIndent, 0, kChannelRightIndent, getHeight());
  }
  
}

void sower::Editor::GridHeader::mouseDrag(const MouseEvent & e) {

}

void sower::Editor::GridHeader::mouseUp(const MouseEvent & e) {
  if (e.mods.isPopupMenu() && (!isSingleMode())) Editor::instance->openChannelMultiMenu();
}

void Editor::Grid::paint(Graphics& g) {
  Editor& editor = *Editor::instance;
  // when height <= scrollbar thickness, viewport hides scrollbars:
  if (getParentComponent()->getHeight() <= kScrollbarThickness) return; 

  auto rc = g.getClipBounds();
  int x = rc.getX(); int right = rc.getRight();
  int y = rc.getY(); int bottom = rc.getBottom();
  // get start indices and rounded-to-cell coords
  int iCol = x / kColumnWidth;
  x -= x % kColumnWidth;
  int iRow = y / kRowHeight;
  y -= y % kRowHeight;

  int dQuants = editor.columnToQuant(1);
  int iQuantPlayed = columnToQuant(editor.iColumnAnimated);
  for (Quant* row = editor.getQuants(iRow); y < bottom; y += kRowHeight, row += editor.numQuants) {
    int xCol = x;
    int iQuant = editor.columnToQuant(iCol);
    for (Quant* quant = row + iQuant; xCol < right;  xCol += kColumnWidth, quant += dQuants, iQuant += dQuants) {
      int isUpBeat;
      if(!useTriplets()) isUpBeat = getMinResolution(iQuant) ? 1 : 0;
      else isUpBeat = (iQuant % (columnToQuant(1,0) + columnToQuant(1,0) / 2)) != 0;
      int iImg = ImageIndex::cellOff1 + isUpBeat + (quant->isSet() ? 2 : 0);
      g.drawImageAt(editor.images[iImg], xCol, y);
      if (iQuantPlayed == iQuant) {
        g.drawImageAt(editor.images[ImageIndex::cellPlayMask], xCol, y);
      }
    }
  }
}

void sower::Editor::Grid::mouseDown(const MouseEvent & e) {
  if (!e.mods.isLeftButtonDown()) return;
  int y = e.getMouseDownY();
  int x = e.getMouseDownX(); 
  int iColumn = x / kColumnWidth;
  jassert(columnToQuant(iColumn) < Editor::instance->numQuants);
  int iRow = y / kRowHeight;
  iLastColumn = iColumn;
  if (config.flags & config.kPreviewCell) {
    Editor::instance->playPreview(iRow);
  }
  Editor::instance->setRowActive(iRow);
  
  bSet = !Editor::instance->getQuants(iRow, columnToQuant(iLastColumn))->isSet();
  updateActiveCell();
}

void sower::Editor::Grid::updateActiveCell() {
  int iQuant = columnToQuant(iLastColumn);
  if (config.flags & config.kPreviewCell) {
    Editor::instance->playPreview(Editor::instance->iRowActive);
  }
  if (bSet) Editor::instance->insertNote(Editor::instance->iRowActive, iQuant);
  else  Editor::instance->removeNote(Editor::instance->iRowActive, iQuant);
  repaint(iLastColumn * kColumnWidth,  Editor::instance->iRowActive * kRowHeight, kColumnWidth, kRowHeight);
  Editor::instance->controlPanel.repaint(
    (iLastColumn - project.iLeftColumnVisible) * kColumnWidth, 
    kParamIndent, kColumnWidth,
    Editor::instance->controlPanel.getHeight() - kParamIndent);
}

void sower::Editor::Grid::mouseDrag(const MouseEvent & e) {
  if (!e.mods.isLeftButtonDown()) return;
  int x = e.getPosition().getX();
  if ((x < 0) || (x >= getWidth())) return;
  int y = e.getPosition().getY();
  if ((y < 0) || (y >= getHeight())) return;
  int iColumn = x / kColumnWidth;
  int iRow = y / kRowHeight;
  if ((iLastColumn == iColumn) && (Editor::instance->iRowActive == iRow)) return;
  Editor::instance->setRowActive(iRow);
  iLastColumn = iColumn;
  updateActiveCell();
}

void sower::Editor::Grid::mouseUp(const MouseEvent & e) {
  if (e.mods.isLeftButtonDown()) 
    Editor::instance->setRowActive(Editor::instance->iRowActive);
  else if (e.mods.isPopupMenu()) {
    int y = e.getMouseDownY();
    int x = e.getMouseDownX();
    int iQuant = columnToQuant(x / kColumnWidth);
    jassert(iQuant < Editor::instance->numQuants);
    int iRow = y / kRowHeight;
    if (Editor::instance->getQuants(iRow, iQuant)->isSet()) {
      Editor::instance->openNoteDialog
      (e.getMouseDownScreenX(), e.getMouseDownScreenY(), iRow, iQuant);
    }
  }
}



void Editor::GridHolder::visibleAreaChanged(const juce::Rectangle<int>& rc) noexcept {
  // check row/column boundaries and correct if needed
  int iCol = rc.getX() / kColumnWidth;
  int dx = rc.getX() % kColumnWidth;
  int iRow = rc.getY() / kRowHeight;
  int dy = rc.getY() % kRowHeight;
  if (dx || dy) {
    setViewPosition(rc.getX() - dx, rc.getY() - dy); // will call visibleAreaChanged again...
    return;
  }
  int nCols = rc.getWidth() / kColumnWidth;
  int dw = rc.getWidth() % kColumnWidth;
  nCols += dw ? 1 : 0;
  int nRows = rc.getHeight() / kRowHeight;
  int dh = rc.getHeight() % kRowHeight;
  nRows += dh ? 1 : 0;
  Editor::instance->scrolled(iCol, nCols, iRow, nRows);
}

