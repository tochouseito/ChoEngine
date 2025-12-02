#pragma once
#ifdef THEATRIAENGINE_EXPORTS
#define THEATRIA_API __declspec(dllexport)
#else
#define THEATRIA_API __declspec(dllimport)
#endif