#pragma once
#ifdef USE_THEATRIAENGINE_SCRIPT
#define NAMEOF(x) #x

// --- 登録用マクロ（Object::Register などメンバ関数内で使用） ---
#define REGISTER_FIELD(member) \
    do { \
        Script scriptComp = GetComponent<Script>(); \
        scriptComp->fields[#member] = { \
            static_cast<void*>(&(this->member)), \
            std::type_index(typeid(this->member)), \
            {0u, 0u} \
        }; \
    } while(0)

#define REGISTER_FIELD_MINMAX(member, minv, maxv) \
    do { \
        Script scriptComp = GetComponent<Script>(); \
        scriptComp->fields[#member] = { \
            static_cast<void*>(&(this->member)), \
            std::type_index(typeid(this->member)), \
            { static_cast<uint32_t>(minv), static_cast<uint32_t>(maxv) } \
        }; \
    } while(0)

// #define REGISTER_METHOD(method)
#endif // USE_THEATRIAENGINE_SCRIPT
