#ifndef PRINT3_DSA_H_
#define PRINT3_DSA_H_

#include <stdlib.h>

#define da_add(da, item)                                                             \
    do {                                                                             \
        if ((da).length >= (da).capacity) {                                          \
            (da).capacity = (da).capacity ? 2 * (da).capacity : 4;                   \
            (da).items = realloc((da).items, (da).capacity * sizeof((da).items[0])); \
            assert((da).items && "Could not resize dynamic array.");                 \
        }                                                                            \
        (da).items[(da).length++] = item;                                            \
    } while (0)

#define da_add3(da, item1, item2, item3) \
    da_add(da, item1);                   \
    da_add(da, item2);                   \
    da_add(da, item3)

#define da_add4(da, item1, item2, item3, item4) \
    da_add(da, item1);                          \
    da_add(da, item2);                          \
    da_add(da, item3);                          \
    da_add(da, item4)

#endif
