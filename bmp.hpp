#pragma once

#include <CL/cl.hpp>
#include <string>
#include <vector>
typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#pragma pack(push,1)
typedef struct tagBITMAPFILEHEADER {
  WORD  bfType;
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#pragma pack(pop)

struct Bitmap
{
public:
    Bitmap();
    Bitmap(const Bitmap &rhs);
    Bitmap & operator=(const Bitmap &rhs);

    PBITMAPFILEHEADER fileHeader;
    PBITMAPINFOHEADER infoHeader;
    std::vector<char> extraHeader;
    bool grey;

    void write(std::string filename);
    void read(std::string filename);

    union
    {
        cl_float4 *colourData;
        float *greyData;
    };
};
