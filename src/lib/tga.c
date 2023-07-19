#include "tga.h"

uint32_t* tga_parse(uint32_t* data, uint8_t* ptr, int size) {
    int i;
    int j;
    int k;
    int x;
    int y;
    int w = (ptr[13] << 8) + ptr[12];
    int h = (ptr[15] << 8) + ptr[14];
    int o = (ptr[11] << 8) + ptr[10];
    int m = (ptr[1] ? (ptr[7] >> 3) * ptr[5] : 0) + 18;

    if (w < 1 || h < 1) {
        return 0;
    }

    switch (ptr[2]) {
        case 1:
            if (ptr[6] != 0 || ptr[4] != 0 || ptr[3] != 0 || (ptr[7] != 24 && ptr[7] != 32)) {
                return 0;
            }
            for (y = i = 0; y < h; y++) {
                k = ((!o ? h - y - 1 : y) * w);
                for (x = 0; x < w; x++) {
                    j = ptr[m + k++] * (ptr[7] >> 3) + 18;
                    data[2 + i++] = ((ptr[7] == 32 ? ptr[j + 3] : 0xFF) << 24) | (ptr[j + 2] << 16) | (ptr[j + 1] << 8) | ptr[j];
                }
            }
            break;

        case 2:
            if (ptr[5] != 0 || ptr[6] != 0 || ptr[1] != 0 || (ptr[16] != 24 && ptr[16] != 32)) {
                return 0;
            }
            for (y = i = 0; y < h; y++) {
                j = (!o ? h - y - 1 : y) * w * (ptr[16] >> 3);
                for (x = 0; x < w; x++) {
                    data[2 + i++] = ((ptr[16] == 32 ? ptr[j + 3] : 0xFF) << 24) | (ptr[j + 2] << 16) | (ptr[j + 1] << 8) | ptr[j];
                    j += ptr[16] >> 3;
                }
            }
            break;

        case 9:
            if (ptr[6] != 0 || ptr[4] != 0 || ptr[3] != 0 || (ptr[7] != 24 && ptr[7] != 32)) {
                return 0;
            }
            y = i = 0;
            for (x = 0; x < w * h && m < size;) {
                k = ptr[m++];
                if (k > 127) {
                    k -= 127;
                    x += k;
                    j = ptr[m++] * (ptr[7] >> 3) + 18;
                    while (k--) {
                        if (!(i % w)) {
                            i = (!o ? h - y - 1 : y) * w;
                            ++y;
                        }
                        data[2 + i++] = ((ptr[7] == 32 ? ptr[j + 3] : 0xFF) << 24) | (ptr[j + 2] << 16) | (ptr[j + 1] << 8) | ptr[j];
                    }
                } else {
                    ++k;
                    x += k;
                    while (k--) {
                        j = ptr[m++] * (ptr[7] >> 3) + 18;
                        if (!(i % w)) {
                            i = ((!o ? h - y - 1 : y) * w);
                            ++y;
                        }
                        data[2 + i++] = ((ptr[7] == 32 ? ptr[j + 3] : 0xFF) << 24) | (ptr[j + 2] << 16) | (ptr[j + 1] << 8) | ptr[j];
                    }
                }
            }
            break;

        case 10:
            if (ptr[5] != 0 || ptr[6] != 0 || ptr[1] != 0 || (ptr[16] != 24 && ptr[16] != 32)) {
                return 0;
            }
            y = i = 0;
            for (x = 0; x < w * h && m < size;) {
                k = ptr[m++];
                if (k > 127) {
                    k -= 127;
                    x += k;
                    while (k--) {
                        if (!(i % w)) {
                            i = ((!o ? h - y - 1 : y) * w);
                            ++y;
                        }
                        data[2 + i++] = ((ptr[16] == 32 ? ptr[m + 3] : 0xFF) << 24) | (ptr[m + 2] << 16) | (ptr[m + 1] << 8) | ptr[m];
                    }
                    m += ptr[16] >> 3;
                } else {
                    ++k;
                    x += k;
                    while (k--) {
                        if (!(i % w)) {
                            i = ((!o ? h - y - 1 : y) * w);
                            ++y;
                        }
                        data[2 + i++] = ((ptr[16] == 32 ? ptr[m + 3] : 0xFF) << 24) | (ptr[m + 2] << 16) | (ptr[m + 1] << 8) | ptr[m];
                        m += ptr[16] >> 3;
                    }
                }
            }
            break;

        default:
            return 0;
    }

    data[0] = w;
    data[1] = h;

    return data;
}