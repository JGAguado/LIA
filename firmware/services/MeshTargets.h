#pragma once

#include "configuration.h"

/// Shared mesh-destination configuration for LIA's services, per
/// firmware/AGENTS.md "Target Node" -- defined once here so TrackerService,
/// ChargeStatusService, and any future service agree.

// MAC E7:25:DC:E6:D6:63 -> NodeNum is the low 32 bits (see NodeDB.cpp's
// nodeNum derivation). Migrate to a configurable node ID in a later phase.
constexpr NodeNum kLiaTargetNode = 0xDCE6D663;

// Messages are sent only on this named channel, never the default/public
// one, so they're decodable only by other users who share its PSK -- see
// firmware/services/README.md "Channel targeting".
constexpr const char *kLiaChannelName = "Test";
