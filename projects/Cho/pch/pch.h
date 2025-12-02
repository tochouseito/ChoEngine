#pragma once

//#define WIN32_LEAN_AND_MEAN // ヘッダーから余計な情報を除外

#ifdef NDEBUG
#include <cassert>
#endif // NDEBUG

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <chomath.h>
using namespace Theatria::Math;
#include "Core/ChoLog/ChoLog.h"
using namespace theatria;
