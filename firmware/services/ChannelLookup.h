#pragma once

#include <cstdint>

/// Returns the index of the channel named `name` (case-insensitive), or -1
/// if no such channel is currently configured. Callers must fail closed on
/// -1 rather than falling back to the default/public channel -- that
/// fallback would silently defeat the point of restricting a send to a
/// named, private channel. See firmware/services/README.md "Channel
/// targeting".
int16_t findLiaChannelIndexByName(const char *name);
