#include "MenuModel.h"

void MenuModel::reset(int count, int visible_rows) {
  _count = count;
  _visible = visible_rows < 1 ? 1 : visible_rows;
  _sel = 0;
  _top = 0;
}

void MenuModel::reclamp() {
  if (_count <= 0) { _sel = 0; _top = 0; return; }
  if (_sel < _top) _top = _sel;
  if (_sel >= _top + _visible) _top = _sel - _visible + 1;
  if (_top < 0) _top = 0;
}

void MenuModel::next() {
  if (_count <= 0) return;
  _sel = (_sel + 1) % _count;
  reclamp();
}

void MenuModel::prev() {
  if (_count <= 0) return;
  _sel = (_sel - 1 + _count) % _count;
  reclamp();
}
