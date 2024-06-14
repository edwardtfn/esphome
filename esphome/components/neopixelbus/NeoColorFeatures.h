/*-------------------------------------------------------------------------
NeoColorFeatures includes all the feature classes that describe color order and
color depth for NeoPixelBus template class

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

// Core Element base classes
// 
#include "NeoElementsNoSettings.h"
#include "NeoByteElements.h"

// Core Feature base classes
#include "Neo2Byte555Feature.h"
#include "Neo3ByteFeature.h"
#include "Neo3Byte777Feature.h"
#include "Neo4ByteFeature.h"
#include "Neo5ByteFeature.h"
#include "Neo6ByteFeature.h"
#include "Neo6xByteFeature.h"
#include "Neo6xxByteFeature.h"
#include "Neo3WordFeature.h"
#include "Neo4WordFeature.h"
#include "Neo5WordFeature.h"

#include "DotStarX4ByteFeature.h"
#include "DotStarL4ByteFeature.h"
#include "DotStarX4WordFeature.h"
#include "DotStarL4WordFeature.h"

// NeoPixel Features
//
#include "NeoRgbFeatures.h"
#include "NeoRgbwFeatures.h"
#include "NeoRgbwwFeatures.h"
#include "NeoRgb48Features.h"
#include "NeoRgbw64Features.h"
#include "NeoRgbww80Features.h"

#include "NeoRgbwxxFeatures.h"
#include "NeoRgbcwxFeatures.h"
#include "NeoRgbwwwFeatures.h"
#include "NeoSm168x3Features.h"
#include "NeoSm168x4Features.h"
#include "NeoSm168x5Features.h"
#include "NeoTm1814Features.h"
#include "NeoTm1914Features.h"

typedef NeoRgb48Feature NeoRgbUcs8903Feature;
typedef NeoRgbw64Feature NeoRgbwUcs8904Feature;
typedef NeoGrb48Feature NeoGrbWs2816Feature;

// DotStart Features
// 
#include "DotStarRgbFeatures.h"
#include "DotStarLrgbFeatures.h"
#include "Lpd6803RgbFeatures.h"
#include "Lpd8806RgbFeatures.h"

#include "P9813BgrFeature.h"
#include "Tlc59711RgbFeatures.h"

// 7 Segment Features
//
#include "NeoAbcdefgpsSegmentFeature.h"
#include "NeoBacedfpgsSegmentFeature.h"

typedef NeoAbcdefgpsSegmentFeature SevenSegmentFeature; // Abcdefg order is default
