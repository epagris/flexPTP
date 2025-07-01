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
      [ "Command Line Interface", "cli.html#autotoc_md5", null ]
    ] ],
    [ "Events and logging", "monitoring.html", [
      [ "Logging", "monitoring.html#autotoc_md32", [
        [ "Default logging", "monitoring.html#autotoc_md33", [
          [ "Slave clock mode", "monitoring.html#autotoc_md34", [
            [ "Example", "monitoring.html#autotoc_md35", null ]
          ] ],
          [ "Master clock mode", "monitoring.html#autotoc_md36", [
            [ "Example", "monitoring.html#autotoc_md37", null ]
          ] ]
        ] ],
        [ "BMCA state transition logging", "monitoring.html#autotoc_md38", [
          [ "Master and Slave modes", "monitoring.html#autotoc_md39", [
            [ "Example:", "monitoring.html#autotoc_md40", null ]
          ] ]
        ] ],
        [ "Timestamp logging", "monitoring.html#autotoc_md41", [
          [ "Slave mode", "monitoring.html#autotoc_md42", [
            [ "E2E mode", "monitoring.html#autotoc_md43", null ],
            [ "P2P mode", "monitoring.html#autotoc_md44", null ]
          ] ],
          [ "Master P2P mode", "monitoring.html#autotoc_md45", null ]
        ] ],
        [ "Correction fields logging", "monitoring.html#autotoc_md46", [
          [ "Slave mode", "monitoring.html#autotoc_md47", [
            [ "Example", "monitoring.html#autotoc_md48", null ]
          ] ]
        ] ],
        [ "Info notifications", "monitoring.html#autotoc_md49", [
          [ "Slave mode", "monitoring.html#autotoc_md50", [
            [ "Example", "monitoring.html#autotoc_md51", null ]
          ] ]
        ] ],
        [ "Clock has locked in our out notification", "monitoring.html#autotoc_md52", [
          [ "Slave mode", "monitoring.html#autotoc_md53", [
            [ "Example", "monitoring.html#autotoc_md54", null ]
          ] ]
        ] ]
      ] ],
      [ "User events", "monitoring.html#autotoc_md55", null ],
      [ "Statistics", "monitoring.html#autotoc_md56", null ]
    ] ],
    [ "Porting and configuration", "porting.html", [
      [ "Porting", "porting.html#port-config-port", [
        [ "Port option file", "porting.html#autotoc_md57", [
          [ "Example option files", "porting.html#autotoc_md58", null ]
        ] ],
        [ "Network Stack Driver (NSD)", "porting.html#network-stack-driver", [
          [ "NSD examples", "porting.html#autotoc_md59", null ]
        ] ],
        [ "Hardware Clock Driver", "porting.html#hardware-clock-driver", null ],
        [ "Example Hardware Clock Driver files", "porting.html#autotoc_md60", null ]
      ] ],
      [ "Configuration", "porting.html#port-config-configuration", [
        [ "Initial configuration", "porting.html#autotoc_md61", [
          [ "Operation mode parameters", "porting.html#port-config-opmode-params", null ],
          [ "PTP definitions", "porting.html#autotoc_md62", [
            [ "Common", "porting.html#autotoc_md63", null ],
            [ "BMCA", "porting.html#port-config-BMCA", null ],
            [ "Master", "porting.html#port-config-master", null ],
            [ "Clock dataset", "porting.html#port-config-clock-dataset", null ]
          ] ]
        ] ],
        [ "Runtime configuration", "porting.html#autotoc_md64", null ]
      ] ]
    ] ],
    [ "Project organization", "project_organization.html", [
      [ "Modules", "project_organization.html#autotoc_md65", [
        [ "Types, constants and definitions", "project_organization.html#autotoc_md66", null ],
        [ "Settings", "project_organization.html#autotoc_md67", null ],
        [ "Logging and statistics", "project_organization.html#autotoc_md68", null ],
        [ "Utilities", "project_organization.html#autotoc_md69", null ],
        [ "Messaging", "project_organization.html#autotoc_md70", null ],
        [ "Events", "project_organization.html#autotoc_md71", null ],
        [ "Core:", "project_organization.html#autotoc_md72", null ]
      ] ]
    ] ],
    [ "Clock servo", "servo.html", [
      [ "Clock servo", "servo.html#autotoc_md73", [
        [ "Examples", "servo.html#autotoc_md74", null ]
      ] ]
    ] ],
    [ "Software structure", "swoperation.html", [
      [ "Internal modules", "swoperation.html#autotoc_md75", [
        [ "Core", "swoperation.html#core", [
          [ "Initialization", "swoperation.html#autotoc_md76", null ],
          [ "Reset", "swoperation.html#autotoc_md77", null ]
        ] ],
        [ "Best Master Clock Algorithm", "swoperation.html#best-master-clock-algorithm", null ],
        [ "Master", "swoperation.html#master", [
          [ "Compliant peer", "swoperation.html#autotoc_md78", null ],
          [ "P2P Mean Path Delay", "swoperation.html#autotoc_md79", [
            [ "Two-step clock mode", "swoperation.html#autotoc_md80", null ],
            [ "One-step clock mode", "swoperation.html#autotoc_md81", null ]
          ] ]
        ] ],
        [ "Slave", "swoperation.html#slave", [
          [ "Time error", "swoperation.html#autotoc_md82", null ],
          [ "Mean Path Delay", "swoperation.html#autotoc_md83", [
            [ "Two-step clock mode", "swoperation.html#autotoc_md84", null ],
            [ "One-step clock mode", "swoperation.html#autotoc_md85", null ]
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
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_h743_ethernetif_modifications.html",
"globals_func_c.html",
"ptp__core_8c.html#a52febb700b5a0f2af1bc5f31d43593ce",
"ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbcacbc42698eab7e532b20d2e2e8b5dbc7d",
"struct_ptp_core_state.html#a735f4c30ed85b9a77287c63453eefcac"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';