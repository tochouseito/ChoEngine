// pch.h: プリコンパイル済みヘッダー ファイルです。
// 次のファイルは、その後のビルドのビルド パフォーマンスを向上させるため 1 回だけコンパイルされます。
// コード補完や多くのコード参照機能などの IntelliSense パフォーマンスにも影響します。
// ただし、ここに一覧表示されているファイルは、ビルド間でいずれかが更新されると、すべてが再コンパイルされます。
// 頻繁に更新するファイルをここに追加しないでください。追加すると、パフォーマンス上の利点がなくなります。

#ifndef CHOPHYSICS_PCH_H
#define CHOPHYSICS_PCH_H

// プリコンパイルするヘッダーをここに追加します
#define WIN32_LEAN_AND_MEAN					// Windows ヘッダーからほとんど使用されていない部分を除外する
#define NOMINMAX							// min / max マクロの定義を抑止（std::min 等との衝突を回避）
#define _WINSOCK_DEPRECATED_NO_WARNINGS		// 古いWinsock関数の警告抑制（inet_addrなど）
#define _CRT_SECURE_NO_WARNINGS				// strcpy, fopenなどの警告抑制
#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING  // C++17関連の警告抑制
#define _USE_MATH_DEFINES					// M_PIなどの数学定数を使えるようにする

// ------------------------
// Windows API
// ------------------------
#include <windows.h>

// ------------------------
// C++ / 標準ライブラリ
// ------------------------
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <numbers>
#include <ctime>
#include <cstdint>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>
#include <tuple>
#include <limits>
#include <chrono>
#include <stdexcept>
#include <exception>

// ------------------------
// マルチスレッド用(予定)
// ------------------------
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <atomic>

// ------------------------
// DirectX12用(予定)
// ------------------------
// #include <wrl.h> // Microsoft::WRL::ComPtr

// ------------------------
// デバッグ
// ------------------------
#ifdef _DEBUG
    #define ENABLE_ASSERT
    #include <cassert>
    #define DEBUG_LOG(x) std::cout << "[DEBUG] " << x << std::endl;
    #include <crtdbg.h>        // メモリリーク検出 
#endif // _DEBUG

// ------------------------
// リリース
// ------------------------

#ifdef NDEBUG
    // リリースビルドでは無効化
    #define assert(x) ((void)0)
    #define DEBUG_LOG(x) ((void)0)
#endif // NDEBUG

#endif // CHOPHYSICS_PCH_H
