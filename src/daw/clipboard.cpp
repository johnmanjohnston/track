#include "clipboard.h"
#include "plugin_chain.h"
#include "track.h"

namespace track::clipboard {
void *data;
int typecode = TYPECODE_NULL;

void setData(void *item, int typehint) {
    releaseResources();

    data = item;
    typecode = typehint;
}

void *retrieveData() { return data; }
void releaseResources() {
    if (typecode == TYPECODE_CLIP)
        delete (track::clip *)data;
    else if (typecode == TYPECODE_PLUGIN)
        delete (track::pluginClipboardData *)data;
    else if (typecode == TYPECODE_PLUGIN_CHAIN)
        delete (track::pluginChainClipboardData *)data;
    else if (typecode == TYPECODE_NODE)
        delete (track::audioNode *)data;

    typecode = TYPECODE_NULL;
    data = nullptr;
}
} // namespace track::clipboard
