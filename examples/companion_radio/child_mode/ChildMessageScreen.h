#pragma once
#include <helpers/ui/UIScreen.h>
#include <helpers/child/ChildMessageStore.h>
#include <helpers/child/ChildTextWrap.h>

#define CHILD_MSG_CHARS_PER_LINE  20   // SH1106 128px / ~6px font; tune on-device
#define CHILD_MSG_READER_MAX_LINES 24
#define CHILD_MSG_READER_VIS       4   // body rows visible below the header

// ChildMode impls this so the reader can return without depending on ChildMode's type.
class ReaderHandler {
public:
  virtual void onReaderBack() = 0;
};

class ChildMessageScreen : public UIScreen {
  ReaderHandler* _owner;
  ChildMessageStore* _store;
  int _idx;
  int _scroll;
  int _nlines;
  WrapLine _lines[CHILD_MSG_READER_MAX_LINES];
public:
  ChildMessageScreen(ReaderHandler* owner)
    : _owner(owner), _store(nullptr), _idx(0), _scroll(0), _nlines(0) {}
  void open(ChildMessageStore* store, int idx);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
#ifdef CHILD_REMAP_LR_TO_UD
  bool isChildScreen() const override { return true; }
#endif
};
