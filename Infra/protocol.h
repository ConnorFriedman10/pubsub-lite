#pragma once
#include <cstdint>

// First byte of every message sent to/from the broker
const uint8_t MSG_SUBSCRIBE = 0x01;
const uint8_t MSG_PUBLISH   = 0x02;
const uint8_t MSG_DATA      = 0x03;
