#include "sower_editor.h"
#include "reaper_plugin_functions.h"
using namespace sower;


Editor::NoteDialog::NoteDialog(int x, int y, int idxRow, int idxQuant):
 
  slVelocity(false),
  slOffset(false),
  slLength(false),
  btnOk("Ok"),
  btnCancel("Cancel"),
  iRow(idxRow),
  iQuant(idxQuant) {

  const int leftMargin = 8;
  const int rightMargin = 8;
  const int topMargin = 8;
  const int bottomMargin = 40;
  const int indentVert = 8;
  const int indentVertLabel = 4;

  Editor& editor = *Editor::instance;
  Editor::Quant& quant = *(editor.getQuants(iRow, iQuant));

  lblVelocity.setTopLeftPosition(leftMargin, topMargin);
  lblVelocity.setText("Velocity: " + String(quant.velocity));
  addAndMakeVisible(lblVelocity);

  slVelocity.setBounds(leftMargin, lblVelocity.getBottom() + indentVertLabel, 150, kScrollbarThickness);
  slVelocity.setSingleStepSize(1.0);
  slVelocity.setRangeLimits(1.0, 128.0, NotificationType::dontSendNotification);
  slVelocity.addListener(this); // lblSwingAmountValue.setText()
  slVelocity.setCurrentRange((double)quant.velocity, 1.0, NotificationType::dontSendNotification);
  addAndMakeVisible(slVelocity);

  lblOffset.setTopLeftPosition(leftMargin, slVelocity.getBottom() + indentVert);
  lblOffset.setText("Offset: " + String(quant.ppqQuantOffset));
  addAndMakeVisible(lblOffset);

  slOffset.setBounds(leftMargin, lblOffset.getBottom() + indentVertLabel, 150, kScrollbarThickness);
  slOffset.setSingleStepSize(1.0);
  slOffset.setRangeLimits((int)(-editor.ppqQuant/2), (int)(editor.ppqQuant / 2), NotificationType::dontSendNotification);
  slOffset.addListener(this); // lblSwingAmountValue.setText()
  slOffset.setCurrentRange((double)quant.ppqQuantOffset, 1.0, NotificationType::dontSendNotification);
  addAndMakeVisible(slOffset);

  lblLength.setTopLeftPosition(leftMargin, slOffset.getBottom() + indentVert);
  lblLength.setText("Length: " + String(quant.ppqLength));
  addAndMakeVisible(lblLength);

  slLength.setBounds(leftMargin, lblLength.getBottom() + indentVertLabel, 150, kScrollbarThickness);
  slLength.setSingleStepSize(1.0);
  slLength.setRangeLimits(1.0, Editor::instance->ppqQN * 4 + 1, NotificationType::dontSendNotification);
  slLength.addListener(this); // lblSwingAmountValue.setText()
  slLength.setCurrentRange((double)quant.ppqLength, 1.0, NotificationType::dontSendNotification);
  addAndMakeVisible(slLength);


  btnOk.setBounds(leftMargin + 150 - 128, slLength.getBottom() + indentVert * 2 , 60, kControlHeight);
  btnOk.addListener(this);
  addAndMakeVisible(btnOk);

  btnCancel.setBounds(leftMargin + 150 - 60, btnOk.getY(), 60, kControlHeight);
  btnCancel.addListener(this);
  addAndMakeVisible(btnCancel);

  juce::Rectangle<int> bounds(x, y,
    leftMargin + 150 + rightMargin, btnCancel.getBottom() + bottomMargin);
  setBounds(bounds.constrainedWithin(Desktop::getInstance().getDisplays().getTotalBounds(true)));
  
  setOpaque(true);
  setAlwaysOnTop(true);
  setName("Set note");
  addToDesktop(ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasTitleBar
    | ComponentPeer::windowIsTemporary);

  setVisible(true);
  toFront(true);
}


void sower::Editor::NoteDialog::buttonClicked(Button* btn) {
  close(btn == &btnOk);
}

void sower::Editor::NoteDialog::userTriedToCloseWindow() {
  close(false);
}

bool sower::Editor::NoteDialog::keyPressed(const KeyPress& key) {
  if (key.getKeyCode() == KeyPress::escapeKey) {
    close(false); return true;
  }
  else if (key.getKeyCode() == KeyPress::returnKey) {
    close(true); return true;
  }
  //else if ()
  return false;
}

void sower::Editor::NoteDialog::paint(Graphics & g) {
  g.fillAll(Editor::instance->backColor);
}

void sower::Editor::NoteDialog::close(bool ok) {
  if (ok) {
    Editor::instance->updateNote(iRow, iQuant, (int)slVelocity.getCurrentRangeStart(), 
      (int)slOffset.getCurrentRangeStart(),(int)slLength.getCurrentRangeStart());
    if (iRow == Editor::instance->iRowActive) {
      int iCol = Editor::instance->quantToColumnFloor(iQuant);
      Editor::instance->controlPanel.repaint((iCol - project.iLeftColumnVisible) * Editor::kColumnWidth,
        kParamIndent, Editor::kColumnWidth, getHeight() - kParamIndent);
    }
  }
  exitModalState(0);
  if(Editor::instance)Editor::instance->toFront(true);
}



void sower::Editor::NoteDialog::scrollBarMoved(ScrollBar* bar,
  double newRangeStart) {
  String txt;
  Label* lbl;
  if (bar == &slVelocity) {
    lbl = &lblVelocity; txt = "Velocity: ";
  }
  else if (bar == &slOffset) {
    lbl = &lblOffset; txt = "Offset: ";
  }
  else {
    lbl = &lblLength; txt = "Length: ";
  } 
  lbl->setText(txt + String((int)newRangeStart));
}





