#ifndef PX_INIFILEPARSER_H
#define PX_INIFILEPARSER_H

#include "../JuceLibraryCode/JuceHeader.h"

class IniFileParser {
  public:
    IniFileParser(): file(), lines() {}
    
    IniFileParser(const String& path): file(path), lines() {
      file.create();
      file.readLines(lines);
    }

    IniFileParser(const File& file) : file(file), lines() {
      file.create();
      file.readLines(lines);
    }

    IniFileParser(const IniFileParser& other): file(other.file), lines() {
      file.create();
      file.readLines(lines);
    }

    ~IniFileParser() {}
    
    IniFileParser& operator=(const String& path) {
      file = path;
      file.create();
      file.readLines(lines);
      return *this;
    }
  
    IniFileParser& operator=(const File& f) {
      file = f;
      file.create();
      file.readLines(lines);
      return *this;
    }

    IniFileParser& operator=(const IniFileParser& rhs) {
      file = rhs.file;
      file.create();
      file.readLines(lines);
      return *this;
    }

    bool isEmpty() const noexcept {return lines.isEmpty();}
    bool isNotEmpty() const noexcept { return !isEmpty(); }

    void write() {
      TemporaryFile tempFile(file, TemporaryFile::useHiddenFile);
      for (auto it : lines) {
        tempFile.getFile().appendText(it);
        tempFile.getFile().appendText(String("\n"));
      }
      tempFile.overwriteTargetFileWithTemporary();
    }

    String* getSection(const String& name) const noexcept {
      auto line = lines.begin();
      if (name.isEmpty()) return line;
      while (line != lines.end()) {
        auto pLine = line->getCharPointer().findEndOfWhitespace();
        ++line;
        if (pLine.getAndAdvance() == '[') {
          auto pName = name.getCharPointer();
          for (;;) {
            auto cLine = pLine.getAndAdvance();
            if (cLine == 0) break;
            auto cName = pName.getAndAdvance();
            if (cLine != cName) {
              if (cName != 0) break;
              if (cLine != ']') break;
              return line;
            }
          }
        }
      }
      return line;
    }

    String::CharPointerType getKey(const String& name, const String& section = String()) const noexcept {
      for (auto line = getSection(section); line != lines.end(); ++line) {
        auto pLine = line->getCharPointer().findEndOfWhitespace();
        if (*pLine == '[') break; // next section
        auto pName = name.getCharPointer().findEndOfWhitespace();
        for (;;) {
          auto cLine = pLine.getAndAdvance();
          if(cLine == 0) break;
          auto cName = pName.getAndAdvance();
          if (cName != cLine) {
            if (cName != 0) break;
            cLine = pLine.findEndOfWhitespace().getAndAdvance();
            if (cLine != '=') break;
            ++pLine;
            return pLine.findEndOfWhitespace();
          }
        }
      }
      return String::CharPointerType(0);
    }


    String getStringValue(const String& name, const String& defValue = String(), 
      const String& section = String()) const noexcept {
      auto p = getKey(name, section);
      if(!p) return defValue;
      if (p.isEmpty()) return defValue;
      return String(p);
    }

    int getIntValue(const String& name, int defValue = 0,
      const String& section = String()) const noexcept {
      auto p = getKey(name, section);
      if (!p) return defValue;
      if (p.isEmpty()) return defValue;
      return p.getIntValue32();
    }

    int64 getLargeIntValue(const String& name, int64 defValue = 0,
      const String& section = String()) const noexcept {
      auto p = getKey(name, section);
      if (!p) return defValue;
      if (p.isEmpty()) return defValue;
      return p.getIntValue64();
    }

    float getFloatValue(const String& name, float defValue = 0.0,
      const String& section = String()) const noexcept { 
      auto p = getKey(name, section);
      if (!p) return defValue;
      if (p.isEmpty()) return defValue;
      return (float)p.getDoubleValue();
    }

    double getDoubleValue(const String& name, double defValue = 0.0,
      const String& section = String()) const noexcept {
      auto p = getKey(name, section);
      if (!p) return defValue;
      if (p.isEmpty()) return defValue;
      return  p.getDoubleValue();
    }

    void setValue(const String& keyName, String::CharPointerType value, 
      const String& section = String()) {
      String keyLine(keyName); keyLine += '='; keyLine += value.getAddress();
      auto line = getSection(section);
      if ((line == lines.end()) && (section.isNotEmpty())) {
        String secLine('['); secLine += section; secLine += ']';
        lines.add(secLine);
      }
      for (; line != lines.end(); ++line) {
        auto pLine = line->getCharPointer().findEndOfWhitespace();
        auto cLine = pLine.getAndAdvance();
        if (cLine == '[') break; // next section
        auto pName = keyName.getCharPointer();
        for (;;) {
          auto cName = pName.getAndAdvance();
          if (cLine != cName) {
            if (!cLine) break; if (cName) break;
            cLine = pLine.findEndOfWhitespace().getAndAdvance();
            if (cLine == '=') {
              *line = keyLine; return;
            }
            break;
          }
          cLine = pLine.getAndAdvance();
        }
      }
      lines.add(keyLine);
    }

    void setValue(const String& keyName, const String& value,
      const String& section = String()) {
      setValue(keyName, value.getCharPointer(), section);
    }

    void setValue(const String& keyName, int value,
      const String& section = String()) {
      setValue(keyName, String(value).getCharPointer(), section);
    }

    void setValue(const String& keyName, int64 value,
      const String& section = String()) {
      setValue(keyName, String(value).getCharPointer(), section);
    }

    void setValue(const String& keyName, float value,
      const String& section = String()) {
      setValue(keyName, String(value).getCharPointer(), section);
    }

    void setValue(const String& keyName, double value,
      const String& section = String()) {
      setValue(keyName, String(value).getCharPointer(), section);
    }

  protected:
    File file;
    StringArray lines;        
};

 

#endif // #ifndef PX_INIFILEPARSER_H