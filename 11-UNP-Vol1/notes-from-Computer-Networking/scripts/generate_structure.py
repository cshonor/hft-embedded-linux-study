#!/usr/bin/env python3
"""Generate / sync UNP Vol1 four-level directory structure (explicit section names)."""

from __future__ import annotations

import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

NOTES_TEMPLATE = """# {title}

---

## 核心主旨与关键论据



---

## 核心知识点与细节



---

## 逻辑脉络与因果关系



---

## 易错细节与重点结论



---

> 💡 **后续拓展留白**

---

## 个人学习总结

（待填）
"""

PHASES: dict[str, list[int]] = {
    "1_BasicFoundation": list(range(1, 9)),
    "2_AdvancedSkill": [11, 13, 14, 16, 26],
    "3_DeepMaster": [17, 20, 21, 22, 24, 25, 28, 29],
    "4_ArchitectureDesign": [9, 10, 12, 15, 18, 19, 23, 27, 30, 31],
}

CHAPTER_DIRS: dict[int, str] = {
    1: "Chapter01_Introduction",
    2: "Chapter02_TCP_UDP_SCTP",
    3: "Chapter03_SocketProgramIntro",
    4: "Chapter04_BasicTCPSocket",
    5: "Chapter05_TCP_Client_Server_Demo",
    6: "Chapter06_IO_Select_Poll",
    7: "Chapter07_SocketOption",
    8: "Chapter08_BasicUDPSocket",
    9: "Chapter09_BasicSCTPSocket",
    10: "Chapter10_SCTP_Client_Server_Demo",
    11: "Chapter11_Name_Address_Convert",
    12: "Chapter12_IPv4_IPv6_Interop",
    13: "Chapter13_Daemon_Inetd",
    14: "Chapter14_AdvancedIO_Func",
    15: "Chapter15_UnixDomainProtocol",
    16: "Chapter16_NonBlockingIO",
    17: "Chapter17_Ioctl_Operate",
    18: "Chapter18_RoutingSocket",
    19: "Chapter19_KeyManageSocket",
    20: "Chapter20_Broadcast",
    21: "Chapter21_Multicast",
    22: "Chapter22_AdvancedUDPSocket",
    23: "Chapter23_AdvancedSCTPSocket",
    24: "Chapter24_OutOfBandData",
    25: "Chapter25_SignalDriveIO",
    26: "Chapter26_Thread",
    27: "Chapter27_IP_Option",
    28: "Chapter28_RawSocket",
    29: "Chapter29_DataLinkAccess",
    30: "Chapter30_Client_Server_DesignMode",
    31: "Chapter31_Stream",
}

# Explicit fourth-level folder names (authoritative)
SECTIONS: dict[int, list[str]] = {
    1: [
        "1.1_Overview", "1.2_SimpleTimeClient", "1.3_ProtocolIndependence",
        "1.4_ErrorHandlingWrapper", "1.5_SimpleTimeServer", "1.6_ProgramIndex",
        "1.7_OSIModel", "1.8_BSDNetworkHistory", "1.9_TestNetworkHost",
        "1.10_UnixStandard", "1.11_64BitArchitecture", "1.12_Summary",
    ],
    2: [
        "2.1_Overview", "2.2_GeneralGraph", "2.3_UDP_Protocol", "2.4_TCP_Protocol",
        "2.5_SCTP_Protocol", "2.6_TCP_Connect_Terminate", "2.7_TIME_WAIT_State",
        "2.8_SCTP_Connect_Terminate", "2.9_PortNumber", "2.10_TCP_Port_ConcurrentServer",
        "2.11_Buffer_Size_Limit", "2.12_StandardInternetService", "2.13_AppProtocolUsage",
        "2.14_Summary",
    ],
    3: [
        "3.1_Overview", "3.2_SocketAddressStructure", "3.3_Value_Result_Argument",
        "3.4_ByteOrderFunction", "3.5_ByteOperateFunction", "3.6_Inet_Addr_Series",
        "3.7_Inet_Pton_Ntop", "3.8_Sock_Ntop_RelatedFunc", "3.9_Readn_Writen_Readline",
        "3.10_Summary",
    ],
    4: [
        "4.1_Overview", "4.2_Socket_Function", "4.3_Connect_Function", "4.4_Bind_Function",
        "4.5_Listen_Function", "4.6_Accept_Function", "4.7_Fork_Exec_Function",
        "4.8_ConcurrentServer", "4.9_Close_Function", "4.10_Getsockname_Getpeername",
        "4.11_Summary",
    ],
    5: [
        "5.1_Overview", "5.2_Server_Main", "5.3_Server_Str_Echo", "5.4_Client_Main",
        "5.5_Client_Str_Cli", "5.6_Normal_Start", "5.7_Normal_Exit", "5.8_POSIX_Signal",
        "5.9_SIGCHLD_Process", "5.10_Wait_Waitpid_Func", "5.11_Accept_Interrupted",
        "5.12_Server_Process_Abort", "5.13_SIGPIPE_Signal", "5.14_Server_Host_Crash",
        "5.15_Server_Host_Restart", "5.16_Server_Host_Shutdown", "5.17_TCP_Demo_Summary",
        "5.18_Data_Format_Transfer", "5.19_Summary",
    ],
    6: [
        "6.1_Overview", "6.2_IO_Model_Type", "6.3_Select_Function", "6.4_Str_Cli_Revised",
        "6.5_Batch_Input_Process", "6.6_Shutdown_Function", "6.7_Str_Cli_Final_Revised",
        "6.8_TCP_Server_Revised", "6.9_Pselect_Function", "6.10_Poll_Function",
        "6.11_TCP_Server_Poll_Revised", "6.12_Summary",
    ],
    7: [
        "7.1_Overview", "7.2_Getsockopt_Setsockopt", "7.3_Option_Check_DefaultValue",
        "7.4_Socket_State_Rule", "7.5_Common_Socket_Option", "7.6_IPv4_Socket_Option",
        "7.7_ICMPv6_Socket_Option", "7.8_IPv6_Socket_Option", "7.9_TCP_Socket_Option",
        "7.10_SCTP_Socket_Option", "7.11_Fcntl_Control_Func", "7.12_Summary",
    ],
    8: [
        "8.1_Overview", "8.2_Recvfrom_Sendto_Func", "8.3_UDP_Server_Main",
        "8.4_UDP_Server_Dg_Echo", "8.5_UDP_Client_Main", "8.6_UDP_Client_Dg_Cli",
        "8.7_Datagram_Loss_Problem", "8.8_Response_Data_Verify", "8.9_Server_Offline_State",
        "8.10_UDP_Demo_Summary", "8.11_UDP_Connect_Usage", "8.12_Dg_Cli_Revised",
        "8.13_UDP_FlowControl_Defect", "8.14_UDP_Outbound_Interface",
        "8.15_TCP_UDP_Mixed_Server", "8.16_Summary",
    ],
    9: [
        "9.1_Overview", "9.2_Interface_Model", "9.3_Sctp_Bindx_Func", "9.4_Sctp_Connectx_Func",
        "9.5_Sctp_Getpaddrs_Func", "9.6_Sctp_Freepaddrs_Func", "9.7_Sctp_Getladdrs_Func",
        "9.8_Sctp_Freeladdrs_Func", "9.9_Sctp_Sendmsg_Func", "9.10_Sctp_Recvmsg_Func",
        "9.11_Sctp_Opt_Info_Func", "9.12_Sctp_Peeloff_Func", "9.13_Shutdown_Func",
        "9.14_SCTP_Notification", "9.15_Summary",
    ],
    10: [
        "10.1_Overview", "10.2_SCTP_OneToMany_Server", "10.3_SCTP_OneToMany_Client",
        "10.4_Sctp_Str_Cli_Func", "10.5_Head_Blocking_Problem", "10.6_Stream_Number_Control",
        "10.7_Connection_Terminate_Control", "10.8_Summary",
    ],
    11: [
        "11.1_Overview", "11.2_DNS_System", "11.3_Gethostbyname_Func",
        "11.4_Gethostbyaddr_Func", "11.5_Getservbyname_Getservbyport",
        "11.6_Getaddrinfo_Func", "11.7_Gai_strerror_Func", "11.8_Freeaddrinfo_Func",
        "11.9_Getaddrinfo_IPv6", "11.10_Getaddrinfo_CaseDemo", "11.11_Host_Serv_Func",
        "11.12_Tcp_Connect_Func", "11.13_Tcp_Listen_Func", "11.14_Udp_Client_Func",
        "11.15_Udp_Connect_Func", "11.16_Udp_Server_Func", "11.17_Getnameinfo_Func",
        "11.18_Reentrant_Function", "11.19_Gethostbyname_r_Gethostbyaddr_r",
        "11.20_Old_IPv6_Convert_Func", "11.21_Other_Network_Info", "11.22_Summary",
    ],
    12: [
        "12.1_Overview", "12.2_IPv4_Client_IPv6_Server", "12.3_IPv6_Client_IPv4_Server",
        "12.4_IPv6_Address_Macro", "12.5_Source_Code_Portability", "12.6_Summary",
    ],
    13: [
        "13.1_Overview", "13.2_Syslogd_Daemon", "13.3_Syslog_Func", "13.4_Daemon_Init_Func",
        "13.5_Inetd_Daemon", "13.6_Daemon_Inetd_Func", "13.7_Summary",
    ],
    14: [
        "14.1_Overview", "14.2_Socket_Timeout_Set", "14.3_Recv_Send_Func",
        "14.4_Readv_Writev_Func", "14.5_Recvmsg_Sendmsg_Func", "14.6_Auxiliary_Data",
        "14.7_Pending_Data_Check", "14.8_Socket_StdIO_Mix", "14.9_Advanced_Poll_Method",
        "14.10_TCP_Transaction_Type", "14.11_Summary",
    ],
    15: [
        "15.1_Overview", "15.2_UnixDomain_Socket_Addr", "15.3_Socketpair_Func",
        "15.4_Socket_Basic_Func", "15.5_Unix_Stream_Client_Server",
        "15.6_Unix_Datagram_Client_Server", "15.7_File_Descriptor_Transfer",
        "15.8_Sender_Credential_Receive", "15.9_Summary",
    ],
    16: [
        "16.1_Overview", "16.2_NonBlock_Read_Write", "16.3_NonBlock_Connect",
        "16.4_NonBlock_Connect_TimeClient", "16.5_NonBlock_Connect_WebClient",
        "16.6_NonBlock_Accept", "16.7_Summary",
    ],
    17: [
        "17.1_Overview", "17.2_Ioctl_Function", "17.3_Socket_Ioctl_Operate",
        "17.4_File_Ioctl_Operate", "17.5_Network_Interface_Config",
        "17.6_Get_Ifi_Info_Func", "17.7_Interface_Control_Operate",
        "17.8_ARP_Cache_Operate", "17.9_Route_Table_Operate", "17.10_Summary",
    ],
    18: [
        "18.1_Overview", "18.2_DataLink_Socket_Addr", "18.3_Socket_Read_Write",
        "18.4_Sysctl_Operate", "18.5_Get_Ifi_Info_Func", "18.6_Interface_Name_Index_Func",
        "18.7_Summary",
    ],
    19: [
        "19.1_Overview", "19.2_Socket_Read_Write", "19.3_Security_DB_Dump",
        "19.4_Static_Security_Create", "19.5_Dynamic_Security_Maintain", "19.6_Summary",
    ],
    20: [
        "20.1_Overview", "20.2_Broadcast_Address", "20.3_Unicast_Broadcast_Compare",
        "20.4_Broadcast_Dg_Cli", "20.5_Race_Condition_Problem", "20.6_Summary",
    ],
    21: [
        "21.1_Overview", "21.2_Multicast_Address", "21.3_LAN_Multicast_Broadcast",
        "21.4_WAN_Multicast_Transfer", "21.5_Source_Specific_Multicast",
        "21.6_Multicast_Socket_Option", "21.7_Mcast_Join_Related_Func",
        "21.8_Multicast_Dg_Cli", "21.9_Multicast_Session_Declare",
        "21.10_Send_Receive_Multicast_Data", "21.11_SNTP_Protocol_Practice",
        "21.12_Summary",
    ],
    22: [
        "22.1_Overview", "22.2_Flag_DestIP_InterfaceIndex", "22.3_Datagram_Truncation",
        "22.4_UDP_TCP_Scene_Choice", "22.5_UDP_Reliable_Transform",
        "22.6_Bind_Specified_Interface", "22.7_Concurrent_UDP_Server",
        "22.8_IPv6_Packet_Info", "22.9_IPv6_Path_MTU_Control", "22.10_Summary",
    ],
    23: [
        "23.1_Overview", "23.2_AutoClose_OneToMany_Server", "23.3_Partial_Data_Deliver",
        "23.4_SCTP_Notification_Msg", "23.5_Unordered_Data_Transfer",
        "23.6_Bind_Address_Subset", "23.7_Local_Remote_Addr_Query",
        "23.8_IP_Association_ID_Match", "23.9_Heartbeat_Addr_Unreachable",
        "23.10_Association_Split_Operate", "23.11_Time_Parameter_Control",
        "23.12_SCTP_TCP_Scene_Choice", "23.13_Summary",
    ],
    24: [
        "24.1_Overview", "24.2_TCP_OutOfBand_Data", "24.3_Sockatmark_Func",
        "24.4_TCP_OutOfBand_Summary", "24.5_Client_Server_Heartbeat", "24.6_Summary",
    ],
    25: [
        "25.1_Overview", "25.2_Socket_Signal_Drive_IO", "25.3_SIGIO_UDP_Echo_Server",
        "25.4_Summary",
    ],
    26: [
        "26.1_Overview", "26.2_Thread_Create_Exit", "26.3_Thread_Str_Cli",
        "26.4_Thread_TCP_Server", "26.5_Thread_Private_Data", "26.6_Web_Client_Multi_Connect",
        "26.7_Mutex_Lock", "26.8_Condition_Variable", "26.9_Web_Client_Connect_Supplement",
        "26.10_Summary",
    ],
    27: [
        "27.1_Overview", "27.2_IPv4_Packet_Option", "27.3_IPv4_Source_Route_Option",
        "27.4_IPv6_Extend_Header", "27.5_IPv6_Hop_Dest_Option", "27.6_IPv6_Route_Header",
        "27.7_IPv6_Sticky_Option", "27.8_Historical_IPv6_API", "27.9_Summary",
    ],
    28: [
        "28.1_Overview", "28.2_RawSocket_Create", "28.3_RawSocket_Send_Data",
        "28.4_RawSocket_Recv_Data", "28.5_Ping_Program_Implement",
        "28.6_Traceroute_Program_Implement", "28.7_ICMP_Message_Daemon", "28.8_Summary",
    ],
    29: [
        "29.1_Overview", "29.2_BSD_Packet_Filter", "29.3_DataLink_Provider_Interface",
        "29.4_Linux_Packet_Socket", "29.5_Libpcap_Capture_Lib", "29.6_Libnet_Packet_Build_Lib",
        "29.7_UDP_Checksum_Check", "29.8_Summary",
    ],
    30: [
        "30.1_Overview", "30.2_TCP_Client_Design_Pattern", "30.3_TCP_Test_Client",
        "30.4_TCP_Iterative_Server", "30.5_TCP_Fork_Concurrent_Server",
        "30.6_PreFork_Server_NoLock", "30.7_PreFork_Server_FileLock",
        "30.8_PreFork_Server_ThreadLock", "30.9_PreFork_Server_Fd_Transfer",
        "30.10_TCP_Thread_Concurrent_Server", "30.11_PreThread_Server_SingleAccept",
        "30.12_PreThread_Server_MainAccept", "30.13_Summary",
    ],
    31: [
        "31.1_Overview", "31.2_Stream_Structure_Profile", "31.3_Getmsg_Putmsg_Func",
        "31.4_Getpmsg_Putpmsg_Func", "31.5_Ioctl_Stream_Control",
        "31.6_Transport_Provider_Interface", "31.7_Summary",
    ],
}

# Old name -> new name for notes migration (Ch 1-8 renames only)
RENAME_MAP: dict[str, str] = {
    "2.3_UDP": "2.3_UDP_Protocol",
    "2.4_TCP": "2.4_TCP_Protocol",
    "2.5_SCTP": "2.5_SCTP_Protocol",
    "3.6_Inet_Addr_Family": "3.6_Inet_Addr_Series",
    "3.8_Sock_Ntop_Related": "3.8_Sock_Ntop_RelatedFunc",
    "4.2_Socket_Func": "4.2_Socket_Function",
    "4.3_Connect_Func": "4.3_Connect_Function",
    "4.4_Bind_Func": "4.4_Bind_Function",
    "4.5_Listen_Func": "4.5_Listen_Function",
    "4.6_Accept_Func": "4.6_Accept_Function",
    "4.7_Fork_Exec_Func": "4.7_Fork_Exec_Function",
    "4.9_Close_Func": "4.9_Close_Function",
    "5.9_SIGCHLD_Handle": "5.9_SIGCHLD_Process",
    "5.10_Wait_Waitpid": "5.10_Wait_Waitpid_Func",
    "5.11_Accept_Interrupt": "5.11_Accept_Interrupted",
    "5.12_Server_Process_Exit": "5.12_Server_Process_Abort",
    "5.18_Data_Format": "5.18_Data_Format_Transfer",
    "6.2_IO_Model": "6.2_IO_Model_Type",
    "6.3_Select_Func": "6.3_Select_Function",
    "6.4_Str_Cli_Revise": "6.4_Str_Cli_Revised",
    "6.5_Batch_Input": "6.5_Batch_Input_Process",
    "6.6_Shutdown_Func": "6.6_Shutdown_Function",
    "6.7_Str_Cli_Final_Revise": "6.7_Str_Cli_Final_Revised",
    "6.8_TCP_Server_Revise": "6.8_TCP_Server_Revised",
    "6.9_Pselect_Func": "6.9_Pselect_Function",
    "6.10_Poll_Func": "6.10_Poll_Function",
    "6.11_TCP_Server_Poll_Revise": "6.11_TCP_Server_Poll_Revised",
    "7.3_Option_Check_Default": "7.3_Option_Check_DefaultValue",
    "7.4_Socket_State": "7.4_Socket_State_Rule",
    "7.11_Fcntl_Func": "7.11_Fcntl_Control_Func",
    "8.2_Recvfrom_Sendto": "8.2_Recvfrom_Sendto_Func",
    "8.7_Datagram_Lost": "8.7_Datagram_Loss_Problem",
    "8.8_Response_Verify": "8.8_Response_Data_Verify",
    "8.9_Server_Not_Running": "8.9_Server_Offline_State",
    "8.11_UDP_Connect": "8.11_UDP_Connect_Usage",
    "8.13_UDP_No_FlowControl": "8.13_UDP_FlowControl_Defect",
    "8.14_UDP_Out_Interface": "8.14_UDP_Outbound_Interface",
    "8.15_TCP_UDP_Select_Server": "8.15_TCP_UDP_Mixed_Server",
}


def chapter_phase(ch: int) -> str:
    for phase, chapters in PHASES.items():
        if ch in chapters:
            return phase
    raise KeyError(ch)


def is_template_notes(path: Path) -> bool:
    if not path.exists():
        return True
    text = path.read_text(encoding="utf-8")
    return text.strip() == NOTES_TEMPLATE.format(title=text.split("\n", 1)[0].removeprefix("# ").strip()).strip()


def ensure_section(chapter: Path, sec: str, title: str) -> None:
    """Flat layout: ChapterXX/3.2_Foo.md and ChapterXX/code/3.2_Foo/{original_c,...}."""
    chapter.mkdir(parents=True, exist_ok=True)
    notes = chapter / f"{sec}.md"
    if not notes.exists():
        notes.write_text(NOTES_TEMPLATE.format(title=title), encoding="utf-8")
    code_root = chapter / "code" / sec
    for sub in ("original_c", "rewrite_go", "rewrite_rust"):
        (code_root / sub).mkdir(parents=True, exist_ok=True)


def migrate_notes(old: Path, chapter: Path, sec: str) -> None:
    """Migrate legacy section folder (notes.md + code/) to flat files."""
    dest = chapter / f"{sec}.md"
    old_notes = old / "notes.md"
    if old_notes.exists() and (not dest.exists() or is_template_notes(dest)):
        if not is_template_notes(old_notes):
            shutil.copy2(old_notes, dest)
    old_code = old / "code"
    new_code = chapter / "code" / sec
    if old_code.exists():
        for sub in old_code.iterdir():
            if sub.is_dir():
                dest_dir = new_code / sub.name
                dest_dir.mkdir(parents=True, exist_ok=True)
                for f in sub.iterdir():
                    if f.is_file() and f.name != ".gitkeep":
                        shutil.copy2(f, dest_dir / f.name)


def remove_orphans() -> int:
    removed = 0
    expected_dirs: set[Path] = set()
    expected_mds: set[Path] = set()
    for phase, chapters in PHASES.items():
        for ch in chapters:
            ch_path = ROOT / phase / CHAPTER_DIRS[ch]
            for sec in SECTIONS[ch]:
                expected_dirs.add(ch_path / sec)
                expected_mds.add(ch_path / f"{sec}.md")

    for phase in ROOT.glob("[0-9]_*"):
        if not phase.is_dir():
            continue
        for ch_dir in phase.glob("Chapter*"):
            for item in list(ch_dir.iterdir()):
                if item.is_dir() and item.name == "code":
                    continue
                if item.is_dir() and item not in expected_dirs:
                    shutil.rmtree(item)
                    removed += 1
                elif (
                    item.is_file()
                    and item.suffix == ".md"
                    and item.name != "study.md"
                    and item not in expected_mds
                ):
                    item.unlink()
                    removed += 1
    return removed


def find_old_section(ch: int, new_name: str) -> Path | None:
    phase = chapter_phase(ch)
    ch_path = ROOT / phase / CHAPTER_DIRS[ch]
    if (ch_path / f"{new_name}.md").exists():
        return None
    if (ch_path / new_name).exists():
        return None
    # direct rename map
    for old, new in RENAME_MAP.items():
        if new == new_name:
            p = ch_path / old
            if p.exists():
                return p
    # prefix match (e.g. 9.1_Introduction -> 9.1_Overview)
    prefix = new_name.split("_", 1)[0]
    for p in ch_path.glob(f"{prefix}_*"):
        if p.is_dir() and p.name != new_name:
            return p
    return None


def generate_outline() -> None:
    lines = [
        "# UNP Vol1 目录索引",
        "",
        "> 由 `scripts/generate_structure.py` 自动生成，与 `SECTIONS` 字典一致。",
        "",
    ]
    total = 0
    for phase, chapters in PHASES.items():
        lines.append(f"## {phase}/")
        lines.append("")
        for ch in chapters:
            ch_dir = CHAPTER_DIRS[ch]
            secs = SECTIONS[ch]
            total += len(secs)
            lines.append(f"### {ch_dir}/ ({len(secs)} sections)")
            lines.append("")
            for sec in secs:
                lines.append(f"- `{sec}.md`")
            lines.append("")
    lines.append(f"**Total: {total} sections**")
    (ROOT / "OUTLINE.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    created = 0
    migrated = 0
    for phase, chapters in PHASES.items():
        (ROOT / phase).mkdir(parents=True, exist_ok=True)
        for ch in chapters:
            ch_path = ROOT / phase / CHAPTER_DIRS[ch]
            ch_path.mkdir(parents=True, exist_ok=True)
            for sec in SECTIONS[ch]:
                old = find_old_section(ch, sec)
                if old and old.is_dir():
                    migrate_notes(old, ch_path, sec)
                    migrated += 1
                title = sec.replace("_", " ")
                ensure_section(ch_path, sec, title)
                created += 1

    orphans = remove_orphans()
    generate_outline()
    total = sum(len(v) for v in SECTIONS.values())
    print(f"Sections: {total} | ensured: {created} | migrated: {migrated} | removed orphans: {orphans}")


if __name__ == "__main__":
    main()
