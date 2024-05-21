/* A simple test harness for memory alloction. */

#include "mm_alloc.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    int *data;

    data = (int *)mm_malloc(4);
    printf("%p\n", data);
    data[0] = 1;
    printf("%d\n", data[0]);
    mm_free(data);
    printf("%d\n", data[0]);
    printf("malloc sanity test successful!\n");
    return 0;
}
