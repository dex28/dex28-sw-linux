---------------------------------
XAPP058 Reference C Code v5.01.03
---------------------------------

This dynamic_target subdirectory contains a copy of the micro.c file
from the src directory that has been modified to allow a user to
dynamically specify the target device in a multi-device scan chain
to which the XSVF will be applied.  The basic premise of the modification
is to allow the user to dynamically specify the number of bypass bits
for the leading and trailing device(s).  The number of instruction bits
must be specified as well as the bypass register data bits.


Files/Directories:

  playxsvfdyn501.exe                    - Example executable for Xilinx
                                          Parallel Cable based on
                                          micro_dynamic_target.h/.c.
                                          Works with WinDriver v5.05b or
                                          earlier that is installed with
                                          JTAG Programmer or iMPACT 5.2.03i
                                          or earlier.

  playxsvfdyn501b.exe                   - Example executable for Xilinx
                                          Parallel Cable based on
                                          micro_dynamic_target.h/.c.
                                          Works with WinDriver v6.0 or
                                          later that is installed with
                                          iMPACT 6.1i or later.

  micro_dynamic_target.c                - Modified micro.c.  The main function
                                          xsvfExecute allows you to specify
                                          leading and trailing device info.
                                          Take this file and replace the
                                          micro.c in the src directory
                                          to create an XSVF player with
                                          dynamic targetting capability.

  micro_dynamic_target.h                - .h file for micro_dynamic_target.c.
                                          Defines xsvfExecute interface.

  micro_dynamic_target_for_4_virtex.c   - Example using an array to store
                                          leading and trailing device info
                                          for 4 Virtex devices.

  micro_dynamic_target_for_4_xc18v00.c  - Example using an array to store
                                          leading and trailing device info
                                          for 4 XC18V00 devices.

  The leading and trailing device parameters are:

    iHir        - The number of bits to shift before the instruction from
                  the XSVF file.  Ones are shifted in these header bit
                  positions which comprise the BYPASS instruction to
                  the devices.
                = Sum of the instruction register lengths for devices
                  following the target device.

    iTir        - The number of bits to shift after the instruction from
                  the XSVF file.  Ones are shifted in these trailer bit
                  positions which comprise the BYPASS instruction to
                  the devices.
                = Sum of the instruction register lengths for devices
                  preceding the target device.

    iHdr        - The number of bits to shift before the data from
                  the XSVF file.  Zeros are shifted in these header bit
                  positions.
                = The sum of the bypass register bits for devices following
                  the target device.  Because a device bypass register is
                  always one bit wide, the sum is equivalent to the number
                  of devices following the target device.

    iHdr        - The number of bits to shift after the data from
                  the XSVF file.  Zeros are shifted in these trailing bit
                  positions.
                = The sum of the bypass register bits for devices preceding
                  the target device.  Because a device bypass register is
                  always one bit wide, the sum is equivalent to the number
                  of devices preceding the target device.

    iHdrFpga    - Special data header bit specification for XSDRB and XSDRTDOB
                  commands that are used for shifting the FPGA bitstreams.
                  The Virtex (and Virtex-equivalent devices which include
                  the VirtexE, SpartanII, and Virtex-II)
                  have a peculiar requirement that the bitstream arrives
                  to the TDI pin of the device on a 32-bit boundary.
                  (The Virtex-II bitstream must arrive to the TDI pin on a
                  64-bit boundary.)
                  When the Virtex is in the midst of a scan chain,
                  the bypass bits from the devices preceding the target
                  Virtex must be included in this 32-bit boundary calculation.
                  Thus, the additional header bits must be added to re-align
                  the bitstream data to the 32-bit boundary.  If the target
                  Virtex is not the first device in the scan chain, then
                  the following calculation must be used.
                = 32 - ( sum of bypass register bits for devices preceding the
                  target Virtex device )
                  Again, because the bypass register is always one bit wide,
                  this parameter is equivalent to the following:
                  32 - ( number of devices preceding the target Virtex device )
                  For the Virtex-II the following calculation must be used:
                = 64 - ( sum of bypass register bits for devices preceding the
                  target Virtex device )
                  Again, because the bypass register is always one bit wide,
                  this parameter is equivalent to the following:
                  64 - ( number of devices preceding the target Virtex device )

  Note:  If all of the above parameters are equal to zero, then the
         functionality is equivalent to the base XSVF player src code
         that takes an XSVF file created for a fully specified scan chain!


Create SVF Files to Program Your Device:

  Follow the basic instructions in the readme.txt in the root directory
  for creating the SVF files.  However, you should define the scan chain
  as only a single device (which is your target device kind).  Thus, the
  SVF is for a single device and a single device scan chain.


Translating SVF to XSVF:

  Follow the instructions in the readme.txt in the root directory for
  translating the SVF to XSVF.


XSVF C Code Instructions:

  Replace the micro.c file in the src directory with the
  micro_dynamic_target.c file to add dynamic targeting capability.
  Replace the micro.h file in the src directory with the
  micro_dynamic_target.h file to define the dynamic targeting interface
  to the xsvfExecute function.
  Follow the instructions in the readme.txt in the root directory for
  implementing the src code.


