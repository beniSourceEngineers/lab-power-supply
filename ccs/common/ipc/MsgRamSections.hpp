#ifndef IPC_MSGRAMSECTIONS_HPP
#define IPC_MSGRAMSECTIONS_HPP

#include <stdint.h>

// Linker sections for IPC channels, placed by msgram_sections.inc. Each section
// holds exactly one object: a POD struct with all storage for that direction.
// Both cores define that object from the same header, so it is at the region's
// origin and has the same layout on both:
//
//   #pragma DATA_SECTION(IPC_SECTION_CPU1_TO_CPU2)
//   Cpu1ToCpu2Channels cpu1ToCpu2;
//
// The sections are NOINIT, as each core may only write its own message RAM. Do
// not use MSGRAM_CPU*_TO_CPU*: driverlib's ipc.c has its buffers there.
#define IPC_SECTION_CPU1_TO_CPU2 "IPC_CPU1_TO_CPU2"
#define IPC_SECTION_CPU2_TO_CPU1 "IPC_CPU2_TO_CPU1"

namespace ipc {

// Origins of the regions, same as in msgram_memory.inc. For checks at startup.
const uint32_t kChannelsAddrCpu1ToCpu2 = 0x03FD00UL;
const uint32_t kChannelsAddrCpu2ToCpu1 = 0x03F900UL;

// Size of each region in 16-bit words, same as in msgram_memory.inc.
const uint32_t kChannelsRegionWords = 0x300UL;

} // namespace ipc

#endif
