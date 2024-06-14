/*-------------------------------------------------------------------------
NeoMethods includes all the classes that describe pulse/data sending methods using
bitbang, SPI, or other platform specific hardware peripherl support.  

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/NeoPixelBus)

-------------------------------------------------------------------------
This file is part of the Makuna/NeoPixelBus library.

NeoPixelBus is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixelBus is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/
#pragma once

// Generic Two Wire (clk and data) methods
//
#include "DotStarGenericMethod.h"
#include "Lpd8806GenericMethod.h"
#include "Lpd6803GenericMethod.h"
#include "Ws2801GenericMethod.h"
#include "P9813GenericMethod.h"
#include "Tlc5947GenericMethod.h"
#include "Tlc59711GenericMethod.h"
#include "Sm16716GenericMethod.h"
#include "Mbi6033GenericMethod.h"
#include "Hd108GenericMethod.h"

//Adafruit Pixie via UART, not platform specific
//
#include "PixieStreamMethod.h"

// Platform specific and One Wire (data) methods
//
#if defined(ARDUINO_ARCH_ESP8266)

#include "NeoEsp8266DmaMethod.h"
#include "NeoEsp8266I2sDmx512Method.h"
#include "NeoEsp8266UartMethod.h"
#include "NeoEspBitBangMethod.h"

#elif defined(ARDUINO_ARCH_ESP32)

#if !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32H2)
#include "NeoEsp32I2sMethod.h"
#include "NeoEsp32RmtMethod.h"
#include "DotStarEsp32DmaSpiMethod.h"
#include "NeoEsp32I2sXMethod.h"


#endif
#include "NeoEspBitBangMethod.h"

#elif defined(ARDUINO_ARCH_NRF52840) // must be before __arm__

#include "NeoNrf52xMethod.h"

#elif defined(ARDUINO_ARCH_RP2040) // must be before __arm__

#include "Rp2040/NeoRp2040x4Method.h"

#elif defined(__arm__) // must be before ARDUINO_ARCH_AVR due to Teensy incorrectly having it set

#include "NeoArmMethod.h"

#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)

#include "NeoAvrMethod.h"

#else
#error "Platform Currently Not Supported, please add an Issue at Github/Makuna/NeoPixelBus"
#endif
