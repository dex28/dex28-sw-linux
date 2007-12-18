                    ------------------------------
                    XAPP058 Reference C Code v5.01
                    ------------------------------
                              10/17/2004

[NEW] SVF2XSVF translator support on Linux.
      Note:  Example XSVF PLAYER is not supported on Linux.

[NEW] SVF2XSVF v5.02 backward-compatible with XSXVF v4.xx player for
      XC18V00 and XCF00S PROMs.
      (See readme_xc18v00_xcf00s.txt for important information!!!)

Find technical support answers or contact information at:
http://www.support.xilinx.com

This package contains the translator and reference C code associated with
XAPP058 (http://www.xilinx.com/xapp/xapp058.pdf).

There are two major components included in this archive:
1.  The SVF-to-XSVF translator.
2.  XSVF player reference C code which you must port to your platform.
    (An example port of the player is provided for the Windows platforms
    for use with the Xilinx Parallel Cables.)


Files/Directories:

  readme.txt - This readme.txt file.

  bin/nt/svf2xsvf502.exe- The SVF-to-XSVF translator for Win32 platforms.
  bin/lin/svf2xsvf502   - The SVF-to-XSVF translator for Linux platforms.
  bin/sol/svf2xsvf502   - Solaris version of svf2xsvf translator.
  bin/hp/svf2xsvf502    - HPUX version of the svf2xsvf translator.

  bin/nt/playxsvf501.exe- Test XSVF player for use with the Xilinx Parallel
                          Cable III/IV on a PC with WinDriver v5.05b or earlier
                          (a.k.a. windrvr.sys) that was installed with
                          JTAG Programmer or iMPACT 5.2.03i or earlier.

  bin/nt/playxsvf501b.exe-Test XSVF player for use with the Xilinx Parallel
                          Cable III/IV on a PC with WinDriver v6.0 or later
                          (a.k.a. windrvr6.sys) that was installed with
                          iMPACT 6.1i or later.

  src/       - This directory contains the reference C code that executes the
               XSVF file format.  See "XSVF C Code Instructions" (below)
               on how to port this code to your platform.
               This code is provided as is and without restriction.

  old/       - This directory contains previous versions of the XAPP058
               source code.  This is provided so that you can diff the
               old version against the latest version to find the differences.

  old/old_v2.00.zip - Old v2.00 svf2xsvf translator provided in case you
                      want to use the XSVF compression mechanism for XC9500
                      or XC9500XL SVF files.  (The v5.xx translator does not
                      implement this compression technique.  However, the
                      v5.xx XSVF player does support this compression
                      mechanism.)


Create SVF Files to Program Your Device:

  XC9500/XC9500XL/XC9500XV/CoolRunner/CoolRunner-II/XC18V00/XCF00/FPGA/
  System ACE MPM/SC:
    - Use iMPACT 5.2.03i or later (available from the WebPACK at
      http://www.xilinx.com/sxpresso/webpack.htm)
    - See iMPACT Help.
    - Create an SVF file that contains all the necessary operations to
      program your target devices.
        - Operations for XC9500/XL, CoolRunner/II, XC18V00, XCF00:
            Erase, Program, Verify
                (Operation->Program + check Erase before programming & Verify)
        - Operations for FPGA:
            Program
                (Operation->Program [no other options required])

Translating SVF to XSVF:

  SVF2XSVF Basic Syntax:

    svf2xsvf501 [options] -i input.svf -o output.xsvf -a output.txt

      -i input.svf   = any input file name for the input SVF file.
      -o output.xsvf = any output file name for output XSVF file.
      -a output.txt  = any output file name for output text version
                       of the XSVF file.  (For debugging/analysis.)

      [options] may be zero or more of the following:
      -d            = delete pre-existing output files.
      -w            = delete pre-existing output files.
      -r N          = max "retry" count = N times.  (XC9500/XL/XV only)
      -fpga         = Special translation for FPGA bitstreams.  (FPGA only)
      -extensions   = add special TAP command extensions (CoolRunner/II only)
      -xwait        = Use the inline XWAIT XSVF command (CoolRunner/II/XC18V00/XCF00)
      -minxstate    = Minimize XSTATE XSVF commands
      -v2           = backward compatible XSVF with XSVF v2 player.
      -xprintsrc N  = print various levels of comments into XSVF (N=0-3)


  SVF2XSVF Options Requirements for Each Family:

    XC9500/XL/XV:                      // No options required

    CoolRunner/II: -r 0 -extensions -xwait

    XC18V00/XCF00: -r 0                // 0 = zero
        (See readme_xc18v00_xcf00s.txt for more XC18V00/XCF00S information!!!)

    All Spartan-II/Spartan-3/Virtex+ Platform FPGAs:
                  -fpga -extensions    // automatically sets -r 0

    All XC4000+/Spartan/Spartan-XL FPGAs:
                  -fpga -rlen 1024     // automatically sets -r 0

    System ACE MPM/SC: -r 0 -xwait

  Command-line Translation Examples:

    XC9500/XL/XV:
    svf2xsvf -d -i xc9536.svf -o xc9536.xsvf -a xc9536.txt

    CoolRunner/II:
    svf2xsvf -d -r 0 -extensions -xwait -i xcr3064xl.svf
             -o xcr3064xl.xsvf -a xcr3064xl.txt

    XC18V00/XCF00:
    svf2xsvf -d -r 0 -i prom.svf -o prom.xsvf -a prom.txt
    (See readme_xc18v00_xcf00s.txt for more XC18V00/XCF00S information!!!)

    Spartan-II/Spartan-3/Virtex+ Platform FPGAs:
    svf2xsvf -d -fpga -extensions -i xcv300.svf -o xcv300.xsvf -a xcv300.txt

    XC4000+/Spartan/Spartan-XL FPGAs:
    svf2xsvf -d -fpga -rlen 1024 -i fpga.svf -o fpga.xsvf -a fpga.txt

    System ACE MPM/SC:
    svf2xsvf -d -r 0 -xwait -i mpm.svf -o mpm.xsvf -a mpm.txt


XSVF C Code Instructions:

  Implement the functions in PORTS.C for your system:
    setPort()    - Set the JTAG signal value.
    readTDOBit() - Get the JTAG TDO signal value.
    waitTime()   - Tune to wait for the given number of microseconds.
                   (Warning:  Some compilers eliminate empty loops
                    in optimized/release mode.)
                   This function is called when the device is in the
                   Run-Test/Idle state to wait in that state for the
                   specified amount of time.
                   Note:  For all devices except the Virtex-II only the
                   time spent waiting in Run-Test/Idle is important and TCK
                   pulses issued during this time are ignored.  The Virtex-II
                   is different in that the given number of microseconds must
                   be interpreted as a minimum number of TCK pulses that must
                   be generated within the Run-Test/Idle state.
    readByte()   - Get the next byte from the XSVF source location.

  Compile all .c code and call xsvfExecute() (defined in micro.h) to
  start the execution of the XSVF.


  Optimizations:
    lenval.h:  #define MAX_LEN 7000
      This #define defines the maximum length (in bytes) of predefined
      buffers in which the XSVF player stores the current shift data.
      This length must be greater than the longest shift length (in bytes)
      in the XSVF files that will be processed.  7000 is a very conservative
      number.  The buffers are stored on the stack and if you have limited
      stack space, you may decrease the MAX_LEN value.

      How to find the "shift length" in bits?
      Look at the ASCII version of the XSVF (generated with the -a option
      for the SVF2XSVF translator) and search for the XSDRSIZE command
      with the biggest parameter.  XSDRSIZE is equivalent to the SVF's
      SDR length plus the lengths of applicable HDR and TDR commands.
      Remember that the MAX_LEN is defined in bytes.  Therefore, the
      minimum MAX_LEN = ceil( max( XSDRSIZE ) / 8 );

      The following MAX_LEN values have been tested and provide relatively
      good margin for the corresponding devices:

        DEVICE       MAX_LEN   Resulting Shift Length Max (in bits)
        ---------    -------   ----------------------------------------------
        XC9500/XL/XV 32        256

        CoolRunner/II 256      2048   - actual max 1 device = 1035 bits

        FPGA         128       1024    - svf2xsvf -rlen 1024

        XC18V00/XCF00
                     1100      8800   - no blank check performed (default)
                                      - actual max 1 device = 8192 bits verify
                                      - max 1 device = 4096 bits program-only

        XC18V00/XCF00 when using the optional Blank Check operation
                     2500      20000  - required for blank check
                                      - blank check max 1 device = 16384 bits


    micro.c:  #define XSVF_SUPPORT_COMPRESSION
      This #define includes support for an XSVF compression mechanism
      that was used by the previous v2.00 SVF2XSVF translator.  If you
      intend to execute v2.00 XSVF with compress, then make sure this
      is defined.  Otherwise, the current v5.xx translator does not
      support this compression mode.  You may omit this definition
      the code size and runtime memory requirements are reduced.


Trouble-shooting:

    For XC18V00 failures, see readme_xc18v00_xcf00s.txt.

    Example playxsvf501.exe does not connect to the cable.
    - If iMPACT is running, iMPACT locks exclusive use of the parallel port.
      Close iMPACT, launch iMPACT, auto-initialize the chain, and close
      iMPACT to reset and disconnect iMPACT from the cable.
    - Try playxsvf501b.exe to see if it works with windrvr6.sys.

    The XSVF fails in the embedded implementation.
    - Use the Xilinx iMPACT software to see if it can program
      the devices on your board through the Xilinx Parallel Cable.
    - Use the bin\nt\playxsvf.exe with the Xilinx Parallel Cable
      from a PC to see if it can program the devices on your board with your
      XSVF file through the Parallel Cable.
      playxsvf has a verbose mode (-v level).
      If the XSVF player reports a mismatch at an XSVF command, it is
      reporting the XSVF command count.  This is equivalent to the line
      number in the corresponding XSVF ascii (.txt) file which can be
      generated using the -a option for the svf2xsvf translator.
    - Create an XSVF file that performs only an IDCODE check
      ("Operation->Get Device ID").  This will verify the TAP functionality
      and communication.
    - Create separate XSVF files for erase-only, blank-check-only,
      program-only, and verify-only to narrow the error region.

    Use the playxsvf example compiled for the Simulator target to see
    the TAP transitions.

    Make sure the compiler is not optimizing out empty timing loops that
    you may have inserted as the implementation of the waitTime function.
    Timing should be within +/- 25% for robust programming.

    Check Xilinx Solution Records at http://support.xilinx.com.

----------------------------------------------------------
XAPP058 - SVF2XSVF Translator and Reference C Code History
----------------------------------------------------------
v5.01:
    Added setPort( TCK, 0 ); to waitTime() function in PORTS.C to ensure
    that TCK is returned to 0 (at least once) at the beginning of
    a Run-Test/Idle wait period for the XC18V00/XCF00.  This addition
    is required for implementations that do NOT toggle TCK during the
    wait period.
    The SVF2XSVF 5.02 translator made backward-compatible with
    v4.xx XSVF player for the XC18V00/XCF00S devices.
    (See readme_xc18v00_xcf00s.txt for more XC18V00/XCF00S information!!!)

v5.00:
    Improved SVF STATE command support.
    Added XWAIT command for on-demand wait instead of pre-specified XRUNTEST.
    XWAIT also required for CoolRunner/II.
    The "extensions", that were previously only enabled via the
    XSVF_SUPPORT_EXTENSIONS macro, is made a standard part of the XSVF player.

v4.14:
    Support XCOMMENT XSVF command for storing SVF comments and/or line
    numbers in the XSVF.

v4.11:
    SVF2XSVF translator update only--no changes to XSVF player C code.
    Add -multiple_runtest option for iMPACT CoolRunner SVF.

v4.10:
    Add new XSIR2 command that supports IR shift lengths > 255 bits.

v4.07:
    [Add Notes] Add notes and code examples for the waitTime function
    in the ports.c file that satisfy the TCK pulse requirements for the
    Virtex-II.

v4.06:
    [Bug Fix] Fixed xsvfGotoTapState for problem with retry transition.

v4.05:
    [Bug Fix] Fixed backward compatibility problem of previous "alpha"
        and "beta" versions which did not work with some older XC9500 devices.
        Fix required XSVF player C source code and svf2xsvf translator
        modifications.
    [Enhancement] Added XENDIR and XENDDR commands to support XPLA devices.
    [Enhancement] Re-wrote micro.c (and lenval.c) due to new TAP transition
        strategy and to cleanup old code.  (Ports.c remained the same.)
    [Enhancement] svf2xsvf -v2 option provides excellent backward compatibility
        for running new XSVF on old v2.00 XSVF player. Works for XC9500/XL
        devices.  For newer devices (XC18V00 and XPLA3), assumes XSDRTDO bug
        when XSDRSIZE==0 was fixed in old player.

v4.00 - v4.04 "beta":
    [New Device] Support XC18V00 and XPLA SVF without SirRunTest preprocessor.
    [New Device] Added XSTATE command to support CoolRunner devices.
    [Bug Fix] svf2xsvf translator fixed bug in which FPGA bitstream could
        be corrupted (by previous translators).
    [Enhancement] svf2xsvf translator re-written.  The single
        svf2xsvf translator performed function of previous svf2xsvf
        and xsvf2ascii.  That is, svf2xsvf output both the XSVF file
        as well as the ASCII text version.
    [Enhancement] svf2xsvf translator freed from shared library dependencies.
    [Enhancement] svf2xsvf translator improves translation speed for large
        FPGA SVF files by greater than 10x.
    [Change] svf2xsvf translator no longer supported XC9500's XSVF compression
        technique.  Use svf2xsvf v2.00 to compress XC9500 SVF files.

eisp_1800a ("alpha"):
    [New Device] Modified v2.00 solution to support XC18V00 and
        CoolRunner devices.  Used existing v2.00 svf2xsvf translator
        and supplemented with the SirRunTest preprocessor to prepare
        XC18V00/XPLA SVF files for the XSVF format.
    [New Device] XSIR command in XSVF player C source code modified to
        wait for pre-specified RUNTEST time after shifting if RUNTEST time was
        non-zero.  If RUNTEST time was zero, XSIR ended in Update-IR
        state so that next command would skip Run-Test/Idle on way to
        next shift state.  The wait was required for XC18V00 and XPLA
        devices.  The skip of Run-Test/Idle was required for the XPLA
        devices.
    [Bug Fix] Fixed bug in XSVF player C code so that an XSDR or XSDRTDO
        executed with a pre-specified XSDRSIZE value of zero skipped
        any kind of TAP transition and only waited for the pre-specified
        RUNTEST time in the Run-Test/Idle state.  The v2.00 translator
        performed this sequence when an arbitrary wait was required.
        But, the v2.00 XSVF player C source code errantly shifted one bit
        even though zero bits were specified.
    [Bug Fix] Fixed bug in which XSDRTDOC and XSDRTDOE called the function that
        performed the XSDRTDOB operation.

v2.00:
    [New Device] svf2xsvf, xsvf2ascii, and XSVF player C code updated to
        support FPGA bitstreams (i.e. chunking of long SDRs) in SVF.

pre-v2.00:
    [Feature] svf2xsvf translator converted SVF to binary XSVF format.
    [Feature] xsvf2ascii debugging translator converted binary XSVF format
        to human-readable ASCII text format.
    [Feature] XSVF player C code supported XC9500 and XC9500XL CPLDs.

