/* Sections of the channels in ipc/ (see ipc/MsgRamSections.hpp), shared by the
   linker command files of both cores. Not initialised at boot: each core may
   only write its own message RAM. */
   IPC_CPU1_TO_CPU2 : > IPCCPU1TOCPU2RAM, PAGE = 1, type = NOINIT
   IPC_CPU2_TO_CPU1 : > IPCCPU2TOCPU1RAM, PAGE = 1, type = NOINIT
