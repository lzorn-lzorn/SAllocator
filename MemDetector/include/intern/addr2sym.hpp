#pragma once

#include <iostream>
#include <string>

#if __unix__
# include <cxxabi.h>
# include <execinfo.h>

inline std::string addr2sym(void *addr) {
    if (addr == nullptr) {
        return "null";
    }
    char **strings = backtrace_symbols(&addr, 1);
    if (strings == nullptr) {
        return "???";
    }
    std::string ret = strings[0];
    free(strings);
    auto pos = ret.find('(');
    if (pos != std::string::npos) {
        auto pos2 = ret.find('+', pos);
        if (pos2 != std::string::npos) {
            auto pos3 = ret.find(')', pos2);
            auto offset = ret.substr(pos2, pos3 - pos2);
            if (pos2 != pos + 1) {
                ret = ret.substr(pos + 1, pos2 - pos - 1);
                char *demangled =
                    abi::__cxa_demangle(ret.data(), nullptr, nullptr, nullptr);
                if (demangled) {
                    ret = demangled;
                    free(demangled);
                } else {
                    ret += "()";
                    ret += offset;
                }
            } else {
                ret = ret.substr(0, pos) + offset;
                auto slash = ret.rfind('/');
                if (slash != std::string::npos) {
                    ret = ret.substr(slash + 1);
                }
            }
        }
    }
    return ret;
}
#elif _MSC_VER
# include <windows.h>
# include <dbghelp.h>
# include <stdio.h>

# pragma comment(lib, "dbghelp.lib")

extern "C" char *__unDName(char *, char const *, int, void *, void *, int);

inline void init_dbg_help() {
    static bool initialized = false;
    if (!initialized) {
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        initialized = true;
    }
}


inline std::string addr2sym(void *addr) {
    // std::cout << "_MSC_VER" << std::endl;
    init_dbg_help();
    if (addr == nullptr) {
        return "null";
    }
    DWORD64 dwDisplacement;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;
    if (!SymFromAddr(GetCurrentProcess(), (DWORD64)addr, &dwDisplacement,pSymbol)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "0x%llu", (DWORD64)addr);
        return buf;
    }

    char undecorated[1024] = {};
    if (UnDecorateSymbolName(pSymbol->Name, undecorated, sizeof(undecorated), UNDNAME_COMPLETE)) {
        return std::string(undecorated);
    }

    return std::string(pSymbol->Name);  // fallback
}
#elif _WIN32
# include <windows.h>
# include <dbghelp.h>

# pragma comment(lib, "dbghelp.lib")
inline void init_dbg_help() {
    static bool initialized = false;
    if (!initialized) {
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        initialized = true;
    }
}


inline std::string addr2sym(void *addr) {
    init_dbg_help();
    std::cout << "_WIN32" << std::endl;
    DWORD64 dwDisplacement = 0;
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    if (!SymFromAddr(GetCurrentProcess(), (DWORD64)addr, &dwDisplacement,
                     pSymbol)) {
        return "???";
    }
    std::string name(pSymbol->Name);
    // demangle
    // if (name.find("??") == 0) {
    //     char undname[1024];
    //     if (UnDecorateSymbolName(pSymbol->Name, undname, sizeof(undname),
    //                              UNDNAME_COMPLETE)) {
    //         name = undname;
    //     }
    // }
    // return name;
    char undecorated[1024];
    if (UnDecorateSymbolName(pSymbol->Name, undecorated, sizeof(undecorated), UNDNAME_COMPLETE)) {
        return undecorated;
    }
    return pSymbol->Name;
}
#else
# include <sstream>

inline std::string addr2sym(void *addr) {
    if (addr == nullptr) {
        return "null";
    }
    std::ostringstream oss;
    oss << "0x" << std::hex << addr;
    return oss.str();
}
#endif
