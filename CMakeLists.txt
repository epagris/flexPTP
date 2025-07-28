cmake_minimum_required(VERSION 3.15)

set(FLEXPTP_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src/flexptp)

# Hardware port selection
set(FLEXPTP_HWPORT_OK 1)

if (FLEXPTP_HWPORT MATCHES "^H74[35]_LWIP$")
    set(FLEXPTP_HWPORT_SRC
        port/example_ports/ptp_port_stm32h743_lwip.c
        port/example_ports/ptp_port_stm32h743_lwip.h
    )
    set(FLEXPTP_HWPORT_BASE "H743_LWIP")
elseif(FLEXPTP_HWPORT MATCHES "^H74[35]_ETHERLIB$")
    set(FLEXPTP_HWPORT_SRC
        port/example_ports/ptp_port_stm32h743_etherlib.c
        port/example_ports/ptp_port_stm32h743_etherlib.h
    ) 
    set(FLEXPTP_HWPORT_BASE "H743_ETHERLIB")
elseif(FLEXPTP_HWPORT MATCHES "^F[47][0-9][0-9]_LWIP$") # STM32F4xx
    set(FLEXPTP_HWPORT_SRC
        port/example_ports/ptp_port_stm32f407_lwip.c
        port/example_ports/ptp_port_stm32f407_lwip.h
    )
    set(FLEXPTP_HWPORT_BASE "F407_LWIP")
elseif(FLEXPTP_HWPORT MATCHES "^F[47][0-9][0-9]_ETHERLIB$")
    set(FLEXPTP_HWPORT_SRC
        port/example_ports/ptp_port_stm32f407_etherlib.c
        port/example_ports/ptp_port_stm32f407_etherlib.h
    ) 
    set(FLEXPTP_HWPORT_BASE "F407_ETHERLIB")
elseif(FLEXPTP_HWPORT STREQUAL "CH32F207_ETHERLIB")
    set(FLEXPTP_HWPORT_SRC
        port/example_ports/ptp_port_ch32f207_etherlib.c
        port/example_ports/ptp_port_ch32f207_etherlib.h
    ) 
    set(FLEXPTP_HWPORT_BASE "CH32F207_ETHERLIB")
elseif(FLEXPTP_HWPORT STREQUAL "TM4C1294_LWIP")
    set(FLEXPTP_HWPORT_SRC
        port/example_ports/ptp_port_tiva_tm4c1294.c
        port/example_ports/ptp_port_tiva_tm4c1294.h
    ) 
    set(FLEXPTP_HWPORT_BASE "TM4C1294_LWIP")
else()
    set(FLEXPTP_HWPORT_OK 0) 
endif()

if (FLEXPTP_HWPORT_OK)
    message("flexPTP: '" ${FLEXPTP_HWPORT} "' hardware port selected")

    if (NOT FLEXPTP_HWPORT STREQUAL FLEXPTP_HWPORT_BASE)
        message("flexPTP: hardware port is automatically derived from '${FLEXPTP_HWPORT_BASE}'")
    endif()

    list(TRANSFORM FLEXPTP_HWPORT_SRC PREPEND "${FLEXPTP_SRC_DIR}/")
else()
    if (FLEXPTP_HWPORT_SRC)
        message("flexPTP: custom hardware port defined")
    else()
        message("flexPTP: Unknown or undefined hardware port: '" ${FLEXPTP_HWPORT} "', "
                "please populate FLEXPTP_HWPORT_SRC with the files of your custom "
                "hardware port or pick a predefined one!")
    endif()
endif()

# Selecting Network Stack Driver
set(FLEXPTP_NSD_OK 1)

if (FLEXPTP_NSD STREQUAL "LWIP")
    set(FLEXPTP_NSD_SRC
        port/example_netstack_drivers/nsd_lwip.c
    )
    set(FLEXPTP_NETWORK_STACK lwipcore)
elseif(FLEXPTP_NSD STREQUAL "ETHERLIB")
    set(FLEXPTP_NSD_SRC
        port/example_netstack_drivers/nsd_etherlib.c
    )
    set(FLEXPTP_NETWORK_STACK etherlib)
else()
    set(FLEXPTP_NSD_OK 0)
endif()

if (FLEXPTP_NSD_OK)
    message("flexPTP: '" ${FLEXPTP_NSD} "' network stack driver selected")
    list(TRANSFORM FLEXPTP_NSD_SRC PREPEND "${FLEXPTP_SRC_DIR}/")
else()
    if (FLEXPTP_NSD_SRC)
        message("flexPTP: custom network stack driver defined")
    else()
        message("flexPTP: Unknown or undefined network stack driver: '" ${FLEXPTP_NSD} "', "
                "please populate FLEXPTP_NSD_SRC with the files of your custom network stack "
                "driver and FLEXPTP_NETWORK_STACK with the name of the custom network stack "
                "library or pick a predefined one!")
    endif()
endif()

# Selecting the Servo
set(FLEXPTP_SERVO_OK 1)

if (FLEXPTP_SERVO STREQUAL "PID")
    set(FLEXPTP_SERVO_SRC
        servo/pid_controller.c
        servo/pid_controller.h)
elseif (FLEXPTP_SERVO STREQUAL "KALMAN")
    set(FLEXPTP_SERVO_SRC
        servo/kalman_filter.c
        servo/kalman_filter.h)        
elseif (FLEXPTP_SERVO STREQUAL "DEBUG")
    set(FLEXPTP_SERVO_SRC
        servo/debug_servo.c
        servo/debug_servo.h)   
else()
    set(FLEXPTP_SERVO_OK 0)
endif()

if (FLEXPTP_SERVO_OK)
    message("flexPTP: '" ${FLEXPTP_SERVO} "' clock servo selected")
    list(TRANSFORM FLEXPTP_SERVO_SRC PREPEND "${FLEXPTP_SRC_DIR}/")
else()
    if (NOT FLEXPTP_SERVO_SRC)
        message("flexPTP: WARNING! No clock servo defined! If this is a mistake, then "
                "populate FLEXPTP_SERVO_SRC with the list of the clock servo module.")
    endif()
endif()

# Customizing compile target
set(FLEXPTP_TARGET flexptp)

if (FLEXPTP_TARGET_TAG)
    set(FLEXPTP_TARGET "${FLEXPTP_TARGET}_${FLEXPTP_TARGET_TAG}")
    message("Custom flexPTP target: ${FLEXPTP_TARGET}")
endif()

set(FLEXPTP_SRC    
    cli_cmds.c
    cli_cmds.h
    clock_utils.c
    clock_utils.h
    common.c
    common.h
    config.c
    config.h
    event.c
    event.h
    format_utils.c
    format_utils.h
    logging.c
    logging.h
    master.c
    master.h
    msg_utils.c
    msg_utils.h
    network_stack_driver.h
    profiles.c
    profiles.h
    ptp_core.c
    ptp_core.h
    ptp_defs.c
    ptp_defs.h
    ptp_profile_presets.c
    ptp_profile_presets.h
    ptp_raw_msg_circbuf.c
    ptp_raw_msg_circbuf.h
    ptp_servo_types.h
    ptp_sync_cycle_data.h
    ptp_types.h
    bmca.c
    bmca.h
    servo
    settings_interface.c
    settings_interface.h
    slave.c
    slave.h
    stats.c
    stats.h
    task_ptp.c
    task_ptp.h
    timeutils.c
    timeutils.h
    tlv.c
    tlv.h
)
list(TRANSFORM FLEXPTP_SRC PREPEND "${FLEXPTP_SRC_DIR}/")
list(APPEND FLEXPTP_SRC ${FLEXPTP_HWPORT_SRC} ${FLEXPTP_NSD_SRC} ${FLEXPTP_SERVO_SRC})

set(FLEXPTP_LOCAL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/src)
set(FLEXPTP_INCLUDE_EXPORT ${FLEXPTP_LOCAL_INCLUDES} PARENT_SCOPE) # Export the location of the flexPTP headers

add_library(${FLEXPTP_TARGET} STATIC ${FLEXPTP_SRC})
target_include_directories(${FLEXPTP_TARGET} PRIVATE ${FLEXPTP_INCLUDES} ${FLEXPTP_LOCAL_INCLUDES})
target_compile_options(${FLEXPTP_TARGET} PRIVATE ${FLEXPTP_CPU_PARAMS})
target_compile_definitions(${FLEXPTP_TARGET} PRIVATE ${FLEXPTP_COMPILE_DEFS})

if (FLEXPTP_NETWORK_STACK)
    message("flexPTP: linking against '" ${FLEXPTP_NETWORK_STACK} "' network library")
    if (NOT FLEXPTP_DONT_LINK_NETWORK_LIB)
        target_link_libraries(${FLEXPTP_TARGET} ${FLEXPTP_NETWORK_STACK})
    endif()
elseif()
    message("flexPTP: No network stack defined to link against! "
            "Please set FLEXPTP_NETWORK_STACK to a chosen network stack library!")
endif()

add_custom_target(
    flexptp-docs
    COMMAND doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
