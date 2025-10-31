#include "utility.h"

int track::utility::getIndexOfClip(audioNode *node, clip *clip) {
    int retval = -1;

    for (size_t i = 0; i < node->clips.size(); ++i) {
        if (&node->clips[i] == clip) {
            retval = i;
            break;
        }
    }

    return retval;
}
