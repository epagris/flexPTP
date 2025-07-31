var ptp__types_8h =
[
    [ "PtpFlags", "struct_ptp_flags.html", "struct_ptp_flags" ],
    [ "PtpHeader", "struct_ptp_header.html", "struct_ptp_header" ],
    [ "PtpDelay_RespIdentification", "struct_ptp_delay___resp_identification.html", "struct_ptp_delay___resp_identification" ],
    [ "RawPtpMessage", "struct_raw_ptp_message.html", "struct_raw_ptp_message" ],
    [ "PtpAnnounceBody", "struct_ptp_announce_body.html", "struct_ptp_announce_body" ],
    [ "PtpBmcaState", "struct_ptp_bmca_state.html", "struct_ptp_bmca_state" ],
    [ "PtpProfileTlvElement", "struct_ptp_profile_tlv_element.html", "struct_ptp_profile_tlv_element" ],
    [ "PtpProfile", "struct_ptp_profile.html", "struct_ptp_profile" ],
    [ "PtpTlvHeader", "struct_ptp_tlv_header.html", "struct_ptp_tlv_header" ],
    [ "PtpSlaveMessagingState", "struct_ptp_slave_messaging_state.html", "struct_ptp_slave_messaging_state" ],
    [ "PtpMasterMessagingState", "struct_ptp_master_messaging_state.html", "struct_ptp_master_messaging_state" ],
    [ "PtpP2PSlaveInfo", "struct_ptp_p2_p_slave_info.html", "struct_ptp_p2_p_slave_info" ],
    [ "PtpHWClockState", "struct_ptp_h_w_clock_state.html", "struct_ptp_h_w_clock_state" ],
    [ "PtpNetworkState", "struct_ptp_network_state.html", "struct_ptp_network_state" ],
    [ "PtpStats", "struct_ptp_stats.html", "struct_ptp_stats" ],
    [ "PtpCoreState", "struct_ptp_core_state.html", "struct_ptp_core_state" ],
    [ "FLEXPTP_FREERTOS", "ptp__types_8h.html#ad68f01211bcc2a5342a9d23ecfdcaa3b", null ],
    [ "MAX_PTP_MSG_SIZE", "ptp__types_8h.html#ab9a5b1004564a82c7f0aab1069fcc2ba", null ],
    [ "PTP_TLV_HEADER", "ptp__types_8h.html#a7f9a1eafe20e661bf08b831ea74a4f99", null ],
    [ "PTP_VARIANCE_HAS_NOT_BEEN_COMPUTED", "ptp__types_8h.html#ac8bbf4f548e8bad3306b48543d16b6b9", null ],
    [ "PtpMasterProperties", "ptp__types_8h.html#a8a829d9118e4062f301ae8539d9134ad", null ],
    [ "PtpSyncCallback", "ptp__types_8h.html#a46935577d0036742a30c9042d42a519e", null ],
    [ "PtpUserEventCallback", "ptp__types_8h.html#a45cabd2ea4c608b4b84ce719019cd8e0", null ],
    [ "TimerType", "ptp__types_8h.html#a5a0e9e1971b37b7cf635ab8b75d03da6", null ],
    [ "BmcaCandidateState", "ptp__types_8h.html#a0f2deb690644fe2c5751684018fe84a6", [
      [ "BMCA_NO_CANDIDATE", "ptp__types_8h.html#a0f2deb690644fe2c5751684018fe84a6aa4dd92f7040358db83f64cff994d1524", null ],
      [ "BMCA_CANDIDATE_COLLECTION", "ptp__types_8h.html#a0f2deb690644fe2c5751684018fe84a6a247f93ab5be884d4744b0dd251dbd809", null ]
    ] ],
    [ "BmcaMasterState", "ptp__types_8h.html#ad514a688f783ca8534e1a2c2aef3cbec", [
      [ "BMCA_NO_MASTER", "ptp__types_8h.html#ad514a688f783ca8534e1a2c2aef3cbeca51c419d06739f89ec4ba60085f046a5a", null ],
      [ "BMCA_MASTER_REMOTE", "ptp__types_8h.html#ad514a688f783ca8534e1a2c2aef3cbecac5af3fa9c1592495726792d233c25ad7", null ],
      [ "BMCA_MASTER_ME", "ptp__types_8h.html#ad514a688f783ca8534e1a2c2aef3cbeca1be4fb8f5eff4dd536ef8280ea38c8e5", null ]
    ] ],
    [ "PtpBmcaFsmState", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccb", [
      [ "PTP_BMCA_INITIALIZING", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba8c969ae4e7b20149960de517fa64583b", null ],
      [ "PTP_BMCA_LISTENING", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba882b4ccc50bf493dae35fae6e1f3c813", null ],
      [ "PTP_BMCA_PRE_MASTER", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccbabde0249f5c5f5eca979237038afdd64f", null ],
      [ "PTP_BMCA_MASTER", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba7bcc2a3a617a4f945becfea7ebd35668", null ],
      [ "PTP_BMCA_SLAVE", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba8941f37d6a3073e35c34ff0668e16c29", null ],
      [ "PTP_BMCA_PASSIVE", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba86114209d02bbe67a354d828a408c052", null ],
      [ "PTP_BMCA_UNCALIBRATED", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba1803ee52ab8c65716318aa57b402c307", null ],
      [ "PTP_BMCA_FAULTY", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba06ebe65ba202bfa2e3cdc0b9c24ce979", null ],
      [ "PTP_BMCA_DISABLED", "ptp__types_8h.html#a53e41c362b71e8f5e6bf2489fd9f5ccba95628fca3861bc5fe4738386a5429ad2", null ]
    ] ],
    [ "PtpClockAccuracy", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fc", [
      [ "PTP_CA_25NS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcaa9cc9702bd9583aebbc784f318a8039b", null ],
      [ "PTP_CA_100NS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcae90a05bf0a0c04ea931df69b52b86172", null ],
      [ "PTP_CA_250NS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca644e3b448472b4eeffe2236ad817de3b", null ],
      [ "PTP_CA_1US", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcab8af247337ec04d28bbd6140d2f2a1b0", null ],
      [ "PTP_CA_2_5US", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca2b925c97d95d3da199a480b2e22099bf", null ],
      [ "PTP_CA_10US", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcad55b156a13cd27827d5ed3f3d1697c46", null ],
      [ "PTP_CA_25US", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcac9c2448fd580cd536af77d47f54656a8", null ],
      [ "PTP_CA_100US", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca5dcf8e7a76acd8ccde9fd76ab4fa80b4", null ],
      [ "PTP_CA_250US", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca9b6c64e15454af8b1fae399f04777f50", null ],
      [ "PTP_CA_1MS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca9031d820229d3719a26b1926610986b8", null ],
      [ "PTP_CA_2_5MS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcaf47f3c138ec093d7cf1008b786e2b20d", null ],
      [ "PTP_CA_10MS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcafdb08652cd00cc835c64c629b730ae50", null ],
      [ "PTP_CA_25MS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcaef2bd13252e68ee453f73393fc66dd8b", null ],
      [ "PTP_CA_100MS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca93b602a7c7da0c1e9eced42ed6e8f28f", null ],
      [ "PTP_CA_250MS", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca53db54e9832b405a2dc73fc863e7095d", null ],
      [ "PTP_CA_1S", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fcaef3658e6c4630d795ebaa5c9cb74ea75", null ],
      [ "PTP_CA_10S", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca685536d59ee393be0ae033981be3fb63", null ],
      [ "PTP_CA_GT10S", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca593f4751ecb9d2d0661b55cc46978d25", null ],
      [ "PTP_CA_UNKNOWN", "ptp__types_8h.html#a94931f16cbb6ca5dcb74c0e3d15ba9fca9130e22a503c82458eca61d2e7a20baa", null ]
    ] ],
    [ "PtpClockClass", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9", [
      [ "PTP_CC_PRIMARY_REFERENCE", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a75a29fbc2c262999b328ccc23da863da", null ],
      [ "PTP_CC_PRIMARY_REFERENCE_HOLDOVER", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a63fc0a9dfe91f349b13b63e0d0637c76", null ],
      [ "PTP_CC_APPLICATION_SPECIFIC", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a78eb08a9b46ad8ed918908ee797daf33", null ],
      [ "PTP_CC_APPLICAION_SPECIFIC_HOLDOVER", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9aa21795d295ed725597ae68ee83f3e0c7", null ],
      [ "PTP_CC_PRIMARY_REFERENCE_DEGRAD_A", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9aba7528747d0c12cb17ae1ac339298f87", null ],
      [ "PTP_CC_APPLICATION_SPECIFIC_DEGRAD_A", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a2fa761cde578d20e1d9bdaf12d9bacff", null ],
      [ "PTP_CC_PRIMARY_REFERENCE_DEGRAD_B", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a15c8dbf9df44c1acaa21b0e63e0c86e3", null ],
      [ "PTP_CC_APPLICATION_SPECIFIC_DEGRAD_B", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a5eb08d6f09faebdbe947dd7ae3341961", null ],
      [ "PTP_CC_DEFAULT", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a37c25e2f0f3aa468f376e8afb606de06", null ],
      [ "PTP_CC_SLAVE_ONLY", "ptp__types_8h.html#a5af6e9a813f31df8ce81457a72a802c9a798249c5983d93b06e690ab45e58264c", null ]
    ] ],
    [ "PtpControl", "ptp__types_8h.html#a150b207b681c01a6befbe53e941be6aa", [
      [ "PTP_CON_Sync", "ptp__types_8h.html#a150b207b681c01a6befbe53e941be6aaad36dca818d57748d272ee00e82f29072", null ],
      [ "PTP_CON_Delay_Req", "ptp__types_8h.html#a150b207b681c01a6befbe53e941be6aaa4f5002b8091c67ce622b1df0e79e0362", null ],
      [ "PTP_CON_Follow_Up", "ptp__types_8h.html#a150b207b681c01a6befbe53e941be6aaa75231573238596e00fccde414245bf2c", null ],
      [ "PTP_CON_Delay_Resp", "ptp__types_8h.html#a150b207b681c01a6befbe53e941be6aaa958e6a2751a6051e4f8b7bf20f8fee69", null ],
      [ "PTP_CON_Other", "ptp__types_8h.html#a150b207b681c01a6befbe53e941be6aaa2743d542618a92166b40492bdb1250f3", null ]
    ] ],
    [ "PtpDelayMechanism", "ptp__types_8h.html#aa353086afe318bec4ec7ce3757ba478e", [
      [ "PTP_DM_E2E", "ptp__types_8h.html#aa353086afe318bec4ec7ce3757ba478ea158f3d6c09c384726ffa8a36562cde6a", null ],
      [ "PTP_DM_P2P", "ptp__types_8h.html#aa353086afe318bec4ec7ce3757ba478ea32cfbd5559afa86a42b55676491a024f", null ]
    ] ],
    [ "PtpFastCompState", "ptp__types_8h.html#a8eb694373758507149a9b074ec1003b1", [
      [ "PTP_FC_IDLE", "ptp__types_8h.html#a8eb694373758507149a9b074ec1003b1ab2f800f98b76254fc79e5866e4bf4a6e", null ],
      [ "PTP_FC_SKEW_CORRECTION", "ptp__types_8h.html#a8eb694373758507149a9b074ec1003b1a6607bdd6c525537c064efd59ce008086", null ],
      [ "PTP_FC_TIME_CORRECTION", "ptp__types_8h.html#a8eb694373758507149a9b074ec1003b1a95416ba42cfb4a0d04fa3264b6fb162c", null ],
      [ "PTP_FC_TIME_CORRECTION_PROPAGATION", "ptp__types_8h.html#a8eb694373758507149a9b074ec1003b1a0b73474d2ca31643656984a277fb2315", null ]
    ] ],
    [ "PtpLogMsgPeriods", "ptp__types_8h.html#a3f2d7e7b45322fb28732f5734365b2d3", [
      [ "PTP_LOGPER_MIN", "ptp__types_8h.html#a3f2d7e7b45322fb28732f5734365b2d3a34f4ccd3f633f863e6972c5a00859827", null ],
      [ "PTP_LOGPER_MAX", "ptp__types_8h.html#a3f2d7e7b45322fb28732f5734365b2d3a2a570cf4d31047f27c5a28ed5de85b73", null ],
      [ "PTP_LOGPER_SYNCMATCHED", "ptp__types_8h.html#a3f2d7e7b45322fb28732f5734365b2d3a4c6adfdd5b2e49e66f1db8ff5457e062", null ]
    ] ],
    [ "PtpM2SState", "ptp__types_8h.html#a9ca78918bde2b70e797a084a284e164a", [
      [ "SIdle", "ptp__types_8h.html#a9ca78918bde2b70e797a084a284e164aac813365626eb29ef2ed641cc916cd5e0", null ],
      [ "SWaitFollowUp", "ptp__types_8h.html#a9ca78918bde2b70e797a084a284e164aa6a8826fafe0141039045606786a79fd9", null ]
    ] ],
    [ "PtpMessageClass", "ptp__types_8h.html#a8dd57af72bd46fa7defd2f1b0f692b4b", [
      [ "PTP_MC_EVENT", "ptp__types_8h.html#a8dd57af72bd46fa7defd2f1b0f692b4ba6ae20b4d1987c3bd3fea6ac1ab16db8d", null ],
      [ "PTP_MC_GENERAL", "ptp__types_8h.html#a8dd57af72bd46fa7defd2f1b0f692b4ba700de684dc15c347968e9d23e82cbdc6", null ]
    ] ],
    [ "PtpMessageType", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69", [
      [ "PTP_MT_Sync", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69a6feee8b3789134eb72cad3727b166f74", null ],
      [ "PTP_MT_Delay_Req", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69ad428bc4cb27a61e8030bf8655dd47273", null ],
      [ "PTP_MT_PDelay_Req", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69a9c22d868b744ccaaf4288191735aa799", null ],
      [ "PTP_MT_PDelay_Resp", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69a37e2397636083f1020ffab04704ec09d", null ],
      [ "PTP_MT_Follow_Up", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69ae4993ae6053d06eef13cd7fd38803d59", null ],
      [ "PTP_MT_Delay_Resp", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69a04018aa95935273405f33a2eaf4aa3b3", null ],
      [ "PTP_MT_PDelay_Resp_Follow_Up", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69ae0b2431c7a019a9b4900c2532d6cec0a", null ],
      [ "PTP_MT_Announce", "ptp__types_8h.html#a9e053fb4748c174bc1e870145f65db69acd68e3340fc22ee0407ba8943359b98d", null ]
    ] ],
    [ "PtpP2PSlaveState", "ptp__types_8h.html#ae4104fe8cfc83cb1b1acad40e0c7efb2", [
      [ "PTP_P2PSS_NONE", "ptp__types_8h.html#ae4104fe8cfc83cb1b1acad40e0c7efb2aa2319b51b4047211f4918b0e6dd3196f", null ],
      [ "PTP_P2PSS_CANDIDATE", "ptp__types_8h.html#ae4104fe8cfc83cb1b1acad40e0c7efb2ae81b7aca202cbfbe0d6c2613d7b2b028", null ],
      [ "PTP_P2PSS_ESTABLISHED", "ptp__types_8h.html#ae4104fe8cfc83cb1b1acad40e0c7efb2a33563e2e212a8e184da4c2c9c6a83497", null ]
    ] ],
    [ "PtpProfileFlags", "ptp__types_8h.html#af7660187be027791a7264a12085fb338", [
      [ "PTP_PF_NONE", "ptp__types_8h.html#af7660187be027791a7264a12085fb338a35011cef2115a8cc4b0e018685956b78", null ],
      [ "PTP_PF_ISSUE_SYNC_FOR_COMPLIANT_SLAVE_ONLY_IN_P2P", "ptp__types_8h.html#af7660187be027791a7264a12085fb338a6cf2c9a3ea196c8958967b1bfffb2d6b", null ],
      [ "PTP_PF_SLAVE_ONLY", "ptp__types_8h.html#af7660187be027791a7264a12085fb338a20e5055e66945c8231db009fbc917444", null ],
      [ "PTP_PF_N", "ptp__types_8h.html#af7660187be027791a7264a12085fb338a19ae1cd6849caaba9c0309782366f672", null ]
    ] ],
    [ "PtpTimeSource", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672", [
      [ "PTP_TSRC_ATOMIC_CLOCK", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672a0fe365af8bdd243fc0c72afd7f2197cf", null ],
      [ "PTP_TSRC_GPS", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672a8d6a79e66a805ca992ef7489fd015442", null ],
      [ "PTP_TSRC_TERRESTRIAL_RADIO", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672aca35aa78938ced61d2b8cb87abbf3d28", null ],
      [ "PTP_TSRC_PTP", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672a28d9c35f7812b3977b62ee2794f5cedc", null ],
      [ "PTP_TSRC_NTP", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672a75350f2b1bd7649d9974b1291c5c7d2d", null ],
      [ "PTP_TSRC_HAND_SET", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672afcc6c38951381f72fc0820d31e7ca567", null ],
      [ "PTP_TSRC_OTHER", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672abe4aeaeb7676e46f1b3f3b7c4b280cca", null ],
      [ "PTP_TSRC_INTERNAL_OSCILLATOR", "ptp__types_8h.html#afa9baeef926cde7aa743c0e34f7ff672a7d62ed7aacb952d34cd2736c5e757136", null ]
    ] ],
    [ "PtpTlvType", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbc", [
      [ "PTP_TLV_MANAGEMENT", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbca9ca64a1d2ce187ecb2f884e024b9cfb5", null ],
      [ "PTP_TLV_MANAGEMENT_ERROR_STATUS", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbca934c1096ec47b49be9dba9fef533fe47", null ],
      [ "PTP_TLV_ORGANIZATION_EXTENSION", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbcab4d61ca4d586126dc657738da2f52756", null ],
      [ "PTP_TLV_REQUEST_UNICAST_TRANSMISSION", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbcaf5ce01f58eebb5ea81ac97f9d0e75507", null ],
      [ "PTP_TLV_GRANT_UNICAST_TRANSMISSION", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbcaa8a27b31d4c470ed0f1b0f50171f74da", null ],
      [ "PTP_TLV_CANCEL_UNICAST_TRANSMISSION", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbcacbc42698eab7e532b20d2e2e8b5dbc7d", null ],
      [ "PTP_TLV_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbca1af737969394ca6954071b3a07615cb6", null ],
      [ "PTP_TLV_PATH_TRACE", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbca7b9f354fb3b384282bf19bf2b13706ce", null ],
      [ "PTP_TLV_ALTERNATE_TIME_OFFSET_INDICATOR", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbca14e5f93a1a3297907ea2d2178dee38e4", null ],
      [ "PTP_TLV_AUTHENTICATION", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbcaee27e40ae5ecc87b92632f59a6724245", null ],
      [ "PTP_TLV_AUTHENTICATION_CHALLENGE", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbca71163aa49c9897c7340f7930220582d1", null ],
      [ "PTP_TLV_SECURITY_ASSOCIATION_UPDATE", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbcac6c13ecf352d62ce1cfe366c2affe949", null ],
      [ "PTP_TLV_CUM_FREQ_SCALE_FACTOR_OFFSET", "ptp__types_8h.html#a7d266c40bb65b66a1960128671a4abbca23e63ca4b3dfe305598175a65d591e02", null ]
    ] ],
    [ "PtpTransportSpecific", "ptp__types_8h.html#aeb56a68f27eb91b48211dcad78a709d8", [
      [ "PTP_TSPEC_UNKNOWN_DEF", "ptp__types_8h.html#aeb56a68f27eb91b48211dcad78a709d8af762abfb2929779dbf3f8fee8ce01bde", null ],
      [ "PTP_TSPEC_GPTP_8021AS", "ptp__types_8h.html#aeb56a68f27eb91b48211dcad78a709d8a6a35cc8ef27f7db0ebc8fb122d42c161", null ]
    ] ],
    [ "PtpTransportType", "ptp__types_8h.html#a2964210d435f30e9125953c23045ea0b", [
      [ "PTP_TP_IPv4", "ptp__types_8h.html#a2964210d435f30e9125953c23045ea0ba0d18b938a553d2c2eddb2113b1aca22c", null ],
      [ "PTP_TP_802_3", "ptp__types_8h.html#a2964210d435f30e9125953c23045ea0baa0d87f8f9801afbfbfdb6ed79fbee525", null ]
    ] ]
];