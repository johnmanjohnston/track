#include "clipboard.h"
#include "track.h"

namespace track::clipboard {
void *data;
int typecode = TYPECODE_NULL;

void setData(void *item, int typehint) {
    data = item;
    typecode = typehint;
}

void *retrieveData() { return data; }
void releaseResources() {
    if (typecode == TYPECODE_CLIP) {
        delete (track::clip *)data;
    }

    typecode = TYPECODE_NULL;
    data = nullptr;
}
} // namespace track::clipboard
