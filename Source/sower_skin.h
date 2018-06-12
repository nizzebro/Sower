#ifndef SOWER_SKIN_H_INCLUDED
#define SOWER_SKIN_H_INCLUDED

#include "sower.h"
#include "BinaryData.h"
namespace ImageIndex = BinaryData::Items;
namespace sower {
using namespace juce;
using namespace ImageIndex;
  
class Skin : public LookAndFeel_V1 {
  
public:
  Skin();

  enum {
    kSmallTextHeight = 13,
    kLargeTextHeight = 15,
    kControlHeight = kSmallTextHeight + 6
  };

  

protected:
  void loadSkinFile(StringRef skinName); 
  void loadDefaultSkin();
  Image images[BinaryData::numFiles];
  Colour backColor;
  Colour textColor;
  Colour controlColor; // fontColor.interpolatedWith(backColor, 0.3f);
  Font  fontBig;
  Font fontSmall;
private:
  virtual Font getTextButtonFont(TextButton&, int buttonHeight) override;
  virtual juce::Rectangle<int> getTooltipBounds(const String & tipText, 
    Point<int> screenPos, juce::Rectangle<int> parentArea) override;
  virtual void drawTooltip(Graphics & g, const String & text, int width, int height) override;
  virtual Font getPopupMenuFont() override;
  virtual void drawPopupMenuBackground(Graphics& g, int width, int height) override;
  virtual Font getComboBoxFont(ComboBox&) override;
  virtual void drawComboBox(Graphics & g, int width, int height,
     bool isButtonDown, int buttonX, int buttonY, 
    int buttonW, int buttonH, ComboBox & box) override;
  virtual void drawButtonBackground(Graphics & g, Button & button,
    const Colour & backgroundColour, bool isMouseOverButton, bool isButtonDown) override;
  void drawButtonText(Graphics & g, TextButton & button, bool isMouseOverButton, bool isButtonDown);
  virtual void drawTickBox(Graphics& g, juce::Component& component,
    float x, float y, float w, float h,
    const bool ticked,const bool isEnabled,const bool isMouseOverButton,const bool isButtonDown) override;
  virtual void drawToggleButton(Graphics& g, ToggleButton& button,
    bool isMouseOverButton, bool isButtonDown) override;
  virtual void changeToggleButtonWidthToFitText(ToggleButton& button) override;
  void drawLabel(Graphics & g, Label & label) override;
  void fillTextEditorBackground(Graphics & g, int, int, TextEditor & textEditor)override;
  void drawTextEditorOutline(Graphics & g, int width, int height,
    TextEditor & textEditor)override;
  void drawPopupMenuItem(Graphics&, const juce::Rectangle<int>& area,
    bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
    const String& text, const String& shortcutKeyText,
    const Drawable* icon, const Colour* textColour)override;
};

}

#endif // #ifndef SOWER_SKIN_H_INCLUDED