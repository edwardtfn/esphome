#pragma once

#include "esphome/components/display/display_buffer.h"


namespace esphome {
namespace nextion {
class NextionDisplay;

class NextionDisplay : public display::DisplayBuffer {
 public:
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

 protected:
  int get_width_internal() override;
  int get_height_internal() override;
}; // class NextionDisplay
}  // namespace nextion
}  // namespace esphome
