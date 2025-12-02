#ifndef PHYSICS_API_H
#define PHYSICS_API_H

// ------------------------
// DLLExportMacro
// ------------------------
#ifdef PHYSICS_EXPORTS
#define PHYSICS_API __declspec(dllexport)
//#define REGISTER_SCRIPT_FACTORY(SCRIPTNAME) \
//    extern "C" __declspec(dllexport) Marionnette* Create##SCRIPTNAME##Script(GameObject& object) { \
//        return new SCRIPTNAME(object);}
#else
#define PHYSICS_API __declspec(dllimport)
#endif

#endif // PHYSICS_API_H
