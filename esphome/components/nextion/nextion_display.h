#pragma once

#include "esphome/components/display/display_buffer.h"
#include "nextion.h"

namespace esphome {
    namespace nextion {
        class Nextion;
    }
}

class NextionDisplay : public esphome::nextion::Nextion, public display::DisplayBuffer {
 public:
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }
  void draw_pixel_at(int x, int y, Color color) override { Nextion::fill_area(x, y, 1, 1, color); }

 protected:
  int get_width_internal() override;
  int get_height_internal() override;
}; // class NextionDisplay
