#include "sower_editor.h"
#include "reaper_plugin_functions.h"
#include "inifileparser.h"
using namespace sower;

inline void MarkProjDirty() {
#if defined SOWER_VERSION_FULL
  MarkProjectDirty(0);
#endif
}



//  ======== static vars ==========

Editor* Editor::instance = nullptr;

int Editor::ppqNoteLengthUsed = 0;  // auto
int Editor::noteVelocityUsed = 100;

Array<Array<Editor::RowCopyNote> > Editor::copiedRows;

Editor::Label::Label() noexcept {

}

Editor::Label::Label(const String& s) {
  setText(s);
}

void Editor::Label::setText(const String& s) {
  text = s;
  setSize(Editor::instance->fontSmall.getStringWidth(text) + 2, Skin::kSmallTextHeight + 6);
  repaint();
}

void Editor::Label::paint(Graphics& g)  {
  g.setFont(Editor::instance->fontSmall);
  Colour color = Editor::instance->textColor;
  if (!isEnabled())color = color.withAlpha(0.4f);
  g.setColour(color);
  g.drawText(text, 1, 0, getWidth() - 1, getHeight(), Justification::centredLeft);
}

 

Editor::Editor() :
  header(*this),
  toolbar(*this),
  splitterVert(true, &Editor::verticalSplitterDrag),
  splitterHorz(false, &Editor::horizontalSplitterDrag) {

  instance = this; // subcomponents use this everywhere except ctors

  setOpaque(true);
  setBroughtToFrontOnMouseClick(true);
  setWantsKeyboardFocus(true);
  setName("Sower");
  setAlwaysOnTop((project.flags & project.kStayOnTop) != 0);
  setDefaultLookAndFeel(this);


  gridHolder.setScrollBarThickness(kScrollbarThickness);
  gridHolder.getVerticalScrollBar().setAutoHide(false);
  gridHolder.getHorizontalScrollBar().setAutoHide(false);
  gridHolder.setSingleStepSizes(kColumnWidth, kRowHeight);
  gridHolder.setWantsKeyboardFocus(false);
  addAndMakeVisible(header);
  header.setText(isSingleMode() ? getTrackName(project.chnSingle.track) : String("multi"));
  addAndMakeVisible(gridHeader);
  addChildComponent(controlPanelHeader);
  addAndMakeVisible(toolbar);
  addAndMakeVisible(gridHolder);
  addChildComponent(controlPanel);
  addAndMakeVisible(splitterVert);
  addChildComponent(splitterHorz);

  bool isVisible = isControlPanelVisible();
  controlPanelHeader.setVisible(isVisible);
  splitterHorz.setVisible(isVisible);
  controlPanel.setVisible(isVisible);

  gridHolder.setViewedComponent(&grid, false);

  update();

  toolbar.txtDenominator.setText(String(getDenominator()));
  setMinimumOnscreenAmounts(0x10000, 16, 24, 16);
  createWindow();
  setVisible(true);
  toFront(true);

    
  setTooltips();  
    
#if JUCE_WINDOWS
    InitializeCriticalSection(&preview.cs);
#else
    pthread_mutex_init(&preview.mutex, NULL);
#endif
    auto item = CreateNewMIDIItemInProj(0, 0.0, 4.0, 0);
    preview.src = GetMediaItemTake_Source(GetActiveTake(item))->Duplicate();
    DeleteTrackMediaItem(0, item);
    preview.m_out_chan = -1;
    preview.loop = 0;
    preview.volume = 0.75;
    preview.peakvol[0] = 0.75;
    preview.peakvol[1] = 0.75;
    
    midiwrite.global_time = 0.0;
    midiwrite.global_item_time = 0.0;
    midiwrite.srate = 44100;
    midiwrite.length = 44100 * 2;
    midiwrite.overwritemode = 1;		// replace flag
    midiwrite.events = MIDI_eventlist_Create();
    midiwrite.item_playrate = 1.0;
    midiwrite.latency = 0.0;
    midiwrite.overwrite_actives = 0;

    

    if ((config.flags & config.kAnimatePlayback) || (project.flags & project.kSyncBar)) {
      startTimer(70);
    }

    project.flags |= project.kOpened;
}


Editor::~Editor() {
  delete modalComp;
  delete midiProcessor;

  StopTrackPreview2(0, &preview);
  delete preview.src;
  
  #if JUCE_WINDOWS
    DeleteCriticalSection(&preview.cs);
  #else
    pthread_mutex_destroy(&preview.mutex);
  #endif

  MIDI_eventlist_Destroy(midiwrite.events);
  destroyWindow();
  instance = nullptr;
}

#if JUCE_WINDOWS

WNDPROC oldWndProc;

LRESULT dockWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if ((message == WM_COMMAND) && (wParam == IDCANCEL)) {
    if (sower::Editor::getInstance() != nullptr) sower::Editor::getInstance()->userTriedToCloseWindow();
    return 0;
  }
  return CallWindowProc(oldWndProc, hwnd, message, wParam, lParam);
}



void Editor::createWindow() {
  if (isDocked()) {
    addToDesktop(0, reaper);
    DockWindowAddEx((HWND)getWindowHandle(), "Sower", "Sower", true);
    DockWindowActivate((HWND)getWindowHandle());
    getPeer()->setConstrainer(nullptr);
    oldWndProc = (WNDPROC)SetWindowLongPtr((HWND)getWindowHandle(), GWLP_WNDPROC, (LONG_PTR)&dockWndProc);
  }

  else {
    addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowHasTitleBar
                 | ComponentPeer::windowHasCloseButton | ComponentPeer::windowIsResizable, reaper);
    if (project.windowX == -1) centreWithSize(getWidth(), getHeight());
    else setTopLeftPosition(project.windowX, project.windowY);
    getPeer()->setConstrainer(this);
  }
}
#endif

void Editor::destroyWindow() {
  if (isDocked()) {
 
    DockWindowRemove((HWND)getWindowHandle());
  }
  removeFromDesktop();
}

void Editor::openUserGuide() {
  getPluginDataDir().getChildFile("Sower_manual.pdf").startAsProcess();
}

void Editor::userTriedToCloseWindow() noexcept {
  delete this;
  project.flags &= ~project.kOpened;
  MarkProjDirty();
}


bool sower::Editor::isPlaying() noexcept {
  { return (GetPlayStateEx(0) & 1) != 0; }
}


void Editor::grabFocus() {
  if (isDocked()) DockWindowActivate((HWND)getWindowHandle());
  else grabKeyboardFocus();
}


void Editor::focusGained(FocusChangeType cause)  {
  update();
}



bool sower::Editor::keyPressed(const KeyPress & key) noexcept {
  //int c = key.getKeyCode();

  //bool isCmdDown = key.getModifiers().isCommandDown();
  //if (c == KeyPress::leftKey) {
  //  if(!isCmdDown)setBarToPrevious();
   // else gridHolder.getHorizontalScrollBar().moveScrollbarInPages(-1);
  //}
  //else if (c == KeyPress::rightKey) {
  //  if (!isCmdDown)setBarToNext();
  //  else gridHolder.getHorizontalScrollBar().moveScrollbarInPages(1);
  //}
  //else if (c == KeyPress::upKey) {
   // if (!isCmdDown) {
   //   setRowActive(iRowActive - 1);
    //  setRowVisible(iRowActive - 1);
   // }
   // else gridHolder.getVerticalScrollBar().moveScrollbarInPages(-1);
  //}
  //else if (c == KeyPress::downKey) {
   // if (!isCmdDown) {
      //setRowActive(iRowActive + 1);
     // setRowVisible(iRowActive + 1);
   // }
   // else gridHolder.getVerticalScrollBar().moveScrollbarInPages(1);
  //}
 // else if ((c == 'C') && (isCmdDown)) copyRows();
 // else if ((c == 'X') && (isCmdDown))  copyRows(true);
 // else if ((c == 'V') && (isCmdDown))  pasteRows();*/
  return true;
}


void Editor::update() {
  // reload any data that may have been changed from outside:

  // validate tracks may have been deleted
  if (project.chnSingle.track) {
    if (getTrackIndex(project.chnSingle.track) == -1) {
      project.chnSingle.track = nullptr;
      selectedRowsSingle.clearQuick();
      MarkProjDirty();
    }
  }
  for (int i = 0; i < project.chnsMulti.size(); ) {
    if (getTrackIndex(project.chnsMulti[i].track) == -1) {
      project.chnsMulti.remove(i);
      selectedRowsMulti.remove(i);
      MarkProjDirty();
      continue;
    }
    ++i;
  }
  // set ppqQN
  int szOut;  // not used; but get_config_var needs it
  ppqQN = *(int*)get_config_var("miditicksperbeat", &szOut);
  
  updateBar();
  
  // sync loop state to Reaper
  double tmeLoopStart, tmeLoopEnd;  
  GetSet_LoopTimeRange2(0, false, true, &tmeLoopStart, &tmeLoopEnd, false);
  bool hasLoop = ((tmeBarStart == tmeLoopStart) && (tmeBarEnd == tmeLoopEnd) && GetSetRepeatEx(0, -1));
  if (hasLoop != loopState) {
    loopState = hasLoop;
    toolbar.btnLoop.setImage(&images[(loopState ? ImageIndex::loopOn : ImageIndex::loopOff)]);
    toolbar.btnLoop.setTooltip(String("Loop: ") + (loopState ? "on" : "off"));
  }


}



void sower::Editor::scrolled(int iCol, int nCols, int iRow, int nRows) {
  bool wasScrolledX = (iCol != project.iLeftColumnVisible) || (nCols != project.nColumnsVisible);
  bool wasScrolledY = (iRow != project.getTopmostRowVisible()) || (nRows != project.getNumsRowsVisible());
  if (wasScrolledX || wasScrolledY) {
    if (wasScrolledX) {
      project.iLeftColumnVisible = iCol;
      project.nColumnsVisible = nCols;
      controlPanel.repaint();
    }
    if (wasScrolledY) {
      project.setTopmostRowVisible(iRow);
      project.setNumsRowVisible(nRows);
      gridHeader.repaint();
    }
    MarkProjDirty();
  }
}



void Editor::moved() {
  if (!isDocked()) {
    if ((project.windowX != getX()) && (project.windowY != getY())) {
      project.windowX = getX();
      project.windowY = getY();
      MarkProjDirty();
    }
  }
}

void sower::Editor::verticalSplitterDrag(int pos) {
  pos = jlimit((int)kMinimumHeaderWidth, isDocked()? getWidth() - 40 :600, pos);
  if (splitterVert.getX() == pos) return;
  project.channelListWidth = pos;
  resizeWindow(project.nColumnsVisible, project.getNumsRowsVisible());
  MarkProjDirty();
}

void sower::Editor::horizontalSplitterDrag(int pos) {
  pos -= (pos - splitterHorz.getY()) % kRowHeight; // floor to row boundary   
  pos = jlimit(kToolbarHeight + kScrollbarThickness,
    kToolbarHeight + kScrollbarThickness + grid.getHeight(), pos);
  if (pos == splitterHorz.getY()) return;
  resizeWindow(project.nColumnsVisible, (pos - kToolbarHeight - kScrollbarThickness) / kRowHeight);
}


void Editor::resizeWindow(int nCols, int nRows) {

  // calc grid dimensions, width shouldn't be less than toolbar
  int gridAreaWidth = jlimit((int)kMinimumGridAreaWidth, jmax(grid.getWidth(), (int)kMinimumGridAreaWidth), 
    nCols * kColumnWidth);
  int gridAreaHeight = jlimit(0, grid.getHeight(), nRows * kRowHeight);
  
  
  gridHolder.setSize(gridAreaWidth + kScrollbarThickness, gridAreaHeight + kScrollbarThickness);


  
  if (isDocked()) {
    if(getWidth() && getHeight())resized();
    return;
  }

  BorderSize<int> frame;
  if (getPeer()) frame = getPeer()->getFrameSize();
  int wMin = frame.getLeftAndRight() + project.channelListWidth + kSplitterThickness + kScrollbarThickness;
  setMinimumWidth(wMin + kMinimumGridAreaWidth);
  setMaximumWidth(wMin + jmax((int)kMinimumGridAreaWidth, grid.getWidth()));
  if (isControlPanelVisible()) {
    int hMin = frame.getTopAndBottom() + jlimit(0, grid.getHeight(), gridHolder.getMaximumVisibleHeight()) +
      kScrollbarThickness + kMinimumControlPanelHeight;
    setMinimumHeight(hMin);
    setMaximumHeight(hMin + 800 - kMinimumControlPanelHeight);
  }
  else {
    int hMin = frame.getTopAndBottom() + kToolbarHeight + kScrollbarThickness;
    setMinimumHeight(hMin);
    setMaximumHeight(hMin + grid.getHeight());
  }

  int w = project.channelListWidth + kSplitterThickness + gridHolder.getWidth();

  int h = kToolbarHeight + gridHolder.getHeight();
  if (isControlPanelVisible()) h += (kSplitterThickness + project.controlPanelHeight);

  setSize(w, h);
  
}

void Editor::checkIncrements(juce::Rectangle<int>& bounds) {
  BorderSize<int> frame;
  if (!isDocked() && getPeer()) frame = getPeer()->getFrameSize();
  int w = bounds.getWidth();
  if (w > getMinimumWidth()) {
    w -= (frame.getLeftAndRight() + project.channelListWidth + kSplitterThickness + kScrollbarThickness);
    w = jmin(grid.getWidth(), w);
    int d =  w % kColumnWidth; 
    bounds.setWidth(bounds.getWidth() - d);
  }
  if (!isControlPanelVisible()) {
    int h = bounds.getHeight();
    if (h > getMinimumHeight()) {
      h -= (frame.getTopAndBottom() + kToolbarHeight + kScrollbarThickness);
      int d = h % kRowHeight; 
      bounds.setHeight(bounds.getHeight() - d);
    }
  }
}

void Editor::checkBounds(juce::Rectangle<int>& bounds, const juce::Rectangle<int>& oldBounds,
  const juce::Rectangle<int>& limits, bool isStretchingTop, bool isStretchingLeft,
  bool isStretchingBottom, bool isStretchingRight)  noexcept {
  ComponentBoundsConstrainer::checkBounds(bounds, oldBounds, limits, isStretchingTop, isStretchingLeft,
    isStretchingBottom, isStretchingRight);
  checkIncrements(bounds);
}


void Editor::resized() {
  int w = getWidth();
  int h = getHeight();

  const int w1 = project.channelListWidth;
  const int w2 = kSplitterThickness;
  int w3 = w - w1 - w2;
  const int h1 = kToolbarHeight;
  const int h3 = kSplitterThickness;
  int h2, h4;
  if (isControlPanelVisible()) {
    h2 = gridHolder.getHeight();
    h4 = h - h1 - h2 - h3;
  }
  else {
    h2 = h - h1;
    if (isDocked()) h2 = jmin(h2, grid.getHeight() + kScrollbarThickness);
    h4 = controlPanel.getHeight();
  }

  header.setBounds(0, 0, w1, h1);
  toolbar.setBounds(w1 + w2, 0, w3, h1);
  gridHeader.setBounds(0, h1, w1, h2 - kScrollbarThickness);
  gridHolder.setBounds(w1 + w2, h1, w3, h2);
  controlPanelHeader.setBounds(0, h1 + h2 + h3, w1, h4);
  controlPanel.setBounds(w1 + w2, h1 + h2 + h3, w3 - kScrollbarThickness, h4);
  splitterVert.setBounds(w1, 0, w2, h);
  splitterHorz.setBounds(0, h1 + h2, w, h3);
}

void sower::Editor::mouseWheelMove(const MouseEvent & event, const MouseWheelDetails & wheel){
  if (wheel.isInertial) return;
  int dx = kColumnWidth; if ((wheel.deltaX < 0) || (wheel.isReversed)) dx = -dx;
  int dy = kRowHeight; if ((wheel.deltaY < 0) || (wheel.isReversed)) dy = -dy;
  gridHolder.setViewPosition(gridHolder.getViewPositionX() - dx, gridHolder.getViewPositionY() - dy);
}


void sower::Editor::processMenuResult(int i) {
  switch (i) {
    case 0: return;
      // // common main menu items
    case 1: toggleChannelMode(); return;
    case 2: copyRows(); return;
    case 3: cutRows(); return;
    case 4: pasteRows(); return;
    case 5: toggleAlwaysOnTop(); return;
    case 6: toggleDocked(); return;
    case 7: openUserGuide(); return;
    case 50: toggleUseTriplets(); return;
    case 101: 
    case 102: 
    case 104: setNumberOfBars(i - 100); return;
      // multi-channel mode main menu items
    case 20: openAddChannelDialog(); return;
    case 21: addChannelsMultiWithSelectedTracks(); return;
    case 22: removeSelectedChannelsMulti(); return;
    case 23: removeAllChannelsMulti(); return;
    // channel items
    case 30: openSetChannelDialog(); return;
    case 31: openActiveChannelFX(); return;
    case 32: openActiveChannelTakeInMIDIEditor(); return;
    case 33: openActiveChannelItemProperties(); return;
    // row menu items
    case 40: openChannelSingle(iRowActive); return;
    case 41: swapChannelsMulti(iRowActive, iRowActive - 1); return;
    case 42: swapChannelsMulti(iRowActive, iRowActive + 1); return;
    case 43: removeChannelMulti(iRowActive); return;
  }
}

void sower::Editor::openChannelSingle(int idx) {
  auto chn = project.chnsMulti.getReference(idx);
  bool wasEmpty = (project.chnSingle.track == nullptr);
  project.chnSingle.track = chn.track;
  project.chnSingle.midiChn = chn.midiChn;

  toggleChannelMode();

  if (wasEmpty) {
    resizeWindow(project.iLeftColumnVisible, jmax(4, project.nRowsVisibleMulti));
    setRowVisible(127 - config.pitch);
  }

}

void sower::Editor::swapChannelsMulti(int idx1, int idx2) {
  std::swap(project.chnsMulti.getReference(idx1), project.chnsMulti.getReference(idx2));
  MarkProjDirty();
  if (idx1 == iRowActive) setRowActive(idx2);
  else if (idx2 == iRowActive) setRowActive(idx1);
  updateGrid();
}





void sower::Editor::openMainMenu(Component * c) {
  bool hasTracks = CountTracks(0) != 0;
  bool hasSelectedTracks = hasTracks;
  if (hasTracks) hasSelectedTracks = CountSelectedTracks(0) != 0;
  bool hasRows = getNumRowsMax() != 0;
  bool hasSelectedRows = getSelectedRows().size() != 0;
  PopupMenu m;

  if (isSingleMode()) {
    m.addItem(1, "Switch to multi-channel mode");
    m.addSeparator();
    addChannelMenuItems(m);
  }
  else {
    m.addItem(1, "Switch to single-channel mode");
    m.addSeparator();
    m.addItem(20, "Add channel...", (project.chnsMulti.size() != 128) && hasTracks);
    m.addItem(21, "Add selected tracks", hasSelectedTracks);
    m.addItem(22, "Remove selected channels", hasSelectedRows);
    m.addItem(23, "Remove all channels", hasRows);
    m.addSeparator();
  }
  m.addSeparator();
  PopupMenu::Item item;
  item.isEnabled = hasSelectedRows;
  item.itemID = 2;
  item.text = "Copy ";
  item.shortcutKeyDescription = "Ctrl+C ";
  m.addItem(item);
  item.itemID = 3;
  item.text = "Cut ";
  item.shortcutKeyDescription = "Ctrl+X ";
  m.addItem(item);
  item.itemID = 4;
  item.text = "Paste ";
  item.shortcutKeyDescription = "Ctrl+V ";
  m.addItem(item);
  m.addSeparator();
  m.addItem(50, "Triplets", true, useTriplets() != 0);
  PopupMenu mm;
  mm.addItem(101, "1", true, project.numBars == 1);
  mm.addItem(102, "2", true, project.numBars == 2);
  mm.addItem(104, "4", true, project.numBars == 4);

  m.addSubMenu("Number of bars:", mm);

  m.addSeparator();
  m.addItem(5,"Stay on top", true, project.flags & project.kStayOnTop);
  m.addItem(6,"Docked", true, isDocked());
  m.addItem(7,"User guide");
  processMenuResult(m.showAt(c));
}



void sower::Editor::addChannelMenuItems(PopupMenu & m) noexcept {
  m.addItem(30, "Set channel...");
  m.addSeparator();
  m.addItem(31, "View track FX");
  m.addItem(32, "Open take(s) in MIDI editor");
  m.addItem(33, "Open item properties");
}

void sower::Editor::openChannelMultiMenu() {
  PopupMenu m;
  addChannelMenuItems(m);
  m.addSeparator();
  m.addItem(40, "Open in single-channel mode");
  m.addSeparator();
  m.addItem(41, "Move up", iRowActive != 0);
  m.addItem(42, "Move down", iRowActive != getNumRowsMax() - 1);
  m.addItem(43, "Remove");
  processMenuResult(m.show());
}


void sower::Editor::openNoteDialog(int x, int y, int iQuant, int iRow) {
  if (modalComp) return;
  modalComp = new NoteDialog(x, y, iQuant, iRow);
  modalComp->enterModalState(false, nullptr, true);
}

void sower::Editor::openAddChannelDialog() {
  if (modalComp) return;
  modalComp = new ChannelDialog(0);
  modalComp->enterModalState(false, nullptr, true);
}

void sower::Editor::openSetChannelDialog() {
  if (modalComp) return;
  Project::Channel& chn = isSingleMode() ? project.chnSingle :
    project.chnsMulti.getReference(iRowActive);
  modalComp = new ChannelDialog(&chn);
  modalComp->enterModalState(false, nullptr, true);
}



void Editor::toggleDocked() {
  destroyWindow();
  project.flags ^= project.kDocked;
  MarkProjDirty();
  createWindow();
  resizeWindow(project.nColumnsVisible, project.getNumsRowsVisible());
  if(!isDocked()) grabKeyboardFocus();
}

#if JUCE_WINDOWS // mac is in sower_mac mm
void Editor::toggleAlwaysOnTop() {
  project.flags ^= project.kStayOnTop;
  MarkProjDirty(); 
  setAlwaysOnTop((project.flags & project.kStayOnTop) != 0);
}
#endif

void sower::Editor::toggleLoopState() {
  loopState = !loopState;
  if (loopState) {
    GetSet_LoopTimeRange2(0, true, true, &tmeBarStart, &tmeBarEnd, false);
    SetEditCurPos2(0, tmeBarStart, true, true);
    GetSetRepeatEx(0, 1);
    UpdateArrange();
  }
  else {
    GetSetRepeatEx(0, 0);
  }
  toolbar.btnLoop.setImage(&images[(loopState ? ImageIndex::loopOn : ImageIndex::loopOff)]);
  toolbar.btnLoop.setTooltip(String("Loop: ") + (loopState ? "on" : "off"));
}


void Editor::toggleChannelMode() {
  project.flags ^= project.kSingleMode;
  MarkProjDirty();
  updateGrid();
  header.setText(isSingleMode()? getTrackName(project.chnSingle.track) : String("multi"));
  
}

void Editor::toggleUseTriplets() {
  project.flags ^= project.kTriplets;
  MarkProjDirty();
  updateBar();
  toolbar.txtDenominator.setText(String(getDenominator()));
}

void Editor::toggleControlPanelVisible() {
  project.flags ^= project.kCPanelVisible;
  MarkProjDirty();
  bool isVisible = isControlPanelVisible();
  toolbar.btnControlPanel.setImage(
    &images[(isVisible? ImageIndex::cpanelOn : ImageIndex::cpanelOff)]);
  toolbar.btnControlPanel.setTooltip(String("Control panel: ") + (isControlPanelVisible() ? "on" : "off"));
  controlPanelHeader.setVisible(isVisible);
  splitterHorz.setVisible(isVisible);
  controlPanel.setVisible(isVisible);
  resizeWindow(project.nColumnsVisible, project.getNumsRowsVisible());
}

void sower::Editor::toggleItemStretchMode() {
  project.flags ^= project.kStretchItem;
  MarkProjDirty();
  toolbar.btnItemStretchMode.setImage(useStretchItem()? 
    &images[ImageIndex::modeStretch] : &images[ImageIndex::modeInsert]);
  toolbar.btnItemStretchMode.setTooltip(String("If there's no MIDI item, ")
    + (useStretchItem() ? "stretch the prev. one" : "insert a new one"));
  MarkProjDirty();
}

void sower::Editor::toggleSyncBar() {
  project.flags ^= project.kSyncBar;
  MarkProjDirty();
  toolbar.btnSyncBar.setImage(useSyncBar() ?
    &images[ImageIndex::syncOn] : &images[ImageIndex::syncOff]);
  toolbar.btnSyncBar.setTooltip(String("Sync bar: ") + (useSyncBar() ? "on" : "off"));

  if (useSyncBar()) {
    if (!isTimerRunning()) {
      Editor::instance->iColumnAnimated = -1;
      startTimer(70);
    }
  }
  else {
    if (isTimerRunning() && ((config.flags & config.kAnimatePlayback) == 0)) stopTimer();
  }
}

void sower::Editor::setTooltips(){
  if ((config.flags & config.kShowTooltips) != 0) {
    if (!tooltip) tooltip = new TooltipWindow();
  }
  else tooltip = nullptr;
}

void sower::Editor::playPreview(int iRow) noexcept {
  jassert((iRow >= 0) && (iRow < (isSingleMode()? 128 : project.chnsMulti.size())));
  StopTrackPreview2(0, &preview);
  midiwrite.events->Empty();
  auto track = isSingleMode() ? project.chnSingle.track : project.chnsMulti[iRow].track;
  if (!track) return;
  
  int midiChn = isSingleMode() ? project.chnSingle.midiChn : project.chnsMulti[iRow].midiChn;
  int pitch = isSingleMode() ? (127 - iRow) : project.chnsMulti[iRow].pitch;

  int replaceFlag = 1;

  MIDI_event_t evt;
  evt.size = 3;
  evt.frame_offset = 0;
  evt.midi_message[0] = 0x90 | (unsigned char)midiChn; //note on
  evt.midi_message[1] = (unsigned char)pitch;
  evt.midi_message[2] = 100;
  midiwrite.events->AddItem(&evt);
  evt.frame_offset = ppqQN;
  evt.midi_message[0] = 0x80 | (unsigned char)midiChn; //note off
  evt.midi_message[2] = 0;
  midiwrite.events->AddItem(&evt);

  preview.curpos = 0.0;
  preview.src->Extended(PCM_SOURCE_EXT_ADDMIDIEVENTS, &midiwrite, &replaceFlag, 0);
  preview.preview_track = track;

  PlayTrackPreview2(0, &preview);
  
}

void sower::Editor::copyRows(bool bCut) {
  copiedRows.clearQuick();
  auto& selectedRows = getSelectedRows();
  if (selectedRows.isEmpty()) return;
  copiedRows.resize(selectedRows.size());
  Array<RowCopyNote>* pNotes = copiedRows.begin();
  for (int* pRow = selectedRows.begin(); pRow != selectedRows.end(); ++pRow, ++pNotes) {
    Quant* pQuants = getQuants(*pRow);
    for (int iQuant = 0; iQuant != numQuants; ++iQuant) {
      Quant& quant = pQuants[iQuant];
      if (quant.isSet()) {
        pNotes->add({ (float)(iQuant * ppqQuant + (int)quant.ppqQuantOffset), (int)quant.ppqLength, quant.velocity });
        if (bCut) removeNote(*pRow, iQuant, false);
      }
    }
  }
  if (bCut) {
    if (!copiedRows.isEmpty())  {
      if ((config.flags & config.kUndoCutPaste) !=0)  Undo_OnStateChange2(0, "Sower: Cut notes");
      grid.repaint();
    }
  }
  updateGrid();
}


void sower::Editor::pasteRows() {
  auto selectedRows = getSelectedRows();
  int nRowsMax = jmin(selectedRows.size(), copiedRows.size());
  if (!nRowsMax) return;
  for (int i = 0; i != nRowsMax; ++i) {
    int iRow = selectedRows[i];
    for (int iQuant = 0; iQuant != numQuants; ++iQuant) {
      removeNote(iRow, iQuant, false);
    }
  }
  double ppqEnd = ppqQuant * numQuants - ppqQuant / 2;
  Array<RowCopyNote>* pNotes = copiedRows.begin();
  for (int i = 0; i != nRowsMax; ++i, ++pNotes) {
    int iRow = selectedRows[i];
    auto track = isSingleMode() ? project.chnSingle.track : project.chnsMulti[iRow].track;
    int midiChn = isSingleMode() ? project.chnSingle.midiChn : project.chnsMulti[iRow].midiChn;
    int pitch = isSingleMode() ? (127 - iRow) : project.chnsMulti[iRow].pitch;
    for (auto& note: *pNotes) {
      if (note.ppqStartInBar >= ppqEnd) break;
      insertNote(note.ppqStartInBar, track, 
        midiChn, pitch, note.velocity, note.ppqLength, false);
    }
  }

  if ((config.flags & config.kUndoCutPaste) != 0) {
    Undo_OnStateChange2(0, "Sower: Paste notes");
  }
  updateGrid();
}




// helper function
inline void randomizeValue(Random& r, int& val, int start, int end, double amt) {
  jassert((val >= start) && (val < end) && (amt >= 0.0f) && (amt <= 1.0f));
  int range = end - start;
  if (amt != 1.0f) {
    range = (int)(range * amt);
    start = jlimit(start, end - range, val - range / 2);
  }
  val = start + ((unsigned)r.nextInt()) % range;
}


void sower::Editor::processRows() {
  MidiProcessorConfig& cfg = config.midiProcessor;
  if ((cfg.flags & cfg.kProcessMask) == 0) return;
  if (getSelectedRows().isEmpty()) {
    NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "Sower",
      "No row is selected", this);
    return;
  }
  double tmeStart, tmeEnd;
  int iBarStart, iBarEnd, iBarOld;
  if ((cfg.flags & cfg.kProcessSelectedBars) != 0) {
    GetSet_LoopTimeRange2(0, false, false, &tmeStart, &tmeEnd, false);
    TimeMap2_timeToBeats(0, tmeStart, &iBarStart, 0, 0, 0);
    TimeMap2_timeToBeats(0, tmeEnd, &iBarEnd, 0, 0, 0);
    iBarOld = project.iBar;
    do {
      if (iBarStart != project.iBar) {
        project.iBar = iBarStart;
        updateBar();
      }
      processBar(); ++iBarStart; } 
    while (iBarStart < iBarEnd);
    if (iBarOld != project.iBar) {
      project.iBar = iBarOld;
      updateBar();
    }
  }
  else processBar();
}

void sower::Editor::processBar() {
  MidiProcessorConfig& cfg = config.midiProcessor;
  Random r;
  for (int* pRow = getSelectedRows().begin(); pRow != getSelectedRows().end(); ++pRow) {
    int iRow = *pRow;
    Quant* pQuant = getQuants(iRow);

    struct Note {
      double ppqStartInBar;
      int ppqLength;
      int velocity;
    };

    Array<Note> notes;
    // cut notes to array
    for (int iQuant = 0; iQuant != numQuants; ++iQuant, ++pQuant) {
      Quant& quant = *pQuant;
      if (!quant.isSet()) continue;
      if (iQuant == 0) {
        if ((cfg.flags & cfg.kExcludeFirstBeat) != 0) continue;
      }
      else {
        if ((cfg.flags & (cfg.kExcludeOnBeats | cfg.kExcludeOffBeats)) != 0) {
          int iCol = quantToColumnFloor(iQuant, cfg.excludeResolution);
          if (columnToQuant(iCol, cfg.excludeResolution) == iQuant) {
            bool isOffBeat = (useTriplets() ? iCol % 3 : iCol & 1) != 0;
            if ((isOffBeat) && ((cfg.flags & cfg.kExcludeOffBeats) != 0)) continue;
            if ((!isOffBeat) && ((cfg.flags & cfg.kExcludeOnBeats) != 0)) continue;
          }
        }
      }
      // process note length and velocity before adding them to the array
      int offs = quant.ppqQuantOffset;
      int len = quant.ppqLength;
      int vel = quant.velocity;

      if ((cfg.flags & cfg.kQuantizeLength) != 0) {
        len -= (int)((len - cfg.quantizeLengthValue) * cfg.amtQuantize);
        len = jlimit((int)1, (int)UINT16_MAX, len);
      }
      if ((cfg.flags & cfg.kRandomizeLength) != 0) {
        randomizeValue(r, len, 1, (int)getColumnTicks(0) * 4, cfg.amtRandomize);
      }
      if ((cfg.flags & cfg.kQuantizeVelocity) != 0) {
        vel -= (int)((vel - cfg.quantizeVelocityValue) * cfg.amtQuantize);
        vel = jlimit(1, 127, vel);
      }
      if ((cfg.flags & cfg.kRandomizeVelocity) != 0) {
        randomizeValue(r, vel, 1, 128, cfg.amtRandomize);
      }

      notes.add({iQuant * ppqQuant + offs, len, vel });
      removeNote(iRow, iQuant, false);
    }

    if ((cfg.flags & cfg.kQuantizePosition) != 0) {

      struct NoteQuant {
        Note* note;
        double ppqOffset;
      };

      Array<NoteQuant> quantNotes;

      double ppqQnt = getColumnTicks(cfg.quantizePosResolution);  // ticks per quant
      quantNotes.resize(quantToColumnFloor(numQuants, cfg.quantizePosResolution));
      // fill quants with null note ref and offset limit
      for (auto& quantNote : quantNotes) {
        quantNote.note = nullptr;
        quantNote.ppqOffset = ppqQnt / 2 * cfg.quantizePosWindow;
      }
      // quantize
      for (auto& note : notes) {
        double ppqPos = note.ppqStartInBar + ppqQnt / 2;
        int iQuant = (int)(ppqPos / ppqQnt);
        if (iQuant >= quantNotes.size())  continue;
        double offs = fmod(ppqPos, ppqQnt) - ppqQnt / 2;
        auto& quantNote = quantNotes.getReference(iQuant);
        if (fabs(quantNote.ppqOffset) <= fabs(offs)) continue;
        quantNote.note = &note;
        quantNote.ppqOffset = offs;
      }
 
      for (int i = 0; i != quantNotes.size(); ++i) {
        auto& quantNote = quantNotes.getReference(i);
        if (quantNote.note == nullptr) continue;
        double offs = quantNote.ppqOffset;
        double ppqNotePosInBar = i * ppqQnt + offs;
        if ((cfg.flags & cfg.kSwing) != 0) {
          bool isOffBeat;
          if (!useTriplets()) isOffBeat = ((i & 1) != 0);
          else isOffBeat = ((i % 3) == 2);
          if (isOffBeat) offs -= ppqQnt / 3  * cfg.amtSwing;
        }
        
        if (cfg.amtQuantize != 1.0) offs *= cfg.amtQuantize;
        quantNote.note->ppqStartInBar = ppqNotePosInBar - offs;
       
      } // for quantNotes

      if ((cfg.flags & cfg.kDeleteExtraNotes) != 0) {
        Array<Note> tmp;
        for (auto& quantNote : quantNotes) {
          if (quantNote.note != nullptr) tmp.add(*(quantNote.note));
        }
        std::swap(tmp, notes);
      }

    } // if ((cfg.flags & cfg.kQuantizePosition) != 0) 

    for (auto& note : notes) {
      double ppqNoteStartInRow = note.ppqStartInBar + ppqQuant / 2;
      int iQuant = (int)(ppqNoteStartInRow / ppqQuant);
      int offs = (int)(fmod(ppqNoteStartInRow, ppqQuant) - ppqQuant / 2);
      if ((cfg.flags & cfg.kRandomizePosition) != 0) {
        randomizeValue(r, offs, ((int)-ppqQuant / 2), ((int)ppqQuant / 2) - 1, cfg.amtRandomize);
      }
      Quant& quant = quants.getReference(iRow * numQuants + iQuant);
      if (!quant.isSet()) {
        insertNote(iRow, iQuant, note.velocity, offs, note.ppqLength, false);
      }
    }

  } // for each selected row

  Undo_OnStateChange2(0, "Sower: process notes");
  repaint();

}


void sower::Editor::openMidiProcessor() {
  if(midiProcessor == nullptr)  midiProcessor = new MIDIProcDialog();
}

void sower::Editor::openOptions() {
  if (modalComp) return; 
  modalComp = new OptionsDialog();
  modalComp->enterModalState(false, nullptr, true);
}

void sower::Editor::openActiveChannelFX() {
  MediaTrack* trk = 0;
  if (isSingleMode()) trk = project.chnSingle.track;
  else if (iRowActive != -1) trk = project.chnsMulti.getReference(iRowActive).track;
  int i = TrackFX_GetInstrument(trk);
  int flag = (i == -1 ? 1 : 3);
  TrackFX_Show(trk, i, flag);
}


int sower::Editor::selectActiveChannelItems() noexcept  {
  MediaTrack* trk = nullptr;
  if (isSingleMode()) trk = project.chnSingle.track;
  else if (project.chnsMulti.size() != 0) trk = project.chnsMulti.getReference(iRowActive).track;
  if (!trk) return false;

  SelectAllMediaItems(0, false);

  double ppqBar = ppqQuant * numQuants;
  int i = 0;
  for (int iItem = 0; MediaItem* item = GetTrackMediaItem(trk, iItem); ++iItem) {
    MediaItem_Take* take = GetActiveTake(item);
    double ppqTakeStartInBar = ppqQuant / 2 - MIDI_GetPPQPosFromProjTime(take, tmeBarStart);
    if (ppqTakeStartInBar >= ppqBar) break;
    double tmeItemStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", 0);
    double tmeItemEnd = tmeItemStart + *(double*)GetSetMediaItemInfo(item, "D_LENGTH", 0);
    double ppqTakeEndInBar = ppqTakeStartInBar + MIDI_GetPPQPosFromProjTime(take, tmeItemEnd);
    if (ppqTakeEndInBar <= 0.0) continue;
    SetMediaItemSelected(item, true);
    ++i;
  }
  return i;
}


void sower::Editor::openActiveChannelTakeInMIDIEditor() noexcept {
  if (selectActiveChannelItems() != 0) Main_OnCommand(40109, 0);
}

void sower::Editor::openActiveChannelItemProperties() noexcept {
  if (selectActiveChannelItems() != 0) Main_OnCommand(41589, 0);
}




void Editor::addChannelMulti(MediaTrack* trk, int pitch, int midiChn) noexcept {
  Project::Channel ch = { trk, midiChn, pitch };
  int iRow = project.chnsMulti.indexOf(ch);
  if (iRow == -1) {
    iRow = project.chnsMulti.size();
    if (iRow == 128) return;
    project.chnsMulti.add({ trk, midiChn, pitch });
    MarkProjDirty();
    ++project.nRowsVisibleMulti;
    updateGrid();
  }
  setRowActive(iRow);
  setRowVisible(iRow);
}

void Editor::addChannelsMultiWithSelectedTracks() noexcept {
  int trkflags;
  int nTracks = CountTracks(0);
  int iRow = iRowActive;

  for (int iTrack = 0; iTrack < nTracks; ++iTrack) {
    GetTrackInfo(iTrack, &trkflags);
    if (trkflags & 2) {
      MediaTrack* trk = GetTrack(0, iTrack);
      Project::Channel ch = { trk, config.midiChn, config.pitch };
      int i = project.chnsMulti.indexOf(ch);
      if (i == -1) {
        i = project.chnsMulti.size();
        if (i == 128) break;
        project.chnsMulti.add(ch);
        MarkProjDirty();
        ++project.nRowsVisibleMulti;
      }
      iRow = i;
    }
  }

  updateGrid();
  setRowActive(iRow);
  setRowVisible(iRow);
  
}

void Editor::setChannelMulti(Project::Channel& chn, MediaTrack* trk, int pitch, int midiChn) noexcept {
  if (project.chnsMulti.indexOf({ trk, midiChn, pitch }) != -1) return;
  chn = { trk, midiChn, pitch };
  MarkProjDirty();
  updateGrid();
}

void Editor::setChannelSingle(MediaTrack* trk, int midiChn) noexcept {
  if((project.chnSingle.track == trk) && (project.chnSingle.midiChn == midiChn)) return;
  bool wasEmpty = (project.chnSingle.track == nullptr);
  project.chnSingle.track = trk;
  project.chnSingle.midiChn = midiChn;
  MarkProjDirty();
  updateGrid();
  if (wasEmpty) {
    resizeWindow(project.iLeftColumnVisible, jmax(4, project.nRowsVisibleMulti));
    setRowVisible(127 - config.pitch);
  }
}

void Editor::removeChannelMulti(int idx) noexcept {
  if (project.chnsMulti.size() <= idx) return; // no need but who knows
  project.chnsMulti.remove(idx);
  selectedRowsMulti.removeFirstMatchingValue(idx);
  iRowActive = jmin(iRowActive, project.chnsMulti.size());
  updateGrid();
  MarkProjDirty();
}

void sower::Editor::removeSelectedChannelsMulti() noexcept {
  for(auto idx: selectedRowsMulti) {
    project.chnsMulti.remove(idx);
  }
  selectedRowsMulti.clearQuick();
  iRowActive = jmin(iRowActive, project.chnsMulti.size());
  updateGrid();
  MarkProjDirty();
}

void Editor::removeAllChannelsMulti() noexcept {
  project.chnsMulti.clearQuick();
  selectedRowsMulti.clearQuick();
  iRowActive = 0;
  updateGrid();
  MarkProjDirty();
}

void Editor::setRowVisible(int idx) {
  idx = jlimit(0, getNumRowsMax(), idx);
  int iTop = project.getTopmostRowVisible();
  int d = idx - iTop;
  d -= (d > 0 ? project.getNumsRowsVisible() - 1 : 0);
  if (d == 0) return;
  gridHolder.setViewPosition(gridHolder.getViewPositionX(), (iTop + d) * kRowHeight);
}

void Editor::setRowActive(int idx) noexcept {
  idx = jlimit(0, getNumRowsMax(), idx);
  int iRowActiveOld = iRowActive;
  if (iRowActiveOld == idx) return;
  iRowActive = idx;
  gridHeader.repaint(0, (iRowActiveOld - project.getTopmostRowVisible()) * kRowHeight, gridHeader.getWidth(), kRowHeight);
  gridHeader.repaint(0, (idx - project.getTopmostRowVisible()) * kRowHeight, gridHeader.getWidth(), kRowHeight);
  controlPanel.repaint();
}

void sower::Editor::setBar(int i) {
  i = jlimit(0, INT_MAX - getNumBars(), i);
  if (i == project.iBar) return;
  project.iBar = i; 
  updateBar(); 
  if (useSyncBar() || loopState) {
    if (loopState) GetSet_LoopTimeRange2(0, true, true, &tmeBarStart, &tmeBarEnd, false);
    SetEditCurPos2(0, tmeBarStart, true, true);
    UpdateArrange();
  }
  iColumnAnimated = -1;
  MarkProjDirty();
}



void sower::Editor::setBarToPrevious() { advanceBar(-getNumBars());}
  
void sower::Editor::setBarToNext() { advanceBar(getNumBars()); }


void sower::Editor::advanceBar(int delta) {
  setBar(project.iBar + delta);
}

void sower::Editor::barScrolled(int delta) {
  int i = jlimit((int)0, INT_MAX - getNumBars(), project.iBar + delta);
  toolbar.txtBar.setText(String (i + 1));
}



void Editor::updateBar()  {


  quantDenominator = (useTriplets() ? 6 : 4) << config.maxResolution;
  qnQuant = 4.0 / quantDenominator;

  double tempoOut, qn_startOut, qn_endOut;
  int timesig_numOut, timesig_denomOut;

  TimeMap_GetMeasureInfo(0, project.iBar, &qn_startOut, &qn_endOut, &timesig_numOut,
    &timesig_denomOut, &tempoOut);
  qnBarStart = qn_startOut;
  if (project.numBars > 1) {
    TimeMap_GetMeasureInfo(0, project.iBar + project.numBars - 1, &qn_startOut, &qn_endOut, &timesig_numOut,
      &timesig_denomOut, &tempoOut);
  }
  qnBarEnd = qn_endOut;

  double qnBarLength = qnBarEnd - qnBarStart;
  numQuants = (int)(qnBarLength / qnQuant);
  qnBarEnd -= fmod(qnBarLength, qnQuant); // round down time sig-s like 3/6 or 4/7
  tmeBarStart = TimeMap2_QNToTime(0, qnBarStart);
  tmeBarEnd = TimeMap2_QNToTime(0, qnBarEnd);
  ppqQuant = ppqQN * qnQuant;

  project.resolution = jlimit(0, config.maxResolution, project.resolution);

  minResolution = 0; 
  while ((qnBarLength < 1) && (minResolution != config.maxResolution)){
    qnBarLength *= 2;
    ++minResolution;
  }
  updateGrid();

  toolbar.txtBar.setText(String(project.iBar + 1));

}

void Editor::updateGrid() {

  quants.clearQuick();
  
  numMinResolutionNotes.clearQuick();
  numMinResolutionNotes.resize(config.maxResolution + 1);

  double ppqBar = ppqQuant * numQuants;

  if (isSingleMode()) {
    quants.resize(numQuants * 128);
    Quant* pQuants = quants.getRawDataPointer();
    auto& chn = project.chnSingle;
    for (int iItem = 0; MediaItem* item = GetTrackMediaItem(chn.track, iItem); ++iItem) {
      MediaItem_Take* take = GetActiveTake(item);
      double ppqTakeStartInBar = ppqQuant / 2 - MIDI_GetPPQPosFromProjTime(take, tmeBarStart);
      if (ppqTakeStartInBar >= ppqBar) break;
      double tmeItemStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", 0);
      double tmeItemEnd = tmeItemStart + *(double*)GetSetMediaItemInfo(item, "D_LENGTH", 0);
      double ppqTakeEndInBar = ppqTakeStartInBar + MIDI_GetPPQPosFromProjTime(take, tmeItemEnd);
      if (ppqTakeEndInBar <= 0.0) continue;

      int pitchOut, chanOut, velOut;				// MIDI_GetNote() output variables
      bool selectedOut, mutedOut;
      double startppqposOut, endppqposOut;
      for (int iNote = 0; MIDI_GetNote(take, iNote, &selectedOut, &mutedOut, &startppqposOut, &endppqposOut,
        &chanOut, &pitchOut, &velOut); ++iNote) {
        if (chanOut != chn.midiChn) continue;

        double ppqNotePosInBar = ppqTakeStartInBar + startppqposOut;

        if (ppqNotePosInBar < 0.0) continue;	 // omit any preceding notes
        if (ppqNotePosInBar >= ppqBar) break;
        int iQuant = (int)(ppqNotePosInBar / ppqQuant);
        int ppqQuantOffset = (int)(fmod(ppqNotePosInBar, ppqQuant) - ppqQuant / 2);

        Quant& quant = pQuants[numQuants * (127 - pitchOut) + iQuant];
        if (quant.isSet()) {
          if (abs(quant.ppqQuantOffset) <= abs(ppqQuantOffset)) continue;
        }
        quant.take = take;
        quant.ppqQuantOffset = (uint16_t)ppqQuantOffset;
        quant.ppqLength = (uint32_t)(endppqposOut - startppqposOut);
        quant.velocity = (uint8_t)velOut;

        int res = getMinResolution(iQuant);
        ++(*(numMinResolutionNotes.getRawDataPointer() + res));
        minResolution = jmax(res, minResolution);
       
      } // for note
    } // for item    
  } // if singlechannel mode

  else {  // multichannel
    quants.resize(project.chnsMulti.size() * numQuants);
    Quant* pQuants = quants.getRawDataPointer();
    for (auto& chn : project.chnsMulti) {
      for (int iItem = 0; MediaItem* item = GetTrackMediaItem(chn.track, iItem); ++iItem) {
        MediaItem_Take* take = GetActiveTake(item);
        double ppqTakeStartInBar = ppqQuant / 2 - MIDI_GetPPQPosFromProjTime(take, tmeBarStart);
        if (ppqTakeStartInBar >= ppqBar) break;
        double tmeItemStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", 0);
        double tmeItemEnd = tmeItemStart + *(double*)GetSetMediaItemInfo(item, "D_LENGTH", 0);
        double ppqTakeEndInBar = ppqTakeStartInBar + MIDI_GetPPQPosFromProjTime(take, tmeItemEnd);
        if (ppqTakeEndInBar <= 0.0) continue;

        int pitchOut, chanOut, velOut;				// MIDI_GetNote() output variables
        bool selectedOut, mutedOut;
        double startppqposOut, endppqposOut;
        for (int iNote = 0; MIDI_GetNote(take, iNote, &selectedOut, &mutedOut, &startppqposOut, &endppqposOut,
          &chanOut, &pitchOut, &velOut); ++iNote) {
          if ((chanOut != chn.midiChn) || (pitchOut != chn.pitch)) continue;

          double ppqNotePosInBar = ppqTakeStartInBar + startppqposOut;

          if (ppqNotePosInBar < 0.0) continue;	 // omit any preceding notes
          if (ppqNotePosInBar >= ppqBar) break;
          int iQuant = (int)(ppqNotePosInBar / ppqQuant);
          int ppqQuantOffset = (int)(fmod(ppqNotePosInBar, ppqQuant) - ppqQuant / 2);

          Quant& quant = pQuants[iQuant];
          if (quant.isSet()) {
            if (abs(quant.ppqQuantOffset) <= abs(ppqQuantOffset)) continue;
          }
          quant.take = take;
          quant.ppqQuantOffset = (uint16_t)ppqQuantOffset;
          quant.ppqLength = (uint32_t)(endppqposOut - startppqposOut);
          quant.velocity = (uint8_t)velOut;

          int res = getMinResolution(iQuant);
          ++(*(numMinResolutionNotes.getRawDataPointer() + res));
          minResolution = jmax(res, minResolution);
        } // for note
      } // for item
      pQuants += numQuants;
    } // for channel
  } // if multichannel mode

  
  setRowActive(jmax(0, jmin(iRowActive, getNumRowsMax())));
  controlPanelHeader.updateRangeOffset();

  int iCol = project.iLeftColumnVisible;
  int nCols = project.nColumnsVisible;
  int iRow = project.getTopmostRowVisible();
  int nRows = project.getNumsRowsVisible();

  grid.setSize(getNumColumnsMax() * kColumnWidth, getNumRowsMax() * kRowHeight);

  resizeWindow(nCols, nRows);
  repaint();
  gridHolder.setViewPosition(iCol * kColumnWidth, iRow * kRowHeight);
  if (minResolution > project.resolution) setResolution(minResolution);

}


void sower::Editor::setNumberOfBars(int n) {
  int d = n - project.numBars;
  if (!d) return;
  project.numBars = n;
  updateBar();
  if (d > 0) resizeWindow(getNumColumnsMax() - project.iLeftColumnVisible, project.getNumsRowsVisible());
  if (loopState) {
    GetSet_LoopTimeRange2(0, true, true, &tmeBarStart, &tmeBarEnd, false);
  }
  MarkProjDirty();
}

void Editor::setResolution(int n) {
  n = jlimit(minResolution, config.maxResolution, n);
  int d = n - project.resolution;
  if (!d) return;
  project.resolution = n;
  int iCol = project.iLeftColumnVisible;
  int nCols = project.nColumnsVisible;
  grid.setSize(getNumColumnsMax() * kColumnWidth, grid.getHeight());
  if (d < 0) { d = -d; iCol >>= d;  nCols >>= d; }
  else { iCol <<= d;  nCols <<= d; }
  resizeWindow(nCols, project.getNumsRowsVisible());
  gridHolder.setViewPosition(iCol * kColumnWidth, gridHolder.getViewPositionY());
  toolbar.txtDenominator.setText(String(getDenominator()));
  iColumnAnimated = -1;
  MarkProjDirty();
}


  void sower::Editor::resolutionScrolled(int delta) {
    int n = jlimit(minResolution, config.maxResolution, project.resolution + delta);
    toolbar.txtDenominator.setText(String(getDenominator(n)));
  }

  void sower::Editor::advanceResolution(int delta) {
    setResolution(jlimit(minResolution, config.maxResolution, project.resolution + delta));
  }

  void sower::Editor::paint(Graphics& g) {
    g.setColour(backColor);
    g.fillAll();
  }

  void sower::Editor::timerCallback() {
    if ((config.flags & config.kAnimatePlayback)) {
      if (isPlaying()) {
        double tmePlayPos = GetPlayPositionEx(0);
        bool isOutsideBar = ((tmePlayPos < tmeBarStart) || (tmePlayPos >= tmeBarEnd));
        if (isOutsideBar) {
          int iBar;
          TimeMap2_timeToBeats(0, tmePlayPos, &iBar, 0, 0, 0);
          project.iBar = iBar;
          updateBar();
          iColumnAnimated = -1;
        }
        if (iColumnAnimated != -1) grid.repaint(iColumnAnimated * kColumnWidth,
          0, kColumnWidth, grid.getHeight());
        iColumnAnimated = (int)((tmePlayPos - tmeBarStart) /
          (tmeBarEnd - tmeBarStart) * getNumColumnsMax());
        grid.repaint(iColumnAnimated * kColumnWidth, 0, kColumnWidth, grid.getHeight());
        return;
      }
      if (iColumnAnimated != -1) {
        grid.repaint(iColumnAnimated * kColumnWidth, 0, kColumnWidth, grid.getHeight());
        iColumnAnimated = -1;
        double tmePlayPos = GetCursorPositionEx(0);
        bool isOutsideBar = ((tmePlayPos < tmeBarStart) || (tmePlayPos >= tmeBarEnd));
        if (isOutsideBar) {
          int iBar;
          TimeMap2_timeToBeats(0, tmePlayPos, &iBar, 0, 0, 0);
          project.iBar = iBar;
          updateBar();

        }
      }
    }
    if (useSyncBar()) {
      double tmePlayPos = GetCursorPositionEx(0);
      bool isOutsideBar = ((tmePlayPos < tmeBarStart) || (tmePlayPos >= tmeBarEnd));
      if (isOutsideBar) {
        int iBar;
        TimeMap2_timeToBeats(0, tmePlayPos, &iBar, 0, 0, 0);
        project.iBar = iBar;
        updateBar();
        iColumnAnimated = -1;
      }
    }
  }

  void sower::Editor::updateNote(int iRow, int iQuant,
    int velocity, int ppqOffset, int ppqLength, bool bUndo) noexcept {

    removeNote(iRow, iQuant, false);
    insertNote(iRow, iQuant, velocity, ppqOffset, ppqLength, false);
    if (bUndo && (config.flags & config.kUndoSetNote)) Undo_OnStateChange2(0, "Sower: Set note properties");
    ppqNoteLengthUsed = ppqLength;
    noteVelocityUsed = velocity;
  }



  void Editor::insertNote(int iRow, int iQuant) noexcept {
    insertNote(iRow, iQuant, noteVelocityUsed ? noteVelocityUsed : 100,
      0, ppqNoteLengthUsed ? ppqNoteLengthUsed : (int)getColumnTicks());
  }

  void Editor::insertNote(int iRow, int iQuant, int velocity,
  int ppqOffset, int ppqLength, bool bUndo) noexcept {
  jassert((iRow >= 0) && (iRow < (isSingleMode() ? 128 : project.chnsMulti.size())));
  jassert((iQuant >= 0) && (iQuant < numQuants));
  jassert((ppqOffset >= (-ppqQuant/2)) && (ppqOffset < ppqQuant / 2));
  if (project.iBar == 0) {
    if ((iQuant == 0) && (ppqOffset < 0)) {
      ppqOffset = 0;
    }
  }
  Quant& quant = quants.getReference(iRow * numQuants + iQuant);
  if (quant.isSet()) return;
  auto track = isSingleMode() ? project.chnSingle.track : project.chnsMulti[iRow].track;
  if (!track) return;
  int midiChn = isSingleMode() ? project.chnSingle.midiChn : project.chnsMulti[iRow].midiChn;
  int pitch = isSingleMode() ? (127 - iRow) : project.chnsMulti[iRow].pitch;


  MediaItem_Take* take = insertNote(ppqQuant * iQuant + ppqOffset,
    track, midiChn, pitch, velocity, ppqLength, 
    bUndo && (config.flags & config.kUndoInsertDelete) != 0);
  if (take != nullptr) {
    quant.take = take;
    quant.ppqLength = (uint32_t)ppqLength;
    quant.ppqQuantOffset = (uint16_t)ppqOffset;
    quant.velocity = (uint8_t)velocity;

    int res = getMinResolution(iQuant);
    ++(*(numMinResolutionNotes.getRawDataPointer() + res));
    minResolution = jmax(res, minResolution);
    if (minResolution > project.resolution) setResolution(minResolution);
  }
  }


MediaItem_Take* Editor::insertNote(double ppqNoteStartInBar, MediaTrack* track, int midiChn,
  int pitch, int velocity, int ppqLength, bool bUndo) noexcept {

  MediaItem* itemToUse = nullptr;

  double tmeItemToStretchOrInsertStart = 0.0;
  double tmeItemToStretchOrInsertEnd = tmeBarEnd;

  MediaItem_Take* takeToUse = nullptr;
  double ppqNoteStartInTake = 0.0;
  bool found = false;


  for (int iItem = 0; MediaItem* item = GetTrackMediaItem(track, iItem); ++iItem) {
    double tmeItemStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", 0);
    MediaItem_Take* take = GetActiveTake(item);
    double ppqBarPosInTake = MIDI_GetPPQPosFromProjTime(take, tmeBarStart);
    ppqNoteStartInTake = ppqBarPosInTake + ppqNoteStartInBar;

    if (ppqNoteStartInTake < 0.0) {
      tmeItemToStretchOrInsertEnd = jmin(tmeBarEnd, tmeItemStart);
      break;
    }

    itemToUse = item;
    takeToUse = take;

    double tmeItemEnd = tmeItemStart + *(double*)GetSetMediaItemInfo(item, "D_LENGTH", 0);
    double ppqTakeLength =  MIDI_GetPPQPosFromProjTime(take, tmeItemEnd);

    if (ppqNoteStartInTake < ppqTakeLength) {
      found = true; break;
    }

    tmeItemToStretchOrInsertStart = (useStretchItem() ? tmeItemStart : tmeItemEnd);
  }


  if (!found) {
    
    if (useStretchItem() && (itemToUse != nullptr)) {
      double tmeLen = tmeItemToStretchOrInsertEnd - tmeItemToStretchOrInsertStart;
      GetSetMediaItemInfo(itemToUse, "D_LENGTH", &tmeLen);
      double ppqTakeEnd = MIDI_GetPPQPosFromProjTime(takeToUse, tmeItemToStretchOrInsertEnd);
      if (MIDI_InsertCC(takeToUse, false, false, ppqTakeEnd, 176, midiChn, 123, 0)) {
        double startppqposOut;
        bool selectedOut, mutedOut;
        int chanmsgOut, chanOut, msg2Out, msg3Out;
        for (int i = 0; MIDI_GetCC(takeToUse, i, &selectedOut, &mutedOut, &startppqposOut,
          &chanmsgOut, &chanOut, &msg2Out, &msg3Out); ) {
          if ((chanOut == midiChn) && (msg2Out == 123) && (startppqposOut < ppqTakeEnd)) {
            MIDI_DeleteCC(takeToUse, i); continue;
          }
          ++i;
        }
      }
    }
    else {
      itemToUse = CreateNewMIDIItemInProj(track, jmax(tmeBarStart, tmeItemToStretchOrInsertStart),
        tmeItemToStretchOrInsertEnd, 0);
      takeToUse = GetActiveTake(itemToUse);
      double ppqBarPos = MIDI_GetPPQPosFromProjTime(takeToUse, tmeBarStart);
      ppqNoteStartInTake = ppqBarPos + ppqNoteStartInBar;
      UpdateArrange();
    }
  }
  double ppqNoteEndInTake = ppqNoteStartInTake + ppqLength;
  if (MIDI_InsertNote(takeToUse, false, false, ppqNoteStartInTake,
    ppqNoteEndInTake, midiChn, pitch, velocity, 0)) {

    if (bUndo) {
      if ((config.flags & config.kUndoInsertDelete) != 0) {
        Undo_OnStateChange2(0, "Sower: Insert note");
      }
    }
    UpdateItemInProject(itemToUse);
    return takeToUse;
  }
  return nullptr;
}

 void Editor::removeNote(int iRow, int iQuant, bool bUndo) noexcept {
   jassert((iRow >= 0) && (iRow < (isSingleMode() ? 128 : project.chnsMulti.size())));
   jassert((iQuant >= 0) && (iQuant < numQuants));

   Quant& quant = quants.getReference(iRow * numQuants + iQuant);
   if (!quant.isSet()) return;
   MediaTrack* track = isSingleMode() ? project.chnSingle.track : project.chnsMulti[iRow].track;
   int midiChn = isSingleMode() ? project.chnSingle.midiChn : project.chnsMulti[iRow].midiChn;
   int pitch = isSingleMode() ? (127 - iRow) : project.chnsMulti[iRow].pitch;
    
   if (removeNote(track, quant.take, iQuant * ppqQuant + quant.ppqQuantOffset,
     (uint8_t)midiChn, (uint8_t)pitch,  bUndo && (config.flags & config.kUndoInsertDelete) != 0)) {
     quant.take = nullptr;
     int res = getMinResolution(iQuant);
     int* pNum = numMinResolutionNotes.getRawDataPointer() + res;
     --(*pNum);
     if (!(*pNum) && (minResolution == res)) {
       while (minResolution) {
         --minResolution;
         if (numMinResolutionNotes[minResolution]) break;
       };
     }
   }
 }

bool Editor::removeNote(MediaTrack* track, MediaItem_Take * take, double ppqNoteStartInBar,
  int midiChn, int pitch, bool bUndo) noexcept {
   
  double ppqNoteStartInTake = ppqNoteStartInBar + MIDI_GetPPQPosFromProjTime(take, tmeBarStart);

  double startppqposOut, endppqposOut;
  bool selectedOut, mutedOut;
  int midiChnOut, pitchOut, velOut;

  int iNoteToRemove = -1;
  double ppqOffset = DBL_MAX;

  for (int iNote = 0; MIDI_GetNote(take, iNote, &selectedOut, &mutedOut, &startppqposOut, &endppqposOut,
    &midiChnOut, &pitchOut, &velOut); ++iNote) {
    if ((midiChnOut != midiChn) || (pitchOut != pitch)) continue;
    double d = fabs(startppqposOut - ppqNoteStartInTake);
    if (d >= ppqOffset) {
      if (iNoteToRemove <= iNote) break;
    }
    iNoteToRemove = iNote;
    ppqOffset = d;
  }
  if (iNoteToRemove == -1) return false;
  if (MIDI_DeleteNote(take, iNoteToRemove)) {
    MediaItem* item = GetMediaItemTake_Item(take);
    if ((config.flags & config.kAutoDeleteItem) != 0) {
      if (!MIDI_CountEvts(take, 0, 0, 0)) {
             
        if (CountTakes(item) == 1) {
          DeleteTrackMediaItem(track, item);
          UpdateArrange();
        }
      }
    }
    else UpdateItemInProject(item);
    if (bUndo)  Undo_OnStateChange2(0, "Sower: Delete note");
    return true;
  }
  return false;
 }


