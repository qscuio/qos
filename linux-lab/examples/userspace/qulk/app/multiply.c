#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#define OVS_SAT_MUL(X, Y)                                               \
    ((Y) == 0 ? 0                                                       \
     : (X) <= UINT_MAX / (Y) ? (unsigned int) (X) * (unsigned int) (Y)  \
     : UINT_MAX)

int main()
{
    printf("%d\n", OVS_SAT_MUL(10,20));
}


