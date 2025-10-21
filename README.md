# CoreAssign 
Simple utility to list the CPU core capabilities within the current system, and assign affinity for one or processes to low or high powered cores.


# Sample output for help.
CoreAssign v1.0, Copyright Microsoft Ltd [2020]

Displays details of CPU core capability and sets process affinity to high or low powered cores.

COREASSIGN [/LIST] [/L] [/ATTACH:<PROC-ID>|<REGX-PROCESS-DISPLAY-NAME> [HIGH|LOW]] [/HELP] [/H] [/?]
           [/QUIET] [/Q]

  /LIST /L        List the cores available with detail of high / low power.
  /HELP /H /?     Display this help text.
  /ATTACH:<PROC-ID>|<REGX-PROCESS-DISPLAY-NAME> [HIGH|LOW]
                  Set the core affinity for the process identified by the PROC-ID or <REGX-PROCESS-DISPLAY-NAME>
                  to either a high powered core if HIGH is specified (also by default) or to low powered
                  core if LOW is specified.
                  PROC-ID is the decimal proccess ID for the target single process.
                  REGX-PROCESS-DISPLAY-NAME is a regular expression that will be used to search all running
                  processes, comparing to the display name for the process. Affinity will be set for all matches.
  /QUIET /Q       Suppress the tool banner and other output.
  
# CoreAssign.exe /List
Output on a Xeon W-2275:
CoreAssign v1.0, Copyright Microsoft Ltd [2020]

Found 14 cores grouped by 1 power levels.
There are no capability differences for cores in this system.

Output on a Surface Pro X (SQ2):
CoreAssign v1.0, Copyright Microsoft Ltd [2020]

Found 8 cores grouped by 2 power levels.

Core group: Background cores
  Mask: 0x000f   00001111

Core group: High performance cores
  Mask: 0x00f0   11110000

Full core list:
Core Mask: 0x0001  00000001  efficiency: 0
Core Mask: 0x0002  00000010  efficiency: 0
Core Mask: 0x0004  00000100  efficiency: 0
Core Mask: 0x0008  00001000  efficiency: 0
Core Mask: 0x0010  00010000  efficiency: 1
Core Mask: 0x0020  00100000  efficiency: 1
Core Mask: 0x0040  01000000  efficiency: 1
Core Mask: 0x0080  10000000  efficiency: 1

# CoreAssign.exe /Attach:CPUPerfText.exe High
CoreAssign v1.0, Copyright Microsoft Ltd [2020]

Setting affinity for all processes matching the name: CPUPerfTest.exe to high powered cores.
Found process ID:0xe04 with name 'CPUPerfTest.exe'. Set succeeded.

# CoreAssign.exe /Attach:CPUPe.* High
CoreAssign v1.0, Copyright Microsoft Ltd [2020]

Setting affinity for all processes matching the name: CPUPe.* to high powered cores.
Found process ID:0xe04 with name 'CPUPerfTest.exe'. Set succeeded.

# CoreAssign.exe /Attach:3588 High
CoreAssign v1.0, Copyright Microsoft Ltd [2020]

Setting affinity for process id:3588 to high powered cores.

