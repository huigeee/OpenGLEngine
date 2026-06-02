#pragma once

#include <string>

class ShaderManager {
public:
    static const char* getUnlitVertex();
    static const char* getUnlitFragment();
    static const char* getLitVertex();
    static const char* getLitFragment();
    static const char* getPBRVertex();
    static const char* getPBRFragment();
    static const char* getPBRClearCoatVertex();
    static const char* getPBRClearCoatFragment();
    static const char* getPBRIBLVertex();
    static const char* getPBRIBLFragment();
    static const char* getPBRClearCoatIBLVertex();
    static const char* getPBRClearCoatIBLFragment();
    static const char* getDebugIBLVertex();
    static const char* getDebugIBLFragment();
};
