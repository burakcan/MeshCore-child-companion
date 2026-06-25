#pragma once
#include <helpers/ui/UIScreen.h>
#include <helpers/ui/MenuModel.h>
#include <helpers/child/ChildQuestion.h>
#include <helpers/child/ChildTextWrap.h>

#define CHILD_Q_CHARS_PER_LINE  20   // SH1106 128px / ~6px font; tune on-device
#define CHILD_Q_MAX_QLINES       4

// ChildMode impls this: option chosen, or dismissed.
class QuestionHandler {
public:
  virtual void onQuestionSelect(int opt) = 0;
  virtual void onQuestionCancel() = 0;
};

// Word-wrapped question across the top, scrollable options below.
class ChildQuestionScreen : public UIScreen {
  QuestionHandler* _owner;
  const ChildQuestion* _q;
  const char* _from;                  // "<name> asks"
  int _answered_idx;                  // -1 = unanswered; else chosen option (locked)
  MenuModel _model;
  WrapLine _qlines[CHILD_Q_MAX_QLINES];
  int _nqlines;
  bool _need_layout;                  // reset option model on first render (after translate+wrap)
  int visibleRows() const;            // option rows that fit below the question
public:
  ChildQuestionScreen(QuestionHandler* owner)
    : _owner(owner), _q(nullptr), _from(""), _answered_idx(-1), _nqlines(0), _need_layout(false) {}
  void open(const ChildQuestion* q, int answered_idx, const char* from);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
#ifdef CHILD_REMAP_LR_TO_UD
  bool isChildScreen() const override { return true; }
#endif
};
