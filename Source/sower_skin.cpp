#include "sower_skin.h"
#include "inifileparser.h"


using namespace sower;


Skin::Skin() {
  loadSkinFile(config.theme);
}

void Skin::loadSkinFile(StringRef skinName) {
  loadDefaultSkin();  // defaults
  if ((!skinName) || (!(*skinName)))  return;

  PNGImageFormat fmt;
  Image* pImg = images;

  File dir(getPluginDataDir().getChildFile("Skins").getChildFile(skinName));
  if(!dir.exists()) return;

  for (int i = 0; i != BinaryData::numFiles; ++i) {
    FileInputStream fs(dir.getChildFile(BinaryData::getFileName(i)).withFileExtension("png"));
    if (fs.openedOk()) { *pImg = fmt.decodeImage(fs); }
    ++pImg;
  }

  // now try to read from ini
  IniFileParser ini(dir.getChildFile(skinName).withFileExtension("ini"));
  if (ini.isEmpty()) return;

  
  auto s = ini.getStringValue("backColor");
  if (!s.isEmpty()) {
    char* p = s.getCharPointer().getAddress();
    int r = tok2i(&p); int g = tok2i(&p); int b = tok2i(&p);
    backColor = Colour((uint8_t)r, (uint8_t)g, (uint8_t)b);
  }
  s = ini.getStringValue("textColor");
  if (!s.isEmpty()) {
    char* p = s.getCharPointer().getAddress();
    int r = tok2i(&p); int g = tok2i(&p); int b = tok2i(&p);
    textColor = Colour((uint8_t)r, (uint8_t)g, (uint8_t)b);
  }

  const String& fontNameDef = Font::getDefaultSansSerifFontName();
  fontBig = Font(ini.getStringValue("largeFont", fontNameDef), kLargeTextHeight, Font::bold);
  fontSmall = Font(ini.getStringValue("smallFont", fontNameDef), kSmallTextHeight, Font::plain);
  controlColor = textColor.interpolatedWith(backColor, 0.3f);

  setColour(ScrollBar::ColourIds::backgroundColourId, backColor);
  setColour(ScrollBar::ColourIds::trackColourId, backColor);
  setColour(ScrollBar::ColourIds::thumbColourId, controlColor);

  juce::Colour wc((uint8_t)96, (uint8_t)96, (uint8_t)255);
  setColour(HyperlinkButton::ColourIds::textColourId, 
    textColor);
}

void Skin::loadDefaultSkin() {
  PNGImageFormat fmt;
  Image* pImg = images;
  for (int i = 0; i != BinaryData::numFiles; ++i) {
    MemoryInputStream ms(BinaryData::getFile(i), BinaryData::getFileSize(i), false);
    *pImg = fmt.decodeImage(ms); ++pImg;
  }
  backColor = Colour(50, 44, 35);
  textColor = Colour(236, 220, 200);
  fontBig = Font(Font::getDefaultSansSerifFontName(), kLargeTextHeight, Font::bold);
  fontSmall = Font(Font::getDefaultSansSerifFontName(), kSmallTextHeight, Font::plain);
  controlColor = textColor.interpolatedWith(backColor, 0.3f);

  setColour(ScrollBar::ColourIds::backgroundColourId, backColor);
  setColour(ScrollBar::ColourIds::trackColourId, backColor);
  setColour(ScrollBar::ColourIds::thumbColourId, controlColor);
 
  setColour(HyperlinkButton::ColourIds::textColourId,
    textColor);
 

  juce::Colour wc((uint8_t)96, (uint8_t)96, (uint8_t)255);
  setColour(HyperlinkButton::ColourIds::textColourId, wc);
  
}

TextLayout layoutTooltipText(const String& text) noexcept {

  AttributedString s;
  s.setJustification(Justification::centred);
  s.append(text, Font(13.0f), Colours::black);

  TextLayout tl;
  tl.createLayoutWithBalancedLineLengths(s, 400.0);
  return tl;
}

Font sower::Skin::getTextButtonFont(TextButton &, int buttonHeight) {
  return fontSmall;
}

juce::Rectangle<int> Skin::getTooltipBounds(const String& tipText, Point<int> screenPos, juce::Rectangle<int> parentArea)
{

  const TextLayout tl(layoutTooltipText(tipText));

  const int w = (int)(tl.getWidth() + 8);
  const int h = (int)(tl.getHeight() + 8);
  if (screenPos.x >= parentArea.getRight()) screenPos.x -= w;
  if (screenPos.y >= parentArea.getBottom()) {
    screenPos.y -= h;
  }
  else screenPos.y += 24;
  return juce::Rectangle<int>(screenPos.x, screenPos.y, w, h);
}

void Skin::drawTooltip(Graphics& g, const String& text, int width, int height) {
  g.fillAll(findColour(TooltipWindow::backgroundColourId));

#if ! JUCE_MAC // The mac windows already have a non-optional 1 pix outline, so don't double it here..
  g.setColour(findColour(TooltipWindow::outlineColourId));
  g.drawRect(0, 0, width, height, 1);
#endif

  layoutTooltipText(text) .draw(g, juce::Rectangle<float>((float)width, (float)height - 2));
}

Font Skin::getPopupMenuFont() {
  return fontSmall;
}

void Skin::drawPopupMenuBackground(Graphics& g, int width, int height) {
  g.fillAll(backColor);
#if ! JUCE_MAC
  g.setColour(controlColor);
  g.drawRect(0, 0, width, height);
#endif
}

Font sower::Skin::getComboBoxFont(ComboBox &) {
  return getPopupMenuFont();
}





void sower::Skin::drawComboBox(Graphics& g, int width, int height,
  const bool isButtonDown,
  int buttonX, int buttonY, int buttonW, int buttonH,
  ComboBox& box) {

  g.setColour(controlColor.withMultipliedAlpha(box.isEnabled()? 1.0f :0.4f));
  g.drawRect(0, 0, width, height);

  if (isButtonDown || (!box.isEnabled())) return;
  
#ifdef WIN32  
  const float arrowX = 0.2f;
  const float arrowH = 0.3f;
  Path p;

  p.addTriangle(buttonX + buttonW * 0.5f, buttonY + buttonH * (0.5f + arrowH / 2),
    buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * (0.5f - arrowH / 2),
    buttonX + buttonW * arrowX, buttonY + buttonH * (0.5f - arrowH / 2));

 // g.setColour(box.findColour((isButtonDown) ? ComboBox::backgroundColourId
   // : ComboBox::buttonColourId));
  g.fillPath(p);
#else
  const float arrowX = 0.2f;
  const float arrowH = 0.3f;
  Path p;
  p.addTriangle(buttonX + buttonW * 0.5f, buttonY + buttonH * (0.45f - arrowH),
    buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.45f,
    buttonX + buttonW * arrowX, buttonY + buttonH * 0.45f);

  p.addTriangle(buttonX + buttonW * 0.5f, buttonY + buttonH * (0.55f + arrowH),
    buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.55f,
    buttonX + buttonW * arrowX, buttonY + buttonH * 0.55f);

 // g.setColour(box.findColour((isButtonDown) ? ComboBox::backgroundColourId
   // : ComboBox::buttonColourId));
  g.fillPath(p);
#endif
  if (box.hasKeyboardFocus(false)) {
    g.drawRect(1, 1, buttonX - 2, height - 2);
  }
}

void sower::Skin::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
  bool isMouseOverButton, bool isButtonDown) {
  Path p;
  p.addRoundedRectangle(0.0, 0.0, (float)button.getWidth(), (float)button.getHeight(), 4.0);

  if (button.isEnabled()) {
    Colour bc = backColor.interpolatedWith(textColor, 0.1f);

    if (isButtonDown) bc = bc.darker();
    else if(isMouseOverButton) bc = bc.brighter();
    g.setColour(bc);
    g.fillPath(p);
  }

  g.setColour(controlColor.withMultipliedAlpha(button.isEnabled()? 1.0f : 0.4f));
  g.strokePath(p, PathStrokeType(button.hasKeyboardFocus(false)? 3.0f :1.0f));

}

void sower::Skin::drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) {
  g.setFont(fontSmall);
  g.setColour(textColor .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.4f));

  const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
  const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

  const int fontHeight = roundToInt(fontSmall.getHeight() * 0.6f);
  const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
  const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
  const int textWidth = button.getWidth() - leftIndent - rightIndent;

  if (textWidth > 0)
    g.drawFittedText(button.getButtonText(),
      leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2,
      Justification::centred, 2);
}


void sower::Skin::drawTickBox(Graphics& g, Component& component,
  float x, float y, float w, float h,
  const bool ticked,
  const bool isEnabled,
  const bool isMouseOverButton,
  const bool isButtonDown) {

  auto color = textColor;
  if ((!isEnabled) || isButtonDown) color = color.withMultipliedAlpha(0.4f);

  juce::Rectangle<float> rc(x, y, w, h);

  g.setColour(color);
  g.drawRect(rc);

  if (ticked) {
    rc.reduce(2.0, 2.0);
    const Path tick(getTickShape(1.0f));
    g.fillPath(tick, tick.getTransformToScaleToFit(rc, true));
  }

  
}

void sower::Skin::drawToggleButton(Graphics& g, ToggleButton& button,
  bool isMouseOverButton, bool isButtonDown) {

  

  drawTickBox(g, button, 3.0f, 3.0f,
    (float)kSmallTextHeight, (float)kSmallTextHeight,
    button.getToggleState(),
    button.isEnabled(),
    isMouseOverButton,
    isButtonDown);

  float alpha = button.isEnabled() ? 1.0f : 0.4f;
  g.setColour(textColor.withMultipliedAlpha(alpha));
  g.setFont(fontSmall);


  g.drawText(button.getButtonText(), kSmallTextHeight  + 6, 0, button.getWidth(), button.getHeight(),
    Justification::centredLeft);

  if (button.hasKeyboardFocus(false)) {
    g.setColour(controlColor.withMultipliedAlpha(0.6f));
    g.drawRect(button.getLocalBounds());
  }
}

void sower::Skin::changeToggleButtonWidthToFitText(ToggleButton& button)
{
  button.setSize(kSmallTextHeight + 12 + fontSmall.getStringWidth(button.getButtonText()),
    button.getHeight());
}

void sower::Skin::drawLabel(Graphics& g, Label& label) {
  
  float alpha = label.isEnabled() ? 1.0f : 0.4f;

  g.setColour(textColor.withMultipliedAlpha(alpha));
  g.setFont(fontSmall);

  juce::Rectangle<int> textArea(label.getBorderSize().subtractedFrom(label.getLocalBounds()));

  g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
    jmax(1, (int)(textArea.getHeight() / fontSmall.getHeight())),
    label.getMinimumHorizontalScale());

  //g.setColour(controlColor.withMultipliedAlpha(alpha));
  //g.drawRect(label.getLocalBounds());
}

void sower::Skin::fillTextEditorBackground(Graphics& g, int /*width*/, int /*height*/, TextEditor& textEditor)
{
  //g.fillAll(fontColor);
}

void sower::Skin::drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor) {
   
  g.setColour(controlColor.withMultipliedAlpha(textEditor.isEnabled()? 1.0f : 0.4f));
  int border = (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly()) ? 2 : 1;
  g.drawRect(0, 0, width, height, border);

}

void sower::Skin::drawPopupMenuItem(Graphics& g, const juce::Rectangle<int>& area,
  const bool isSeparator, const bool isActive,
  const bool isHighlighted, const bool isTicked,
  const bool hasSubMenu, const String& text,
  const String& shortcutKeyText,
  const Drawable* icon, const Colour* const textColourToUse) {
  if (isSeparator)
  {
    juce::Rectangle<int> r(area.reduced(5, 0));
    r.removeFromTop(r.getHeight() / 2 - 1);

    g.setColour(Colour(0x33000000));
    g.fillRect(r.removeFromTop(1));

    g.setColour(Colour(0x66ffffff));
    g.fillRect(r.removeFromTop(1));
  }
  else
  {
    Colour textColour(this->textColor);

    if (textColourToUse != nullptr)
      textColour = *textColourToUse;

    juce::Rectangle<int> r(area.reduced(1));

    if (isHighlighted)
    {
      g.setColour(findColour(PopupMenu::highlightedBackgroundColourId));
      g.fillRect(r);

      g.setColour(findColour(PopupMenu::highlightedTextColourId));
    }
    else
    {
      g.setColour(textColour);
    }

    if (!isActive)
      g.setOpacity(0.3f);

    Font font(getPopupMenuFont());

    const float maxFontHeight = area.getHeight() / 1.3f;

    if (font.getHeight() > maxFontHeight)
      font.setHeight(maxFontHeight);

    g.setFont(font);

    juce::Rectangle<float> iconArea(r.removeFromLeft((r.getHeight() * 5) / 4).reduced(3).toFloat());

    if (icon != nullptr)
    {
      icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
    }
    else if (isTicked)
    {
      const Path tick(getTickShape(1.0f));
      g.fillPath(tick, tick.getTransformToScaleToFit(iconArea, true));
    }

    if (hasSubMenu)
    {
      float arrowH = 0.6f * getPopupMenuFont().getAscent();

      float x = (float)r.removeFromRight((int)arrowH).getX();
      float halfH = (float)r.getCentreY();

      Path p;
      p.addTriangle(x, halfH - arrowH * 0.5f,
        x, halfH + arrowH * 0.5f,
        x + arrowH * 0.6f, halfH);

      g.fillPath(p);
    }

    r.removeFromRight(3);
    g.drawFittedText(text, r, Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty())
    {
      Font f2(font);
      f2.setHeight(f2.getHeight() * 0.75f);
      f2.setHorizontalScale(0.95f);
      g.setFont(f2);

      g.drawText(shortcutKeyText, r, Justification::centredRight, true);
    }
  }
}