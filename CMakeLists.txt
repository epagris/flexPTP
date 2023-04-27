set(FLEXPTP_SRC
        # core files
        cli_cmds.c
        cli_cmds.h
        clock_utils.c
        clock_utils.h
        format_utils.c
        format_utils.h
        logging.c
        logging.h
        msg_utils.c
        msg_utils.h
        ptp_core.c
        ptp_core.h
        sbmc.c
        sbmc.h
        stats.c
        stats.h
        timeutils.c
        timeutils.h
        ptp_defs.h
        ptp_types.h

        # servo files
        servo/pd_controller.c
        servo/pd_controller.h

        # hw-port files
        hw_port/flexptp_options_simulation.h
        hw_port/ptp_port_simulation.cpp
        hw_port/ptp_port_simulation.h

        # simulation objects
        hw_port/simsrc/lwip_simulation.h
        hw_port/simsrc/lwip_simulation.cpp
        hw_port/simsrc/FreeRTOS_simulation.h
        hw_port/simsrc/FreeRTOS_simulation.cpp
        hw_port/simsrc/PtpClock.h
        hw_port/simsrc/PtpClock.cpp
        hw_port/simsrc/PtpMasterClock.h
        hw_port/simsrc/PtpMasterClock.cpp
        hw_port/simsrc/PtpSlaveClock.h
        hw_port/simsrc/PtpSlaveClock.cpp

        PARENT_SCOPE)