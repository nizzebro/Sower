#include "sower_editor.h"
#include "reaper_plugin_functions.h"
using namespace sower;


Editor::ChannelDialog::ChannelDialog(Project::Channel* chn):
  lblTrack("Track:"),
  lblMidiChn("MIDI chn:"),
  lblPitch("Pitch:"),
  btnOk("Ok"), 
  btnCancel("Cancel"),
  channel(chn){
  setOpaque(true);
  setAlwaysOnTop(true);
  setName(chn? "Set channel" : "Add channel");

  Editor& editor = *Editor::instance;

  const int leftMargin = 8;
  const int rightMargin = 8;
  const int topMargin = 8;
  const int bottomMargin = 40;
  const int indentVert = 8;
  const int indentVertLabel = 4;
  const int indentHorz = 8;
 

  lblTrack.setTopLeftPosition(leftMargin, topMargin);
  addAndMakeVisible(lblTrack);

  cbTrack.setBounds(leftMargin, lblTrack.getBottom() + indentVertLabel, 150, kControlHeight);
  for (int i = 0; i < CountTracks(0); ++i) { cbTrack.addItem(getTrackName(i), i + 1); }
  cbTrack.setSelectedItemIndex(chn && chn->track ? getTrackIndex(chn->track) : 0);
  addAndMakeVisible(cbTrack);

  lblMidiChn.setTopLeftPosition(jmax(lblTrack.getRight(), cbTrack.getRight()) + indentHorz, topMargin);
  addAndMakeVisible(lblMidiChn);

  cbMidiChn.setBounds(lblMidiChn.getX(), cbTrack.getY(), 80, kControlHeight);
  for (int i = 1; i <= 16; ++i) { cbMidiChn.addItem(String(i), i); }
  cbMidiChn.setSelectedItemIndex(chn ? chn->midiChn : config.midiChn);
  addAndMakeVisible(cbMidiChn);

  lblPitch.setTopLeftPosition(jmax(lblMidiChn.getRight(), cbMidiChn.getRight()) + indentHorz,
    topMargin);
  addAndMakeVisible(lblPitch);

  cbPitch.setBounds(lblPitch.getX(), cbTrack.getY(), 150, kControlHeight);

  if (chn != &project.chnSingle) {
    for (int i = 127; i >= 0; --i) {
      cbPitch.addItem(chn? getNoteName(chn->track, i, chn->midiChn) : getNoteName(i), i+1);
    }
    cbPitch.setSelectedItemIndex(127 - (chn ? chn->pitch : config.pitch));

    cbTrack.addListener(this);
    cbMidiChn.addListener(this);
  }

  addAndMakeVisible(cbPitch);

  int w = chn == &project.chnSingle ? cbPitch.getX() : 
    jmax(cbPitch.getRight(), lblPitch.getRight()) + rightMargin;

  btnOk.setBounds(w - 80 * 2 - indentHorz * 2,
    cbPitch.getBottom() + indentVert * 2, 80, kControlHeight);
  btnOk.addListener(this);
  addAndMakeVisible(btnOk);

  btnCancel.setBounds(w - 80 - indentHorz, btnOk.getY(), 80, kControlHeight);
  btnCancel.addListener(this);
  addAndMakeVisible(btnCancel);

  juce::Rectangle<int> bounds(editor.getScreenX()+ 32, editor.getScreenY() + 32,
    w, btnCancel.getBottom() + bottomMargin);
  setBounds(bounds.constrainedWithin(Desktop::getInstance().getDisplays().getTotalBounds(true)));

  addToDesktop(ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasTitleBar
    | ComponentPeer::windowIsTemporary);

  setVisible(true);
  toFront(true);
  grabKeyboardFocus();
}


void sower::Editor::ChannelDialog::paint(Graphics & g) {
  g.fillAll(Editor::instance->backColor);
}

void Editor::ChannelDialog::comboBoxChanged(ComboBox * comboBoxThatHasChanged) {
  // reload track note names whenever another track and midi chn is selected
  if (comboBoxThatHasChanged == &cbPitch) return;
  int iPitch = cbPitch.getSelectedItemIndex(); // save selected pitch

  MediaTrack* track = GetTrack(0, cbTrack.getSelectedItemIndex());
  int midiChn = cbMidiChn.getSelectedItemIndex();

  for (int i = 127; i >= 0; --i) {
    cbPitch.changeItemText(i + 1, getNoteName(track, i, midiChn));
  }
  cbPitch.setSelectedItemIndex(iPitch); // restore selected index
}

void sower::Editor::ChannelDialog::close(bool ok) {
  if (ok) {
    MediaTrack* track = GetTrack(0, cbTrack.getSelectedItemIndex());
    int midiChn = cbMidiChn.getSelectedItemIndex();
    if (channel == &project.chnSingle) {
      Editor::instance->setChannelSingle(track, midiChn);
    }
    else {
      int pitch = 127 - cbPitch.getSelectedItemIndex();
      // check if there's already a channel with the same track/midiChn/pitch combination...
      Project::Channel chn = { track, midiChn, pitch };
      int iChn = project.chnsMulti.indexOf(chn);
      bool bSame = iChn != -1;
      if (bSame && channel) bSame = (project.chnsMulti.getRawDataPointer() + iChn) != channel;
      if (bSame) {
        NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "Sower", 
          "A channel with these properties is already present", this);
        return;
      }
      if (!channel) Editor::instance->addChannelMulti(track, pitch, midiChn);
      else Editor::instance->setChannelMulti(*channel, track, pitch, midiChn);
    }
  }
  exitModalState(0);
  if(Editor::instance)Editor::instance->toFront(true);
}



void sower::Editor::ChannelDialog::buttonClicked(Button* btn) {
  close(btn == &btnOk);
}

void sower::Editor::ChannelDialog::userTriedToCloseWindow() {
  close(false);
}

bool sower::Editor::ChannelDialog::keyPressed(const KeyPress& key) {
  if (key.getKeyCode() == KeyPress::escapeKey) {
    close(false); return true;
  }
  else if (key.getKeyCode() == KeyPress::returnKey) {
    close(true); return true;
  }
  //else if ()
  return false;
}

