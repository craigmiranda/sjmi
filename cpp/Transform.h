#pragma once

template <class T>T Trim(T n) {

    if (n < 0) return 0;
    if (n > 255) return 255;
    return n;
}

char* YUY2toRGB24(short Y, short Cb, short Cr)
{

    char rgb[3]{};

    short C = Y - 16;
    short D = Cr - 128;
    short E = Cb - 128;

    // calc and set pixel (a) RGB values
    rgb[0] = (char)Trim((298 * C + 409 * E + 128) / 256);
    rgb[1] = (char)Trim((298 * C - 100 * D - 208 * E + 128) / 256);
    rgb[2] = (char)Trim((298 * C + 516 * D + 128) / 256); 

    return rgb;
}


boolean YUY2toRGB24(BYTE*& yuv2, const UINT32& origSize, jbyte*& rgb24)
{

    for (int i = 0; i < origSize - 4; i += 4) { // pick up 4 bytes at a time (ie. 2 pixels)

        // get YUY2 bytes for 2 pixels
        const short aY = (short)yuv2[i]; // use 8byte ints for decimal arithmetic
        const short Cr = (short)yuv2[i + 1];
        const short bY = (short)yuv2[i + 2];
        const short Cb = (short)yuv2[i + 3];

        int pxByteIndex = (i / PX_BYTES_YUY2) * PX_BYTES_RGB; // RGB byte stream index of this red (a) pixel
        char* pxA = YUY2toRGB24(aY, Cr, Cb);

        for (int i = 0; i < PX_BYTES_RGB; ++i)
            rgb24[pxByteIndex+i] = pxA[i];

        pxByteIndex += PX_BYTES_RGB;
        char* pxB = YUY2toRGB24(bY, Cr, Cb);

        for (int i = 0; i < PX_BYTES_RGB; ++i)
            rgb24[pxByteIndex+i] = pxB[i];
    }

    return true;
}
