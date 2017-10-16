/*
    ips_utils.c

    Created by Dmitrii Toksaitov, 2012
*/

#include "ips_utils.h"

#ifdef _WIN32
    #include <windows.h>
#elif MACOS
    #include <sys/param.h>
    #include <sys/sysctl.h>
#else
    #include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#pragma mark - System Information

int ips_utils_get_number_of_cpu_cores()
{
    int result;

#ifdef WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    result = info.dwNumberOfProcessors;
#elif MACOS
    int nm[2];

    size_t length = 4;
    uint32_t count;

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;

    sysctl(nm, 2, &count, &length, NULL, 0);

    if (count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &length, NULL, 0);
    }

    result = count;
#else
    result = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    if (result < 1) {
        result = 1;
    }

    return result;
}

#pragma mark - File I/O

char* ips_utils_read_text_file(const char *path)
{
    char *text = NULL;

    long file_size;
    size_t last_position;

    FILE *file = fopen(path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        rewind(file);

        if (file_size > 0) {
            text = (char *) malloc(sizeof(*text) * ((size_t) file_size + 1));
            last_position = fread(text, sizeof(*text), (size_t) file_size, file);
            text[last_position] = '\0';
        }
    }

    return text;
}
