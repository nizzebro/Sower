#include "sower_editor.h"
#include "reaper_plugin_functions.h"
using namespace sower;


Editor::MIDIProcDialog::MIDIProcDialog():
  tbQuantizePos("Quantize note position"),
  lblQuantizePosWindow("Window"),
  slQuantizePosWindow(false),
  tbQuantizeDelete("Delete extra notes"),
  tbSwing("Swing"),
  slSwingAmount(false),
  
  tbQuantizeLength("Align note length to"),
  slQuantizeLengthValue(false),

  tbQuantizeVelocity("Align velocity to"),
  slQuantizeVelocityValue(false),
  
  lblQuantizeAmount("Quantizing strength"),
  slQuantizeAmount(false),
 
  lblRandomize("Randomize"),
  tbRandomizePos("Start time"),
  tbRandomizeLength("Length"),
  tbRandomizeVelocity("Velocity"),
  lblRandomizeAmount("Randomizing strength"),
  slRandomizeAmount(false),

  lblExclude("Exclude"),
  tbExcludeFirstBeat("1st beat"),
  tbExcludeOnBeats("On-beats"),
  tbExcludeOffBeats("Off-beats"),

  tbSelectBars("Process measures selected in arrange view"),

  btnApply("Apply"),
  btnClose("Close") {
  
  //setAlwaysOnTop(true);
  setName("MIDI processor");

  const int leftMargin = 8;
  const int rightMargin = 8;
  const int topMargin = 8;
  const int bottomMargin = 40;
  const int indentVert = 8;
  const int indentHorz = 8;
  const int marginScrollbar = (kControlHeight - kScrollbarThickness) / 2;
  MidiProcessorConfig& cfg = config.midiProcessor;
  // "Note position" checkbox
  tbQuantizePos.setBounds(leftMargin, topMargin, 0, kControlHeight);
  tbQuantizePos.changeWidthToFitText();
  tbQuantizePos.addListener(this); // to switch group controls enablement
  tbQuantizePos.setToggleState((cfg.flags & cfg.kQuantizePosition) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbQuantizePos);
  // Quantize pos resolution combobox
  cbQuantizePosResolution.setBounds(tbQuantizePos.getRight() + indentHorz, tbQuantizePos.getY(), 80, kControlHeight);
  for (int i = 0; i <= config.maxResolution; ++i) {
    cbQuantizePosResolution.addItem(String("1/") + String(4 << i), i + 1);
  }

  cbQuantizePosResolution.setSelectedItemIndex(cfg.quantizePosResolution, NotificationType::dontSendNotification);
  cbQuantizePosResolution.setEnabled(tbQuantizePos.getToggleState());
  addAndMakeVisible(cbQuantizePosResolution);
  // "Window:" label
  lblQuantizePosWindow.setTopLeftPosition(cbQuantizePosResolution.getRight() + indentHorz, tbQuantizePos.getY());
  lblQuantizePosWindow.setEnabled(tbQuantizePos.getToggleState());
  addAndMakeVisible(lblQuantizePosWindow);
  // Quantize window slider
  slQuantizePosWindow.setBounds(lblQuantizePosWindow.getRight() + indentHorz,
    lblQuantizePosWindow.getY() + marginScrollbar, 100, kScrollbarThickness);
  slQuantizePosWindow.setSingleStepSize(0.1);
  slQuantizePosWindow.setRangeLimits(0.01, 1.01, NotificationType::dontSendNotification);
  slQuantizePosWindow.setCurrentRange(cfg.quantizePosWindow, 0.1, NotificationType::dontSendNotification);
  slQuantizePosWindow.setEnabled(tbQuantizePos.getToggleState());
  addAndMakeVisible(slQuantizePosWindow);
  // "Swing" checkbox
  tbSwing.setBounds(leftMargin + indentHorz * 2, tbQuantizePos.getBottom() + indentVert, 0, kControlHeight);
  tbSwing.changeWidthToFitText();
  tbSwing.setToggleState((cfg.flags & cfg.kSwing) != 0, NotificationType::dontSendNotification);
  tbSwing.addListener(this);  // slider enablement
  tbSwing.setEnabled(tbQuantizePos.getToggleState());
  addAndMakeVisible(tbSwing);
  // Swing amt slider
  slSwingAmount.setBounds(tbSwing.getRight() + indentHorz, tbSwing.getY() + marginScrollbar,
    100, kScrollbarThickness);
  slSwingAmount.setSingleStepSize(0.1);
  slSwingAmount.setRangeLimits(0.01, 1.01, NotificationType::dontSendNotification);
  slSwingAmount.setCurrentRange(cfg.amtSwing, 0.1, NotificationType::dontSendNotification);
  slSwingAmount.setEnabled(tbSwing.isEnabled() && tbSwing.getToggleState());
  addAndMakeVisible(slSwingAmount);
  // "Delete extra notes" checkbox
  tbQuantizeDelete.setBounds(slSwingAmount.getRight() + indentHorz, tbSwing.getY(), 0, kControlHeight);
  tbQuantizeDelete.changeWidthToFitText();
  tbQuantizeDelete.setToggleState((cfg.flags & cfg.kDeleteExtraNotes) != 0, NotificationType::dontSendNotification);
  tbQuantizeDelete.setEnabled(tbQuantizePos.getToggleState());
  addAndMakeVisible(tbQuantizeDelete);
  // "Quantize length" checkbox
  tbQuantizeLength.setBounds(leftMargin, tbSwing.getBottom() + indentVert, 0, kControlHeight);
  tbQuantizeLength.changeWidthToFitText();
  tbQuantizeLength.setToggleState((cfg.flags & cfg.kQuantizeLength) != 0, NotificationType::dontSendNotification);
  tbQuantizeLength.addListener(this);  // slider/value label enablement
  addAndMakeVisible(tbQuantizeLength);
  // Quantize length slider
  slQuantizeLengthValue.setBounds(tbQuantizeLength.getRight() + indentHorz,
    tbQuantizeLength.getY() + marginScrollbar, 200, kScrollbarThickness);
  slQuantizeLengthValue.setSingleStepSize(1.0);
  slQuantizeLengthValue.setRangeLimits(1.0, Editor::instance->ppqQN + 1, NotificationType::dontSendNotification);
  slQuantizeLengthValue.addListener(this); // value label update
  slQuantizeLengthValue.setCurrentRange((double)cfg.quantizeLengthValue, 1.0, NotificationType::dontSendNotification);
  slQuantizeLengthValue.setEnabled(tbQuantizeLength.getToggleState());
  addAndMakeVisible(slQuantizeLengthValue);
  // Quantize length value label
  lblQuantizeLengthValue.setBounds(slQuantizeLengthValue.getRight() + 8,
    slQuantizeLengthValue.getY(), kSmallTextHeight * 8, kControlHeight);
  lblQuantizeLengthValue.setText(String((int)slQuantizeLengthValue.getCurrentRangeStart()));
  lblQuantizeLengthValue.setEnabled(tbQuantizeLength.getToggleState());
  addAndMakeVisible(lblQuantizeLengthValue);
  // "Quantize velocity" checkbox
  tbQuantizeVelocity.setBounds(leftMargin, tbQuantizeLength.getBottom() + indentVert, 0, kControlHeight);
  tbQuantizeVelocity.changeWidthToFitText();
  tbQuantizeVelocity.setToggleState((cfg.flags & cfg.kQuantizeVelocity) != 0, NotificationType::dontSendNotification);
  tbQuantizeVelocity.addListener(this);  /// slider/value label enablement
  addAndMakeVisible(tbQuantizeVelocity);
  // Quantize velocity slider
  slQuantizeVelocityValue.setBounds(tbQuantizeVelocity.getRight() + indentHorz,
    tbQuantizeVelocity.getY() + marginScrollbar, 200, kScrollbarThickness);
  slQuantizeVelocityValue.setSingleStepSize(1.0);
  slQuantizeVelocityValue.setRangeLimits(1.0, 128.0, NotificationType::dontSendNotification);
  slQuantizeVelocityValue.addListener(this); // value label update
  slQuantizeVelocityValue.setCurrentRange((double)cfg.quantizeVelocityValue, 1.0, NotificationType::dontSendNotification);
  lblQuantizeLengthValue.setEnabled(tbQuantizeVelocity.getToggleState());
  addAndMakeVisible(slQuantizeVelocityValue);
  // Quantize velocity value label
  lblQuantizeVelocityValue.setBounds(slQuantizeVelocityValue.getRight() + 8,
    tbQuantizeVelocity.getY(), kSmallTextHeight * 4, kControlHeight);
  lblQuantizeVelocityValue.setText(String((int)slQuantizeVelocityValue.getCurrentRangeStart()));
  lblQuantizeLengthValue.setEnabled(tbQuantizeVelocity.getToggleState());
  addAndMakeVisible(lblQuantizeVelocityValue);
  // Quantize amt label
  lblQuantizeAmount.setTopLeftPosition(leftMargin + indentHorz, tbQuantizeVelocity.getBottom() + indentVert);
  addAndMakeVisible(lblQuantizeAmount);
  // Quantize amt slider
  slQuantizeAmount.setBounds(lblQuantizeAmount.getRight() + indentHorz,
    lblQuantizeAmount.getY() + marginScrollbar, 150, kScrollbarThickness);
  slQuantizeAmount.setSingleStepSize(0.01);
  slQuantizeAmount.setRangeLimits(0.01, 1.001, NotificationType::dontSendNotification);
  slQuantizeAmount.setCurrentRange((double)cfg.amtQuantize, 0.01, NotificationType::dontSendNotification);
  addAndMakeVisible(slQuantizeAmount);
  // "Randomize" label
  lblRandomize.setTopLeftPosition(leftMargin, lblQuantizeAmount.getBottom() + indentVert);
  addAndMakeVisible(lblRandomize);
  // Randomize pos checkbox
  tbRandomizePos.setBounds(lblRandomize.getRight() + indentHorz, lblRandomize.getY(), 0, kControlHeight);
  tbRandomizePos.changeWidthToFitText();
  tbRandomizePos.setToggleState((cfg.flags & cfg.kRandomizeVelocity) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbRandomizePos);
  // Randomize length checkbox
  tbRandomizeLength.setBounds(tbRandomizePos.getRight() + indentHorz, lblRandomize.getY(),
    0, kControlHeight);
  tbRandomizeLength.changeWidthToFitText();
  tbRandomizeLength.setToggleState((cfg.flags & cfg.kRandomizeVelocity) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbRandomizeLength);
  // Randomize velocity checkbox
  tbRandomizeVelocity.setBounds(tbRandomizeLength.getRight() + indentHorz, lblRandomize.getY(),
    0, kControlHeight);
  tbRandomizeVelocity.changeWidthToFitText();
  tbRandomizeVelocity.setToggleState((cfg.flags & cfg.kRandomizeVelocity) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbRandomizeVelocity);
  // Randomize amt label
  lblRandomizeAmount.setTopLeftPosition(leftMargin + indentHorz, tbRandomizePos.getBottom() + indentVert);
  addAndMakeVisible(lblRandomizeAmount);
  // Randomize amt slider
  slRandomizeAmount.setBounds(lblRandomizeAmount.getRight() + indentHorz,
    lblRandomizeAmount.getY() + marginScrollbar, 150, kScrollbarThickness);
  slRandomizeAmount.setSingleStepSize(0.01);
  slRandomizeAmount.setRangeLimits(0.01, 1.001, NotificationType::dontSendNotification);
  slRandomizeAmount.setCurrentRange((double)cfg.amtRandomize, 0.01, NotificationType::dontSendNotification);
  addAndMakeVisible(slRandomizeAmount);
  // "Exclude" label
  lblExclude.setTopLeftPosition(leftMargin, lblRandomizeAmount.getBottom() + indentVert);
  addAndMakeVisible(lblExclude);
  // "1st beat"
  tbExcludeFirstBeat.setBounds(lblExclude.getRight() + indentHorz, lblExclude.getY(), 0, kControlHeight);
  tbExcludeFirstBeat.changeWidthToFitText();
  tbExcludeFirstBeat.setToggleState((cfg.flags & cfg.kExcludeFirstBeat) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbExcludeFirstBeat);
  // "On-beats"
  tbExcludeOnBeats.setBounds(tbExcludeFirstBeat.getRight() + indentHorz, lblExclude.getY(),
    0, kControlHeight);
  tbExcludeOnBeats.changeWidthToFitText();
  tbExcludeOnBeats.setToggleState((cfg.flags & cfg.kExcludeOnBeats) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbExcludeOnBeats);
  // "Off-beats"
  tbExcludeOffBeats.setBounds(tbExcludeOnBeats.getRight() + indentHorz, lblExclude.getY(),
    0, kControlHeight);
  tbExcludeOffBeats.changeWidthToFitText();
  tbExcludeOffBeats.setToggleState((cfg.flags & cfg.kExcludeOffBeats) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbExcludeOffBeats);
  // Exclude resolution combobox
  cbExcludeResolution.setBounds(tbExcludeOffBeats.getRight() + indentHorz, lblExclude.getY(), 80, kControlHeight);
  for (int i = 0; i <= config.maxResolution; ++i) {
    cbExcludeResolution.addItem(String("1/") + String(4 << i), i + 1);
  }
  cbExcludeResolution.setSelectedItemIndex(cfg.excludeResolution, NotificationType::dontSendNotification);
  addAndMakeVisible(cbExcludeResolution);

  tbSelectBars.setBounds(leftMargin, lblExclude.getBottom() + indentVert, 0, kControlHeight);
  tbSelectBars.changeWidthToFitText();
  tbExcludeOnBeats.setToggleState((cfg.flags & cfg.kProcessSelectedBars) != 0, NotificationType::dontSendNotification);
  addAndMakeVisible(tbSelectBars);

  int right = 0;
  for (auto comp : getChildren()) { right = jmax(comp->getRight(), right); }


  btnApply.setBounds(right / 2 - indentHorz / 2 - 80, tbSelectBars.getBottom() + indentVert * 2,
    80, kControlHeight);
  btnApply.addListener(this);
  addAndMakeVisible(btnApply);

  btnClose.setBounds(right / 2 + indentHorz / 2, btnApply.getY(), 80, kControlHeight);
  btnClose.addListener(this);
  addAndMakeVisible(btnClose);

  juce::Rectangle<int> bounds(Editor::instance->getScreenX() + 32, Editor::instance->getScreenY() + 32,
    right + rightMargin, btnClose.getBottom() + bottomMargin);
  setBounds(bounds.constrainedWithin(Desktop::getInstance().getDisplays().getTotalBounds(true)));
  
  setOpaque(true);
  setAlwaysOnTop(true);
  addToDesktop(ComponentPeer::windowHasCloseButton | ComponentPeer::windowHasTitleBar
    | ComponentPeer::windowIsTemporary,
  #if JUCE_WINDOWS
  reaper
  #else 
  0
  #endif
  );          
  setVisible(true);
  btnApply.grabKeyboardFocus();
}


sower::Editor::MIDIProcDialog::~MIDIProcDialog() {
  save();
}


void sower::Editor::MIDIProcDialog::buttonClicked(Button* btn) {
  if (btn == &btnApply) {
    save();
    Editor::instance->processRows();
  }
  else if (btn == &btnClose) userTriedToCloseWindow();
  else if (btn == &tbQuantizePos) {
    cbQuantizePosResolution.setEnabled(btn->getToggleState());
    lblQuantizePosWindow.setEnabled(btn->getToggleState());
    slQuantizePosWindow.setEnabled(btn->getToggleState());
    tbQuantizeDelete.setEnabled(btn->getToggleState());
    tbSwing.setEnabled(btn->getToggleState());
    slSwingAmount.setEnabled(btn->getToggleState() && tbSwing.getToggleState());
  }
  else if (btn == &tbSwing) {
    slSwingAmount.setEnabled(tbQuantizePos.getToggleState() && btn->getToggleState());
  }
  else if (btn == &tbQuantizeLength) {
    slQuantizeLengthValue.setEnabled(btn->getToggleState());
    lblQuantizeLengthValue.setEnabled(btn->getToggleState());
  }
  else if (btn == &tbQuantizeVelocity) {
    slQuantizeVelocityValue.setEnabled(btn->getToggleState());
    lblQuantizeVelocityValue.setEnabled(btn->getToggleState());
  }
}

void sower::Editor::MIDIProcDialog::comboBoxChanged(ComboBox* box) {

}

void sower::Editor::MIDIProcDialog::paint(Graphics & g) {
  g.fillAll(Editor::instance->backColor);
}

void sower::Editor::MIDIProcDialog::scrollBarMoved(ScrollBar* bar,
  double newRangeStart) {
  if (bar == &slQuantizeLengthValue) lblQuantizeLengthValue.setText(String((int)newRangeStart));
  else if (bar == &slQuantizeVelocityValue) lblQuantizeVelocityValue.setText(String((int)newRangeStart));
}

void sower::Editor::MIDIProcDialog::userTriedToCloseWindow() {
  delete this;
  if(Editor::instance)Editor::instance->toFront(true);
}

bool sower::Editor::MIDIProcDialog::keyPressed(const KeyPress& key) {
  if (key.getKeyCode() == KeyPress::escapeKey) {
    userTriedToCloseWindow();
  }
  return true;
}

void sower::Editor::MIDIProcDialog::save() noexcept {
  MidiProcessorConfig& cfg = config.midiProcessor;
  cfg.flags = (tbQuantizePos.getToggleState() ? cfg.kQuantizePosition : 0)
    | (tbQuantizeDelete.getToggleState() ? cfg.kDeleteExtraNotes : 0)
    | (tbQuantizeLength.getToggleState() ? cfg.kQuantizeLength : 0)
    | (tbQuantizeVelocity.getToggleState() ? cfg.kQuantizeVelocity : 0)
    | (tbRandomizePos.getToggleState() ? cfg.kRandomizePosition : 0)
    | (tbRandomizeLength.getToggleState() ? cfg.kRandomizeLength : 0)
    | (tbRandomizeVelocity.getToggleState() ? cfg.kRandomizeVelocity : 0)
    | (tbSwing.getToggleState() ? cfg.kSwing : 0)
    | (tbExcludeFirstBeat.getToggleState() ? cfg.kExcludeFirstBeat : 0)
    | (tbExcludeOnBeats.getToggleState() ? cfg.kExcludeOnBeats : 0)
    | (tbExcludeOffBeats.getToggleState() ? cfg.kExcludeOffBeats : 0)
    | (tbSelectBars.getToggleState() ? cfg.kProcessSelectedBars : 0);

  cfg.quantizePosResolution = cbQuantizePosResolution.getSelectedItemIndex();
  cfg.excludeResolution = cbExcludeResolution.getSelectedItemIndex();
  cfg.quantizeLengthValue = (int) slQuantizeLengthValue.getCurrentRangeStart();
  cfg.quantizeVelocityValue = (int) slQuantizeVelocityValue.getCurrentRangeStart();
  cfg.quantizePosWindow = slQuantizePosWindow.getCurrentRangeStart();
  cfg.amtQuantize = slQuantizeAmount.getCurrentRangeStart();
  cfg.amtSwing = slSwingAmount.getCurrentRangeStart();
  cfg.amtRandomize = slRandomizeAmount.getCurrentRangeStart();

}

