#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

#include "bmp.hpp"

Bitmap::Bitmap()
    :fileHeader(new BITMAPFILEHEADER),
     infoHeader(new BITMAPINFOHEADER),
     extraHeader(),
     grey(true)
{
}

Bitmap::Bitmap(const Bitmap &rhs)
{
    fileHeader = rhs.fileHeader;
    infoHeader = rhs.infoHeader;

    if (rhs.grey)
    {
        size_t size = rhs.infoHeader->biHeight * rhs.infoHeader->biWidth;
        greyData = new float[size];
        memcpy(greyData, rhs.greyData, size*sizeof(float));
    }
    else
    {
        size_t size = rhs.infoHeader->biHeight * rhs.infoHeader->biWidth;
        colourData = new cl_float4[size];
        memcpy(colourData, rhs.colourData, size*sizeof(cl_float4));
    }
}

Bitmap & Bitmap::operator=(const Bitmap &rhs)
{
    fileHeader = rhs.fileHeader;
    infoHeader = rhs.infoHeader;

    if (rhs.grey)
    {
        size_t size = rhs.infoHeader->biHeight * rhs.infoHeader->biWidth;
        greyData = new float[size];
        memcpy(greyData, rhs.greyData, size*sizeof(float));
    }
    else
    {
        size_t size = rhs.infoHeader->biHeight * rhs.infoHeader->biWidth;
        colourData = new cl_float4[size];
        memcpy(colourData, rhs.colourData, size*sizeof(cl_float4));
    }

    return *this;
}

void Bitmap::write(string filename)
{
    ofstream file(filename.c_str());

    file.write((char*)(fileHeader), sizeof(BITMAPFILEHEADER));
    file.write((char*)(infoHeader), sizeof(BITMAPINFOHEADER));
    file.write((char*)(&extraHeader[0]), extraHeader.size());

    cout << "Saving filtered image to " << filename << endl;

    int height = infoHeader->biHeight;
    int width = infoHeader->biWidth;

    int mod = width % 4;
    if(mod != 0) {
        mod = 4 - mod;
    }


    if(grey)
    {
        char tmp;
        char junk = 0;
        for(int i = 0; i < height; i++) {
            // add actual data to the image
            for(int j = 0; j < width; j++) {
                tmp = (unsigned char)greyData[i*width + j];
                file.write(&tmp, 1);
            }
            // For the bmp format, each row has to be a multiple of 4,
            // so I need to read in the junk data and throw it away
            for(int j = 0; j < mod; j++) {
                file.write(&junk, 1);
            }
        }
    }
    else
    {
        char tmp;
        char junk = 0;
        cl_float4 *elt;
        bool alpha = infoHeader->biBitCount == 32;
        for(int i = 0; i < height; i++) {
            // add actual data to the image
            for(int j = 0; j < width; j++) {
                elt = &colourData[i*width + j];
                tmp = (unsigned char)elt->x;
                file.write(&tmp, 1);
                tmp = (unsigned char)elt->y;
                file.write(&tmp, 1);
                tmp = (unsigned char)elt->z;
                file.write(&tmp, 1);
                if (alpha)
                {
                    //tmp = (unsigned char)elt->w;
                    tmp = (unsigned char)0;
                    file.write(&tmp,1);
                }
            }
            // For the bmp format, each row has to be a multiple of 4,
            // so I need to read in the junk data and throw it away
            for(int j = 0; j < mod; j++) {
                file.write(&junk, 1);
            }
        }
    }
}

void Bitmap::read(string filename)
{
    ifstream file;
    file.open(filename.c_str());

    if (file) {
        file.read((char*)fileHeader, sizeof(BITMAPFILEHEADER));
        file.read((char*)infoHeader, sizeof(BITMAPINFOHEADER));

        int height = infoHeader->biHeight;
        int width = infoHeader->biWidth;

        cout << "Using input file " << filename << endl;
        cout << "Dimensions: " << height << 'x' << width << endl;

        int mod = width % 4;
        if(mod != 0) {
            mod = 4 - mod;
        }

        //check if there is extra information, like a colour table
        if (file.tellg() != fileHeader->bfOffBits)
        {
            size_t tableSize = fileHeader->bfOffBits - file.tellg();
            extraHeader.resize(tableSize);
            file.read(&extraHeader[0], tableSize);
        }

        if (infoHeader->biBitCount == 8)
        {
            grey = true;

            greyData = new float[height*width];

            char tmp;
            for(int i = 0; i < height; i++)
            {
                for(int j = 0; j < width; j++) {
                    file.read(&tmp,1);
                    greyData[i*width + j] = (float)(unsigned char)tmp;
                }
                // For the bmp format, each row has to be a multiple of 4,
                // so I need to read in the junk data and throw it away
                for(int j = 0; j < mod; j++)
                {
                    file.read(&tmp,1);
                }
            }
        }
        else
        {
            grey = false;

            colourData = new cl_float4[height*width];

            cl_float4 *elt;
            char tmp;
            bool alpha = infoHeader->biBitCount == 32;
            for(int i = 0; i < height; i++)
            {
                for(int j = 0; j < width; j++) {
                    elt = &colourData[i*width + j];
                    file.read(&tmp,1);
                    elt->x = (float)(unsigned char)tmp;
                    file.read(&tmp,1);
                    elt->y = (float)(unsigned char)tmp;
                    file.read(&tmp,1);
                    elt->z = (float)(unsigned char)tmp;
                    if (alpha)
                    {
                        file.read(&tmp,1);
                        elt->w = (float)(unsigned char)tmp;
                    }
                }
                // For the bmp format, each row has to be a multiple of 4,
                // so I need to read in the junk data and throw it away
                for(int j = 0; j < mod; j++)
                {
                    file.read(&tmp,1);
                }
            }
        }
    }
}
