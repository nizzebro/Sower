#include "sower_editor.h"
#include "reaper_plugin_functions.h"
using namespace sower;


Editor::OptionsDialog::OptionsDialog():
  
  lblVersion(SOWER_APP_STRING), btnWWW("http://nizzei8q.bget.ru/sower", URL("http://nizzei8q.bget.ru/sower")),

  lblMaxResolution("Max. grid resolution"),
  lblMidiChn("Default MIDI chn"),
  lblPitch("Default pitch"),
  tbAutoDeleteItem("Auto-delete MIDI item after the last note deleted"),
  lblAllowUndo("Allow undo points for:"),
  tbAllowUndoCutPaste("Cut/Paste"), 
  tbAllowUndoInsertDelete("Insert/delete"), 
  tbAllowUndoSetNote("Set note properties"),
  lblPreview("Preview sound on click on:"),
  tbPreviewHeader("Channel label"), 
  tbPreviewCells("Grid cells"), 
  tbAnimatePlayback("Animate playback"), 
  tbShowTooltips("Show tooltips"),
  lblSkin("Skin"),
  btnApplySkin("Apply"),
  btnOk("Ok"), 
  btnCancel("Cancel") {


  const int leftMargin = 8;
  const int rightMargin = 8;
  const int topMargin = 8;
  const int bottomMargin = 40;
  const int indentVert = 8;
  const int indentVertLabel = 4;
  const int indentHorz = 8;
  

  lblVersion.setTopLeftPosition(leftMargin, topMargin);
  addAndMakeVisible(lblVersion);

  
  btnWWW.setBounds(lblVersion.getRight() + indentHorz, topMargin, 0, kControlHeight);
  btnWWW.changeWidthToFitText();
  addAndMakeVisible(btnWWW);

  lblMaxResolution.setTopLeftPosition(leftMargin, btnWWW.getBottom() + indentVert);
  addAndMakeVisible(lblMaxResolution);

  cbMaxResolution.setBounds(leftMargin, lblMaxResolution.getBottom() + indentVertLabel, 100, kControlHeight);
  for (int i = 2; i <= 5; ++i) { cbMaxResolution.addItem(String("1/") + String(4 << i), i + 1); }
  cbMaxResolution.setSelectedItemIndex(config.maxResolution - 2);
  addAndMakeVisible(cbMaxResolution);

  lblMidiChn.setTopLeftPosition(jmax(cbMaxResolution.getRight(), lblMaxResolution.getRight())
    + indentHorz, lblMaxResolution.getY());
  addAndMakeVisible(lblMidiChn);

  cbMidiChn.setBounds(lblMidiChn.getX(), cbMaxResolution.getY(), 80, kControlHeight);
  for (int i = 1; i <= 16; ++i) { cbMidiChn.addItem(String(i), i); }
  cbMidiChn.setSelectedItemIndex(config.midiChn);
  addAndMakeVisible(cbMidiChn);

  lblPitch.setTopLeftPosition(jmax(lblMidiChn.getRight(), cbMidiChn.getRight()) + indentHorz, lblMidiChn.getY());
  addAndMakeVisible(lblPitch);

  cbPitch.setBounds(lblPitch.getX(), cbMidiChn.getY(), 80, kControlHeight);
  for (int i = 127; i >= 0; --i) { cbPitch.addItem(getNoteName(i), i + 1); }
  cbPitch.setSelectedItemIndex(127 - config.pitch);
  addAndMakeVisible(cbPitch);

  tbAutoDeleteItem.setBounds(leftMargin, cbPitch.getBottom() + indentVert, 0, kControlHeight);
  tbAutoDeleteItem.changeWidthToFitText();
  tbAutoDeleteItem.setToggleState((config.flags & config.kAutoDeleteItem) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbAutoDeleteItem);

  lblAllowUndo.setTopLeftPosition(leftMargin, tbAutoDeleteItem.getBottom() + indentVert);
  addAndMakeVisible(lblAllowUndo);

  tbAllowUndoCutPaste.setBounds(leftMargin, lblAllowUndo.getBottom() + indentVert, 0, kControlHeight);
  tbAllowUndoCutPaste.changeWidthToFitText();
  tbAllowUndoCutPaste.setToggleState((config.flags & config.kUndoCutPaste) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbAllowUndoCutPaste);

  tbAllowUndoInsertDelete.setBounds(tbAllowUndoCutPaste.getRight() + indentHorz,
    tbAllowUndoCutPaste.getY(), 0, kControlHeight);
  tbAllowUndoInsertDelete.changeWidthToFitText();
  tbAllowUndoInsertDelete.setToggleState((config.flags & config.kUndoInsertDelete) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbAllowUndoInsertDelete);

  tbAllowUndoSetNote.setBounds(tbAllowUndoInsertDelete.getRight() + indentHorz,
    tbAllowUndoCutPaste.getY(), 0, kControlHeight);
  tbAllowUndoSetNote.changeWidthToFitText();
  tbAllowUndoSetNote.setToggleState((config.flags & config.kUndoSetNote) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbAllowUndoSetNote);

  lblPreview.setTopLeftPosition(leftMargin, tbAllowUndoCutPaste.getBottom() + indentVert);
  addAndMakeVisible(lblPreview);
  tbPreviewHeader.setBounds(leftMargin, lblPreview.getBottom() + indentVert, 0, kControlHeight);
  tbPreviewHeader.changeWidthToFitText();
  tbPreviewHeader.setToggleState((config.flags & config.kPreviewHeader) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbPreviewHeader);

  tbPreviewCells.setBounds(tbPreviewHeader.getRight() + indentHorz,
    tbPreviewHeader.getY(), 0, kControlHeight);
  tbPreviewCells.changeWidthToFitText();
  tbPreviewCells.setToggleState((config.flags & config.kPreviewCell) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbPreviewCells);

  tbAnimatePlayback.setBounds(leftMargin, tbPreviewCells.getBottom() + indentVert, 0, kControlHeight);
  tbAnimatePlayback.changeWidthToFitText();
  tbAnimatePlayback.setToggleState((config.flags & config.kAnimatePlayback) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbAnimatePlayback);

  tbShowTooltips.setBounds(tbAnimatePlayback.getRight() + indentHorz,
    tbAnimatePlayback.getY(), 0, kControlHeight);
  tbShowTooltips.changeWidthToFitText();
  tbShowTooltips.setToggleState((config.flags & config.kShowTooltips) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbShowTooltips);

  lblSkin.setTopLeftPosition(leftMargin, tbShowTooltips.getBottom() + indentVert);
  addAndMakeVisible(lblSkin);

  cbSkin.setBounds(leftMargin, lblSkin.getBottom() + indentVertLabel, 250, kControlHeight);

  cbSkin.addItem("Default", 1);
  cbSkin.setSelectedItemIndex(0);
  iSkinApplied = 0;


  File dir = getPluginDataDir().getChildFile("Skins");

  if (dir.exists()) {
    DirectoryIterator iter(dir, false, "*", File::findDirectories);
    for (int i = 1; iter.next(); ++i) {
      auto s = iter.getFile().getFileName();
      cbSkin.addItem(s, i + 1);
      if (s == config.theme) {
        cbSkin.setSelectedItemIndex(i);
        iSkinApplied = i;
      }
    }
  }
  addAndMakeVisible(cbSkin);

  btnApplySkin.setBounds(cbSkin.getRight() + indentHorz, cbSkin.getY(), 80, kControlHeight);
  btnApplySkin.addListener(this);
  addAndMakeVisible(btnApplySkin);

  int right = 0;
  for (auto comp : getChildren()) { right = jmax(comp->getRight(), right); }

  btnOk.setBounds(right / 2 - indentHorz / 2 - 80, cbSkin.getBottom() + indentVert * 2, 80, kControlHeight);
  btnOk.addListener(this);
  addAndMakeVisible(btnOk);

  btnCancel.setBounds(right/2 + indentHorz / 2, btnOk.getY(), 80, kControlHeight);
  btnCancel.addListener(this);
  addAndMakeVisible(btnCancel);

  juce::Rectangle<int> bounds(Editor::instance->getScreenX()+ 32, Editor::instance->getScreenY() + 32,
    right + rightMargin, btnCancel.getBottom() + bottomMargin);
  setBounds(bounds.constrainedWithin(Desktop::getInstance().getDisplays().getTotalBounds(true)));
  setOpaque(true);
  setAlwaysOnTop(true);
  setName("Options");
  addToDesktop(ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasTitleBar
    | ComponentPeer::windowIsTemporary);
  setVisible(true);
  cbMaxResolution.grabKeyboardFocus();
}


void sower::Editor::OptionsDialog::paint(Graphics & g) {
  g.fillAll(Editor::instance->backColor);
}

void sower::Editor::OptionsDialog::applySkin() {
  int iSelected = cbSkin.getSelectedItemIndex(); jassert(iSelected != -1); 
  if((iSelected == iSkinApplied) || (iSelected == -1)) return;
  iSkinApplied = iSelected;
  if (iSkinApplied == 0) config.theme[0] = 0;
  else cbSkin.getText().copyToUTF8(config.theme, sizeof(config.theme));
  Editor::instance->loadSkinFile(config.theme);
  Editor::instance->repaint();
  if (Editor::instance->midiProcessor)Editor::instance->midiProcessor->repaint();
  repaint();
}

void sower::Editor::OptionsDialog::close(bool ok) {
  if (ok) {
    int res = jlimit(2, 5, cbMaxResolution.getSelectedItemIndex() + 2);
    bool needUpdateBar = config.maxResolution != res;
    config.maxResolution = res;
    
    bool needTimer = tbAnimatePlayback.getToggleState() || useSyncBar();
    bool needUpdateTimer = (needTimer != Editor::instance->isTimerRunning());
    
    
    config.midiChn = jlimit( 0, 15, cbMidiChn.getSelectedItemIndex());
    config.pitch = jlimit(0, 127, 127 - cbPitch.getSelectedItemIndex());
    config.flags = (tbAutoDeleteItem.getToggleState() ? config.kAutoDeleteItem : 0)
      | (tbAllowUndoCutPaste.getToggleState() ? config.kUndoCutPaste : 0)
      | (tbAllowUndoInsertDelete.getToggleState() ? config.kUndoInsertDelete : 0)
      | (tbAllowUndoSetNote.getToggleState() ? config.kUndoSetNote : 0)
      | (tbPreviewHeader.getToggleState() ? config.kPreviewHeader : 0)
      | (tbPreviewCells.getToggleState() ? config.kPreviewCell : 0)
      | (tbAnimatePlayback.getToggleState() ? config.kAnimatePlayback : 0)
      | (tbShowTooltips.getToggleState() ? config.kShowTooltips : 0);

    
    applySkin();

    if (needUpdateTimer) {
      if (needTimer) {
        Editor::instance->iColumnAnimated = -1;
        Editor::instance->startTimer(70);
      }
      else Editor::instance->stopTimer();
    }
    if(needUpdateBar) Editor::instance->updateBar();
    Editor::instance->setTooltips();
  }
  exitModalState(0);
  if(Editor::instance)Editor::instance->grabKeyboardFocus();
}



void sower::Editor::OptionsDialog::buttonClicked(Button* btn) {
  if (btn == &btnApplySkin) applySkin();
  else if(btn == &btnOk) close(true);
  else if (btn == &btnCancel) close(false);
}

void sower::Editor::OptionsDialog::userTriedToCloseWindow() {
  close(false);
}

bool sower::Editor::OptionsDialog::keyPressed(const KeyPress& key) {
  if (key.getKeyCode() == KeyPress::escapeKey) {
    close(false); return true;
  }
  else if (key.getKeyCode() == KeyPress::returnKey) {
    close(true); return true;
  }
  //else if ()
  return false;
}

