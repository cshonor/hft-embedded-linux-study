# UNP Vol1 目录索引

> 由 `scripts/generate_structure.py` 自动生成，与 `SECTIONS` 字典一致。

## 1_BasicFoundation/

### Chapter01_Introduction/ (12 sections)

- `1.1_Overview.md`
- `1.2_SimpleTimeClient.md`
- `1.3_ProtocolIndependence.md`
- `1.4_ErrorHandlingWrapper.md`
- `1.5_SimpleTimeServer.md`
- `1.6_ProgramIndex.md`
- `1.7_OSIModel.md`
- `1.8_BSDNetworkHistory.md`
- `1.9_TestNetworkHost.md`
- `1.10_UnixStandard.md`
- `1.11_64BitArchitecture.md`
- `1.12_Summary.md`

### Chapter02_TCP_UDP_SCTP/ (14 sections)

- `2.1_Overview.md`
- `2.2_GeneralGraph.md`
- `2.3_UDP_Protocol.md`
- `2.4_TCP_Protocol.md`
- `2.5_SCTP_Protocol.md`
- `2.6_TCP_Connect_Terminate.md`
- `2.7_TIME_WAIT_State.md`
- `2.8_SCTP_Connect_Terminate.md`
- `2.9_PortNumber.md`
- `2.10_TCP_Port_ConcurrentServer.md`
- `2.11_Buffer_Size_Limit.md`
- `2.12_StandardInternetService.md`
- `2.13_AppProtocolUsage.md`
- `2.14_Summary.md`

### Chapter03_SocketProgramIntro/ (10 sections)

- `3.1_Overview.md`
- `3.2_SocketAddressStructure.md`
- `3.3_Value_Result_Argument.md`
- `3.4_ByteOrderFunction.md`
- `3.5_ByteOperateFunction.md`
- `3.6_Inet_Addr_Series.md`
- `3.7_Inet_Pton_Ntop.md`
- `3.8_Sock_Ntop_RelatedFunc.md`
- `3.9_Readn_Writen_Readline.md`
- `3.10_Summary.md`

### Chapter04_BasicTCPSocket/ (11 sections)

- `4.1_Overview.md`
- `4.2_Socket_Function.md`
- `4.3_Connect_Function.md`
- `4.4_Bind_Function.md`
- `4.5_Listen_Function.md`
- `4.6_Accept_Function.md`
- `4.7_Fork_Exec_Function.md`
- `4.8_ConcurrentServer.md`
- `4.9_Close_Function.md`
- `4.10_Getsockname_Getpeername.md`
- `4.11_Summary.md`

### Chapter05_TCP_Client_Server_Demo/ (19 sections)

- `5.1_Overview.md`
- `5.2_Server_Main.md`
- `5.3_Server_Str_Echo.md`
- `5.4_Client_Main.md`
- `5.5_Client_Str_Cli.md`
- `5.6_Normal_Start.md`
- `5.7_Normal_Exit.md`
- `5.8_POSIX_Signal.md`
- `5.9_SIGCHLD_Process.md`
- `5.10_Wait_Waitpid_Func.md`
- `5.11_Accept_Interrupted.md`
- `5.12_Server_Process_Abort.md`
- `5.13_SIGPIPE_Signal.md`
- `5.14_Server_Host_Crash.md`
- `5.15_Server_Host_Restart.md`
- `5.16_Server_Host_Shutdown.md`
- `5.17_TCP_Demo_Summary.md`
- `5.18_Data_Format_Transfer.md`
- `5.19_Summary.md`

### Chapter06_IO_Select_Poll/ (12 sections)

- `6.1_Overview.md`
- `6.2_IO_Model_Type.md`
- `6.3_Select_Function.md`
- `6.4_Str_Cli_Revised.md`
- `6.5_Batch_Input_Process.md`
- `6.6_Shutdown_Function.md`
- `6.7_Str_Cli_Final_Revised.md`
- `6.8_TCP_Server_Revised.md`
- `6.9_Pselect_Function.md`
- `6.10_Poll_Function.md`
- `6.11_TCP_Server_Poll_Revised.md`
- `6.12_Summary.md`

### Chapter07_SocketOption/ (12 sections)

- `7.1_Overview.md`
- `7.2_Getsockopt_Setsockopt.md`
- `7.3_Option_Check_DefaultValue.md`
- `7.4_Socket_State_Rule.md`
- `7.5_Common_Socket_Option.md`
- `7.6_IPv4_Socket_Option.md`
- `7.7_ICMPv6_Socket_Option.md`
- `7.8_IPv6_Socket_Option.md`
- `7.9_TCP_Socket_Option.md`
- `7.10_SCTP_Socket_Option.md`
- `7.11_Fcntl_Control_Func.md`
- `7.12_Summary.md`

### Chapter08_BasicUDPSocket/ (16 sections)

- `8.1_Overview.md`
- `8.2_Recvfrom_Sendto_Func.md`
- `8.3_UDP_Server_Main.md`
- `8.4_UDP_Server_Dg_Echo.md`
- `8.5_UDP_Client_Main.md`
- `8.6_UDP_Client_Dg_Cli.md`
- `8.7_Datagram_Loss_Problem.md`
- `8.8_Response_Data_Verify.md`
- `8.9_Server_Offline_State.md`
- `8.10_UDP_Demo_Summary.md`
- `8.11_UDP_Connect_Usage.md`
- `8.12_Dg_Cli_Revised.md`
- `8.13_UDP_FlowControl_Defect.md`
- `8.14_UDP_Outbound_Interface.md`
- `8.15_TCP_UDP_Mixed_Server.md`
- `8.16_Summary.md`

## 2_AdvancedSkill/

### Chapter11_Name_Address_Convert/ (22 sections)

- `11.1_Overview.md`
- `11.2_DNS_System.md`
- `11.3_Gethostbyname_Func.md`
- `11.4_Gethostbyaddr_Func.md`
- `11.5_Getservbyname_Getservbyport.md`
- `11.6_Getaddrinfo_Func.md`
- `11.7_Gai_strerror_Func.md`
- `11.8_Freeaddrinfo_Func.md`
- `11.9_Getaddrinfo_IPv6.md`
- `11.10_Getaddrinfo_CaseDemo.md`
- `11.11_Host_Serv_Func.md`
- `11.12_Tcp_Connect_Func.md`
- `11.13_Tcp_Listen_Func.md`
- `11.14_Udp_Client_Func.md`
- `11.15_Udp_Connect_Func.md`
- `11.16_Udp_Server_Func.md`
- `11.17_Getnameinfo_Func.md`
- `11.18_Reentrant_Function.md`
- `11.19_Gethostbyname_r_Gethostbyaddr_r.md`
- `11.20_Old_IPv6_Convert_Func.md`
- `11.21_Other_Network_Info.md`
- `11.22_Summary.md`

### Chapter13_Daemon_Inetd/ (7 sections)

- `13.1_Overview.md`
- `13.2_Syslogd_Daemon.md`
- `13.3_Syslog_Func.md`
- `13.4_Daemon_Init_Func.md`
- `13.5_Inetd_Daemon.md`
- `13.6_Daemon_Inetd_Func.md`
- `13.7_Summary.md`

### Chapter14_AdvancedIO_Func/ (11 sections)

- `14.1_Overview.md`
- `14.2_Socket_Timeout_Set.md`
- `14.3_Recv_Send_Func.md`
- `14.4_Readv_Writev_Func.md`
- `14.5_Recvmsg_Sendmsg_Func.md`
- `14.6_Auxiliary_Data.md`
- `14.7_Pending_Data_Check.md`
- `14.8_Socket_StdIO_Mix.md`
- `14.9_Advanced_Poll_Method.md`
- `14.10_TCP_Transaction_Type.md`
- `14.11_Summary.md`

### Chapter16_NonBlockingIO/ (7 sections)

- `16.1_Overview.md`
- `16.2_NonBlock_Read_Write.md`
- `16.3_NonBlock_Connect.md`
- `16.4_NonBlock_Connect_TimeClient.md`
- `16.5_NonBlock_Connect_WebClient.md`
- `16.6_NonBlock_Accept.md`
- `16.7_Summary.md`

### Chapter26_Thread/ (10 sections)

- `26.1_Overview.md`
- `26.2_Thread_Create_Exit.md`
- `26.3_Thread_Str_Cli.md`
- `26.4_Thread_TCP_Server.md`
- `26.5_Thread_Private_Data.md`
- `26.6_Web_Client_Multi_Connect.md`
- `26.7_Mutex_Lock.md`
- `26.8_Condition_Variable.md`
- `26.9_Web_Client_Connect_Supplement.md`
- `26.10_Summary.md`

## 3_DeepMaster/

### Chapter17_Ioctl_Operate/ (10 sections)

- `17.1_Overview.md`
- `17.2_Ioctl_Function.md`
- `17.3_Socket_Ioctl_Operate.md`
- `17.4_File_Ioctl_Operate.md`
- `17.5_Network_Interface_Config.md`
- `17.6_Get_Ifi_Info_Func.md`
- `17.7_Interface_Control_Operate.md`
- `17.8_ARP_Cache_Operate.md`
- `17.9_Route_Table_Operate.md`
- `17.10_Summary.md`

### Chapter20_Broadcast/ (6 sections)

- `20.1_Overview.md`
- `20.2_Broadcast_Address.md`
- `20.3_Unicast_Broadcast_Compare.md`
- `20.4_Broadcast_Dg_Cli.md`
- `20.5_Race_Condition_Problem.md`
- `20.6_Summary.md`

### Chapter21_Multicast/ (12 sections)

- `21.1_Overview.md`
- `21.2_Multicast_Address.md`
- `21.3_LAN_Multicast_Broadcast.md`
- `21.4_WAN_Multicast_Transfer.md`
- `21.5_Source_Specific_Multicast.md`
- `21.6_Multicast_Socket_Option.md`
- `21.7_Mcast_Join_Related_Func.md`
- `21.8_Multicast_Dg_Cli.md`
- `21.9_Multicast_Session_Declare.md`
- `21.10_Send_Receive_Multicast_Data.md`
- `21.11_SNTP_Protocol_Practice.md`
- `21.12_Summary.md`

### Chapter22_AdvancedUDPSocket/ (10 sections)

- `22.1_Overview.md`
- `22.2_Flag_DestIP_InterfaceIndex.md`
- `22.3_Datagram_Truncation.md`
- `22.4_UDP_TCP_Scene_Choice.md`
- `22.5_UDP_Reliable_Transform.md`
- `22.6_Bind_Specified_Interface.md`
- `22.7_Concurrent_UDP_Server.md`
- `22.8_IPv6_Packet_Info.md`
- `22.9_IPv6_Path_MTU_Control.md`
- `22.10_Summary.md`

### Chapter24_OutOfBandData/ (6 sections)

- `24.1_Overview.md`
- `24.2_TCP_OutOfBand_Data.md`
- `24.3_Sockatmark_Func.md`
- `24.4_TCP_OutOfBand_Summary.md`
- `24.5_Client_Server_Heartbeat.md`
- `24.6_Summary.md`

### Chapter25_SignalDriveIO/ (4 sections)

- `25.1_Overview.md`
- `25.2_Socket_Signal_Drive_IO.md`
- `25.3_SIGIO_UDP_Echo_Server.md`
- `25.4_Summary.md`

### Chapter28_RawSocket/ (8 sections)

- `28.1_Overview.md`
- `28.2_RawSocket_Create.md`
- `28.3_RawSocket_Send_Data.md`
- `28.4_RawSocket_Recv_Data.md`
- `28.5_Ping_Program_Implement.md`
- `28.6_Traceroute_Program_Implement.md`
- `28.7_ICMP_Message_Daemon.md`
- `28.8_Summary.md`

### Chapter29_DataLinkAccess/ (8 sections)

- `29.1_Overview.md`
- `29.2_BSD_Packet_Filter.md`
- `29.3_DataLink_Provider_Interface.md`
- `29.4_Linux_Packet_Socket.md`
- `29.5_Libpcap_Capture_Lib.md`
- `29.6_Libnet_Packet_Build_Lib.md`
- `29.7_UDP_Checksum_Check.md`
- `29.8_Summary.md`

## 4_ArchitectureDesign/

### Chapter09_BasicSCTPSocket/ (15 sections)

- `9.1_Overview.md`
- `9.2_Interface_Model.md`
- `9.3_Sctp_Bindx_Func.md`
- `9.4_Sctp_Connectx_Func.md`
- `9.5_Sctp_Getpaddrs_Func.md`
- `9.6_Sctp_Freepaddrs_Func.md`
- `9.7_Sctp_Getladdrs_Func.md`
- `9.8_Sctp_Freeladdrs_Func.md`
- `9.9_Sctp_Sendmsg_Func.md`
- `9.10_Sctp_Recvmsg_Func.md`
- `9.11_Sctp_Opt_Info_Func.md`
- `9.12_Sctp_Peeloff_Func.md`
- `9.13_Shutdown_Func.md`
- `9.14_SCTP_Notification.md`
- `9.15_Summary.md`

### Chapter10_SCTP_Client_Server_Demo/ (8 sections)

- `10.1_Overview.md`
- `10.2_SCTP_OneToMany_Server.md`
- `10.3_SCTP_OneToMany_Client.md`
- `10.4_Sctp_Str_Cli_Func.md`
- `10.5_Head_Blocking_Problem.md`
- `10.6_Stream_Number_Control.md`
- `10.7_Connection_Terminate_Control.md`
- `10.8_Summary.md`

### Chapter12_IPv4_IPv6_Interop/ (6 sections)

- `12.1_Overview.md`
- `12.2_IPv4_Client_IPv6_Server.md`
- `12.3_IPv6_Client_IPv4_Server.md`
- `12.4_IPv6_Address_Macro.md`
- `12.5_Source_Code_Portability.md`
- `12.6_Summary.md`

### Chapter15_UnixDomainProtocol/ (9 sections)

- `15.1_Overview.md`
- `15.2_UnixDomain_Socket_Addr.md`
- `15.3_Socketpair_Func.md`
- `15.4_Socket_Basic_Func.md`
- `15.5_Unix_Stream_Client_Server.md`
- `15.6_Unix_Datagram_Client_Server.md`
- `15.7_File_Descriptor_Transfer.md`
- `15.8_Sender_Credential_Receive.md`
- `15.9_Summary.md`

### Chapter18_RoutingSocket/ (7 sections)

- `18.1_Overview.md`
- `18.2_DataLink_Socket_Addr.md`
- `18.3_Socket_Read_Write.md`
- `18.4_Sysctl_Operate.md`
- `18.5_Get_Ifi_Info_Func.md`
- `18.6_Interface_Name_Index_Func.md`
- `18.7_Summary.md`

### Chapter19_KeyManageSocket/ (6 sections)

- `19.1_Overview.md`
- `19.2_Socket_Read_Write.md`
- `19.3_Security_DB_Dump.md`
- `19.4_Static_Security_Create.md`
- `19.5_Dynamic_Security_Maintain.md`
- `19.6_Summary.md`

### Chapter23_AdvancedSCTPSocket/ (13 sections)

- `23.1_Overview.md`
- `23.2_AutoClose_OneToMany_Server.md`
- `23.3_Partial_Data_Deliver.md`
- `23.4_SCTP_Notification_Msg.md`
- `23.5_Unordered_Data_Transfer.md`
- `23.6_Bind_Address_Subset.md`
- `23.7_Local_Remote_Addr_Query.md`
- `23.8_IP_Association_ID_Match.md`
- `23.9_Heartbeat_Addr_Unreachable.md`
- `23.10_Association_Split_Operate.md`
- `23.11_Time_Parameter_Control.md`
- `23.12_SCTP_TCP_Scene_Choice.md`
- `23.13_Summary.md`

### Chapter27_IP_Option/ (9 sections)

- `27.1_Overview.md`
- `27.2_IPv4_Packet_Option.md`
- `27.3_IPv4_Source_Route_Option.md`
- `27.4_IPv6_Extend_Header.md`
- `27.5_IPv6_Hop_Dest_Option.md`
- `27.6_IPv6_Route_Header.md`
- `27.7_IPv6_Sticky_Option.md`
- `27.8_Historical_IPv6_API.md`
- `27.9_Summary.md`

### Chapter30_Client_Server_DesignMode/ (13 sections)

- `30.1_Overview.md`
- `30.2_TCP_Client_Design_Pattern.md`
- `30.3_TCP_Test_Client.md`
- `30.4_TCP_Iterative_Server.md`
- `30.5_TCP_Fork_Concurrent_Server.md`
- `30.6_PreFork_Server_NoLock.md`
- `30.7_PreFork_Server_FileLock.md`
- `30.8_PreFork_Server_ThreadLock.md`
- `30.9_PreFork_Server_Fd_Transfer.md`
- `30.10_TCP_Thread_Concurrent_Server.md`
- `30.11_PreThread_Server_SingleAccept.md`
- `30.12_PreThread_Server_MainAccept.md`
- `30.13_Summary.md`

### Chapter31_Stream/ (7 sections)

- `31.1_Overview.md`
- `31.2_Stream_Structure_Profile.md`
- `31.3_Getmsg_Putmsg_Func.md`
- `31.4_Getpmsg_Putpmsg_Func.md`
- `31.5_Ioctl_Stream_Control.md`
- `31.6_Transport_Provider_Interface.md`
- `31.7_Summary.md`

**Total: 320 sections**
