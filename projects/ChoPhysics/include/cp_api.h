#ifndef CP_API_H
#define CP_API_H

// ------------------------
// DLLExportMacro
// ------------------------
#ifdef CHOPHYSICS_EXPORTS
#define CP_API __declspec(dllexport)
//#define REGISTER_SCRIPT_FACTORY(SCRIPTNAME) \
//    extern "C" __declspec(dllexport) Marionnette* Create##SCRIPTNAME##Script(GameObject& object) { \
//        return new SCRIPTNAME(object);}
#else
#define CP_API __declspec(dllimport)
#endif

#endif // CP_API_H
