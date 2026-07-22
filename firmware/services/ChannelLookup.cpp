#include "ChannelLookup.h"

#include "mesh/Channels.h"

#include <cstring>

int16_t findLiaChannelIndexByName(const char *name)
{
    for (ChannelIndex i = 0; i < channels.getNumChannels(); i++) {
        if (strcasecmp(channels.getName(i), name) == 0)
            return i;
    }
    return -1;
}
