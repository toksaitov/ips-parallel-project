/*
    ips_utils.h

    Created by Dmitrii Toksaitov on Sat Aug 20 10:02:03 KGT 2012
*/

#ifndef IPS_UTILS_H
#define IPS_UTILS_H

static inline float ips_utils_clamp(float x, float min, float max)
{
    return x > max ? max : (x < min ? min : x);
}

int ips_utils_get_number_of_cpu_cores();

char* ips_utils_read_text_file(char *path);

#endif

