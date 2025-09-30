// use hex cuz it makes me feel cool
#define TYPECODE_NULL 0x00
#define TYPECODE_CLIPCOMPONENT 0x01
#define TYPECODE_CLIP 0x02
#define TYPECODE_PLUGIN 0x03

namespace track::clipboard {
extern void *data;
extern int typecode;

void setData(void *item, int typehint);
void *retrieveData();
void releaseResources();
} // namespace track::clipboard
