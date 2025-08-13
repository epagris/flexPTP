/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "flexPTP", "index.html", [
    [ "Introduction and overview", "index.html", "index" ],
    [ "Compiling the library", "building.html", [
      [ "Compiling", "building.html#autotoc_md0", [
        [ "CMake managed building", "building.html#autotoc_md1", [
          [ "Example configuration:", "building.html#autotoc_md2", null ]
        ] ],
        [ "IDE managed or manual building", "building.html#autotoc_md3", null ],
        [ "Precompiled library", "building.html#autotoc_md4", null ]
      ] ]
    ] ],
    [ "CLI interface", "cli.html", [
      [ "Command Line Interface", "cli.html#autotoc_md5", [
        [ "Registering commands", "cli.html#cli-registering-commands", [
          [ "Removing commands", "cli.html#autotoc_md6", null ]
        ] ]
      ] ]
    ] ],
    [ "Events and logging", "monitoring.html", [
      [ "Logging", "monitoring.html#autotoc_md33", [
        [ "Default logging", "monitoring.html#autotoc_md34", [
          [ "Slave clock mode", "monitoring.html#autotoc_md35", [
            [ "Example", "monitoring.html#autotoc_md36", null ]
          ] ],
          [ "Master clock mode", "monitoring.html#autotoc_md37", [
            [ "Example", "monitoring.html#autotoc_md38", null ]
          ] ]
        ] ],
        [ "BMCA state transition logging", "monitoring.html#autotoc_md39", [
          [ "Master and Slave modes", "monitoring.html#autotoc_md40", [
            [ "Example:", "monitoring.html#autotoc_md41", null ]
          ] ]
        ] ],
        [ "Timestamp logging", "monitoring.html#autotoc_md42", [
          [ "Slave mode", "monitoring.html#autotoc_md43", [
            [ "E2E mode", "monitoring.html#autotoc_md44", null ],
            [ "P2P mode", "monitoring.html#autotoc_md45", null ]
          ] ],
          [ "Master P2P mode", "monitoring.html#autotoc_md46", null ]
        ] ],
        [ "Correction fields logging", "monitoring.html#autotoc_md47", [
          [ "Slave mode", "monitoring.html#autotoc_md48", [
            [ "Example", "monitoring.html#autotoc_md49", null ]
          ] ]
        ] ],
        [ "Info notifications", "monitoring.html#autotoc_md50", [
          [ "Slave mode", "monitoring.html#autotoc_md51", [
            [ "Example", "monitoring.html#autotoc_md52", null ]
          ] ]
        ] ],
        [ "Clock has locked in our out notification", "monitoring.html#autotoc_md53", [
          [ "Slave mode", "monitoring.html#autotoc_md54", [
            [ "Example", "monitoring.html#autotoc_md55", null ]
          ] ]
        ] ],
        [ "Transmit message management logging", "monitoring.html#autotoc_md56", null ]
      ] ],
      [ "User events", "monitoring.html#autotoc_md58", null ],
      [ "Statistics", "monitoring.html#autotoc_md59", null ]
    ] ],
    [ "Porting and configuration", "porting.html", [
      [ "Porting", "porting.html#port-config-port", [
        [ "Port option file", "porting.html#autotoc_md60", [
          [ "Example option files", "porting.html#autotoc_md61", null ]
        ] ],
        [ "Network Stack Driver (NSD)", "porting.html#network-stack-driver", [
          [ "NSD examples", "porting.html#autotoc_md62", null ]
        ] ],
        [ "Hardware Clock Driver", "porting.html#hardware-clock-driver", [
          [ "ADDEND-interface", "porting.html#porting-addend-interface", [
            [ "Example option files", "porting.html#autotoc_md63", null ],
            [ "Example Hardware Clock Driver files", "porting.html#autotoc_md64", null ]
          ] ],
          [ "High-Level Tuning (HLT) interface", "porting.html#porting-hlt-interface", [
            [ "Example option files", "porting.html#autotoc_md65", null ],
            [ "Example Hardware Clock Driver files", "porting.html#autotoc_md66", null ]
          ] ]
        ] ]
      ] ],
      [ "Configuration", "porting.html#port-config-configuration", [
        [ "Initial configuration", "porting.html#autotoc_md67", [
          [ "Operation mode parameters", "porting.html#port-config-opmode-params", null ],
          [ "PTP definitions", "porting.html#autotoc_md68", [
            [ "Common", "porting.html#autotoc_md69", null ],
            [ "BMCA", "porting.html#port-config-BMCA", null ],
            [ "Master", "porting.html#port-config-master", null ],
            [ "Clock dataset", "porting.html#port-config-clock-dataset", null ]
          ] ]
        ] ],
        [ "Runtime configuration", "porting.html#autotoc_md70", null ]
      ] ]
    ] ],
    [ "Project organization", "project_organization.html", [
      [ "Modules", "project_organization.html#autotoc_md71", [
        [ "Types, constants and definitions", "project_organization.html#autotoc_md72", null ],
        [ "Settings", "project_organization.html#autotoc_md73", null ],
        [ "Logging and statistics", "project_organization.html#autotoc_md74", null ],
        [ "Utilities", "project_organization.html#autotoc_md75", null ],
        [ "Messaging", "project_organization.html#autotoc_md76", null ],
        [ "Events", "project_organization.html#autotoc_md77", null ],
        [ "Core:", "project_organization.html#autotoc_md78", null ]
      ] ]
    ] ],
    [ "Clock servo", "servo.html", [
      [ "Clock servo", "servo.html#autotoc_md79", [
        [ "Interface", "servo.html#autotoc_md80", null ],
        [ "Bundled controllers", "servo.html#autotoc_md81", [
          [ "PID-controller", "servo.html#autotoc_md82", [
            [ "CLI commands", "servo.html#autotoc_md83", null ],
            [ "Example usage definitions", "servo.html#autotoc_md84", null ]
          ] ],
          [ "Kalman-filter", "servo.html#autotoc_md85", [
            [ "CLI commands", "servo.html#autotoc_md86", null ],
            [ "Example usage definitions", "servo.html#autotoc_md87", null ]
          ] ],
          [ "Debug servo", "servo.html#autotoc_md88", [
            [ "CLI commands", "servo.html#autotoc_md89", null ],
            [ "Example usage definitions", "servo.html#autotoc_md90", null ]
          ] ]
        ] ]
      ] ]
    ] ],
    [ "Software structure", "swoperation.html", [
      [ "Internal modules", "swoperation.html#autotoc_md91", [
        [ "Core", "swoperation.html#core", [
          [ "Initialization", "swoperation.html#autotoc_md92", null ],
          [ "Reset", "swoperation.html#autotoc_md93", null ]
        ] ],
        [ "Best Master Clock Algorithm", "swoperation.html#best-master-clock-algorithm", null ],
        [ "Master", "swoperation.html#master", [
          [ "Compliant peer", "swoperation.html#autotoc_md94", null ],
          [ "P2P Mean Path Delay", "swoperation.html#autotoc_md95", [
            [ "Two-step clock mode", "swoperation.html#autotoc_md96", null ],
            [ "One-step clock mode", "swoperation.html#autotoc_md97", null ]
          ] ]
        ] ],
        [ "Slave", "swoperation.html#slave", [
          [ "Time error", "swoperation.html#autotoc_md98", null ],
          [ "Mean Path Delay", "swoperation.html#autotoc_md99", [
            [ "Two-step clock mode", "swoperation.html#autotoc_md100", null ],
            [ "One-step clock mode", "swoperation.html#autotoc_md101", null ]
          ] ]
        ] ]
      ] ],
      [ "External modules", "swoperation.html#external-modules", null ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "Globals", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", "globals_func" ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", "globals_defs" ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_h743_ethernetif_modifications.html",
"flexptp__options__stm32h743_8h.html#a6d05c58b62fafdaf1d107abfc39ab8cf",
"minmax_8h.html#afa99ec4acc4ecb2dc3c2d05da15d0e3f",
"project_organization.html#autotoc_md74",
"ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a798249c5983d93b06e690ab45e58264c",
"struct_ptp_announce_body.html#a87e18f3390e8f5d489c76a197fe29966",
"task__ptp_8h.html#a6d8d64d0927e028889b55b6bff4c7d57"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';