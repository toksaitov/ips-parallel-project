/*
    ips_utils.h

    Created by Dmitrii Toksaitov on Sat Aug 20 10:02:03 KGT 2012
*/

#ifndef IPS_UTILS_H
#define IPS_UTILS_H

#pragma mark - Common Macros

#define IPS_MIN(A,B) (((A)<(B))?(A):(B))
#define IPS_MAX(A,B) (((A)>(B))?(A):(B))
#define IPS_CLAMP(X,MIN,MAX) (IPS_MIN(IPS_MAX((X),(MIN)),(MAX)))
#define IPS_NORMALIZE(X,MIN,MAX) (((X)-(MIN))/((MAX)-(MIN)));

#pragma mark - System Information

int ips_utils_get_number_of_cpu_cores();

#pragma mark - File I/O

char* ips_utils_read_text_file(const char *path);

#endif
