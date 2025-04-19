#include "clipboard.h"

namespace track::clipboard {
void *data;
int typecode = TYPECODE_NULL;

void setData(void *item, int typehint) {
    data = item;
    typecode = typehint;
}

void *retrieveData() { return data; }
} // namespace track::clipboard
