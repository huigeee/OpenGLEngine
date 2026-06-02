#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <map>

struct HDRImage {
    float* data;
    int width;
    int height;
    int channels;

    HDRImage() : data(nullptr), width(0), height(0), channels(0) {}
    ~HDRImage() { clear(); }

    void clear();
    bool isValid() const { return data != nullptr; }
};

struct TextureData {
    unsigned char* data;
    unsigned short* data16;
    int width;
    int height;
    int channels;
    bool is16bit;

    TextureData() : data(nullptr), data16(nullptr), width(0), height(0), channels(0), is16bit(false) {}
    ~TextureData() { clear(); }

    void clear();
    bool isValid() const { return data != nullptr || data16 != nullptr; }
};

struct BRDFLUTData {
    float* data;
    int width;
    int height;

    BRDFLUTData() : data(nullptr), width(0), height(0) {}
    ~BRDFLUTData() { clear(); }

    void clear();
    bool isValid() const { return data != nullptr; }
};

class ResourceLoader {
public:
    ResourceLoader();
    ~ResourceLoader();

    void setFilesDir(const char* dir);

    HDRImage loadHDR(const char* relativePath);
    TextureData loadTexture(const char* relativePath, bool flipUv = true);

    static GLuint loadTexture1(const char* path);

    BRDFLUTData generateBRDFLUT(int width, int height);

    void freeHDR(HDRImage& img);
    void freeTexture(TextureData& tex);
    void freeBRDFLUT(BRDFLUTData& lut);

    const std::string& getFilesDir() const { return filesDir; }

private:
    std::string filesDir;
};
