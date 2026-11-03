/* Message RAM map, shared by the linker command files of both cores. Included
   in PAGE 1 of MEMORY.

   driverlib's ipc.c keeps its message queue buffers (0x88 words) at the start
   of each message RAM. The rest belongs to the channels in ipc/. */
   CPU2TOCPU1RAM    : origin = 0x03F800, length = 0x000100
   IPCCPU2TOCPU1RAM : origin = 0x03F900, length = 0x000300
   CPU1TOCPU2RAM    : origin = 0x03FC00, length = 0x000100
   IPCCPU1TOCPU2RAM : origin = 0x03FD00, length = 0x000300
