#pragma once

class MenuModel {
  int _count;
  int _visible;
  int _sel;
  int _top;
  void reclamp();
public:
  MenuModel() : _count(0), _visible(1), _sel(0), _top(0) {}
  void reset(int count, int visible_rows);
  void prev();
  void next();
  int selected() const { return _sel; }
  int top() const { return _top; }
  int count() const { return _count; }
};
