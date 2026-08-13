#!/usr/bin/env python3
"""Complete gap analysis: book TOC vs existing notes for all 64 chapters."""
import os, json

# Full book TOC extracted from PDF outline
BOOK_TOC = {
1: [("1.1","A Brief History of UNIX and C"),("1.2","A Brief History of Linux"),("1.3","Standardization"),("1.4","Summary")],
2: [("2.1","The Core Operating System: The Kernel"),("2.2","The Shell"),("2.3","Users and Groups"),("2.4","Single Directory Hierarchy, Directories, Links, and Files"),("2.5","File I/O Model"),("2.6","Programs"),("2.7","Processes"),("2.8","Memory Mappings"),("2.9","Static and Shared Libraries"),("2.10","Interprocess Communication and Synchronization"),("2.11","Signals"),("2.12","Threads"),("2.13","Process Groups and Shell Job Control"),("2.14","Sessions, Controlling Terminals, and Controlling Processes"),("2.15","Pseudoterminals"),("2.16","Date and Time"),("2.17","Client-Server Architecture"),("2.18","Realtime"),("2.19","The /proc File System"),("2.20","Summary")],
3: [("3.1","System Calls"),("3.2","Library Functions"),("3.3","The Standard C Library; The GNU C Library (glibc)"),("3.4","Handling Errors from System Calls and Library Functions"),("3.5","Notes on the Example Programs in This Book"),("3.6","Portability Issues"),("3.7","Summary"),("3.8","Exercise")],
4: [("4.1","Overview"),("4.2","Universality of I/O"),("4.3","Opening a File: open()"),("4.4","Reading from a File: read()"),("4.5","Writing to a File: write()"),("4.6","Closing a File: close()"),("4.7","Changing the File Offset: lseek()"),("4.8","Operations Outside the Universal I/O Model: ioctl()"),("4.9","Summary"),("4.10","Exercises")],
5: [("5.1","Atomicity and Race Conditions"),("5.2","File Control Operations: fcntl()"),("5.3","Open File Status Flags"),("5.4","Relationship Between File Descriptors and Open Files"),("5.5","Duplicating File Descriptors"),("5.6","File I/O at a Specified Offset: pread() and pwrite()"),("5.7","Scatter-Gather I/O: readv() and writev()"),("5.8","Truncating a File: truncate() and ftruncate()"),("5.9","Nonblocking I/O"),("5.10","I/O on Large Files"),("5.11","The /dev/fd Directory"),("5.12","Creating Temporary Files"),("5.13","Summary"),("5.14","Exercises")],
6: [("6.1","Processes and Programs"),("6.2","Process ID and Parent Process ID"),("6.3","Memory Layout of a Process"),("6.4","Virtual Memory Management"),("6.5","The Stack and Stack Frames"),("6.6","Command-Line Arguments (argc, argv)"),("6.7","Environment List"),("6.8","Performing a Nonlocal Goto: setjmp() and longjmp()"),("6.9","Summary"),("6.10","Exercises")],
7: [("7.1","Allocating Memory on the Heap"),("7.2","Allocating Memory on the Stack: alloca()"),("7.3","Summary"),("7.4","Exercises")],
8: [("8.1","The Password File: /etc/passwd"),("8.2","The Shadow Password File: /etc/shadow"),("8.3","The Group File: /etc/group"),("8.4","Retrieving User and Group Information"),("8.5","Password Encryption and User Authentication"),("8.6","Summary"),("8.7","Exercises")],
9: [("9.1","Real User ID and Real Group ID"),("9.2","Effective User ID and Effective Group ID"),("9.3","Set-User-ID and Set-Group-ID Programs"),("9.4","Saved Set-User-ID and Saved Set-Group-ID"),("9.5","File-System User ID and File-System Group ID"),("9.6","Supplementary Group IDs"),("9.7","Retrieving and Modifying Process Credentials"),("9.8","Summary"),("9.9","Exercises")],
10: [("10.1","Calendar Time"),("10.2","Time-Conversion Functions"),("10.3","Timezones"),("10.4","Locales"),("10.5","Updating the System Clock"),("10.6","The Software Clock (Jiffies)"),("10.7","Process Time"),("10.8","Summary"),("10.9","Exercise")],
11: [("11.1","System Limits"),("11.2","Retrieving System Limits (and Options) at Run Time"),("11.3","Retrieving File-Related Limits (and Options) at Run Time"),("11.4","Indeterminate Limits"),("11.5","System Options"),("11.6","Summary"),("11.7","Exercises")],
12: [("12.1","The /proc File System"),("12.2","System Identification: uname()"),("12.3","Summary"),("12.4","Exercises")],
13: [("13.1","Kernel Buffering of File I/O: The Buffer Cache"),("13.2","Buffering in the stdio Library"),("13.3","Controlling Kernel Buffering of File I/O"),("13.4","Summary of I/O Buffering"),("13.5","Advising the Kernel About I/O Patterns"),("13.6","Bypassing the Buffer Cache: Direct I/O"),("13.7","Mixing Library Functions and System Calls for File I/O"),("13.8","Summary"),("13.9","Exercises")],
14: [("14.1","Device Special Files (Devices)"),("14.2","Disks and Partitions"),("14.3","File Systems"),("14.4","I-nodes"),("14.5","The Virtual File System (VFS)"),("14.6","Journaling File Systems"),("14.7","Single Directory Hierarchy and Mount Points"),("14.8","Mounting and Unmounting File Systems"),("14.9","Advanced Mount Features"),("14.10","A Virtual Memory File System: tmpfs"),("14.11","Obtaining Information About a File System: statvfs()"),("14.12","Summary"),("14.13","Exercise")],
15: [("15.1","Retrieving File Information: stat()"),("15.2","File Timestamps"),("15.3","File Ownership"),("15.4","File Permissions"),("15.5","I-node Flags (ext2 Extended File Attributes)"),("15.6","Summary"),("15.7","Exercises")],
16: [("16.1","Overview"),("16.2","Extended Attribute Implementation Details"),("16.3","System Calls for Manipulating Extended Attributes"),("16.4","Summary"),("16.5","Exercise")],
17: [("17.1","Overview"),("17.2","ACL Permission-Checking Algorithm"),("17.3","Long and Short Text Forms for ACLs"),("17.4","The ACL_MASK Entry and the ACL Group Class"),("17.5","The getfacl and setfacl Commands"),("17.6","Default ACLs and File Creation"),("17.7","ACL Implementation Limits"),("17.8","The ACL API"),("17.9","Summary"),("17.10","Exercise")],
18: [("18.1","Directories and (Hard) Links"),("18.2","Symbolic (Soft) Links"),("18.3","Creating and Removing (Hard) Links: link() and unlink()"),("18.4","Changing the Name of a File: rename()"),("18.5","Working with Symbolic Links: symlink() and readlink()"),("18.6","Creating and Removing Directories: mkdir() and rmdir()"),("18.7","Removing a File or Directory: remove()"),("18.8","Reading Directories: opendir() and readdir()"),("18.9","File Tree Walking: nftw()"),("18.10","The Current Working Directory of a Process"),("18.11","Operating Relative to a Directory File Descriptor"),("18.12","Changing the Root Directory of a Process: chroot()"),("18.13","Resolving a Pathname: realpath()"),("18.14","Parsing Pathname Strings: dirname() and basename()"),("18.15","Summary"),("18.16","Exercises")],
19: [("19.1","Overview"),("19.2","The inotify API"),("19.3","inotify Events"),("19.4","Reading inotify Events"),("19.5","Queue Limits and /proc Files"),("19.6","An Older System for Monitoring File Events: dnotify"),("19.7","Summary"),("19.8","Exercise")],
20: [("20.1","Concepts and Overview"),("20.2","Signal Types and Default Actions"),("20.3","Changing Signal Dispositions: signal()"),("20.4","Introduction to Signal Handlers"),("20.5","Sending Signals: kill()"),("20.6","Checking for the Existence of a Process"),("20.7","Other Ways of Sending Signals: raise() and killpg()"),("20.8","Displaying Signal Descriptions"),("20.9","Signal Sets"),("20.10","The Signal Mask (Blocking Signal Delivery)"),("20.11","Pending Signals"),("20.12","Signals Are Not Queued"),("20.13","Changing Signal Dispositions: sigaction()"),("20.14","Waiting for a Signal: pause()"),("20.15","Summary"),("20.16","Exercises")],
21: [("21.1","Designing Signal Handlers"),("21.2","Other Methods of Terminating a Signal Handler"),("21.3","Handling a Signal on an Alternate Stack: sigaltstack()"),("21.4","The SA_SIGINFO Flag"),("21.5","Interruption and Restarting of System Calls"),("21.6","Summary"),("21.7","Exercise")],
22: [("22.1","Core Dump Files"),("22.2","Special Cases for Delivery, Disposition, and Handling"),("22.3","Interruptible and Uninterruptible Process Sleep States"),("22.4","Hardware-Generated Signals"),("22.5","Synchronous and Asynchronous Signal Generation"),("22.6","Timing and Order of Signal Delivery"),("22.7","Implementation and Portability of signal()"),("22.8","Realtime Signals"),("22.9","Waiting for a Signal Using a Mask: sigsuspend()"),("22.10","Synchronously Waiting for a Signal"),("22.11","Fetching Signals via a File Descriptor"),("22.12","Interprocess Communication with Signals"),("22.13","Earlier Signal APIs (System V and BSD)"),("22.14","Summary"),("22.15","Exercises")],
23: [("23.1","Interval Timers"),("23.2","Scheduling and Accuracy of Timers"),("23.3","Setting Timeouts on Blocking Operations"),("23.4","Suspending Execution for a Fixed Interval (Sleeping)"),("23.5","POSIX Clocks"),("23.6","POSIX Interval Timers"),("23.7","Timers That Notify via File Descriptors: the timerfd API"),("23.8","Summary"),("23.9","Exercises")],
24: [("24.1","Overview of fork(), exit(), wait(), and execve()"),("24.2","Creating a New Process: fork()"),("24.3","The vfork() System Call"),("24.4","Race Conditions After fork()"),("24.5","Avoiding Race Conditions by Synchronizing with Signals"),("24.6","Summary"),("24.7","Exercises")],
25: [("25.1","Terminating a Process: _exit() and exit()"),("25.2","Details of Process Termination"),("25.3","Exit Handlers"),("25.4","Interactions Between fork(), stdio Buffers, and _exit()"),("25.5","Summary"),("25.6","Exercise")],
26: [("26.1","Waiting on a Child Process"),("26.2","Orphans and Zombies"),("26.3","The SIGCHLD Signal"),("26.4","Summary"),("26.5","Exercises")],
27: [("27.1","Executing a New Program: execve()"),("27.2","The exec() Library Functions"),("27.3","Interpreter Scripts"),("27.4","File Descriptors and exec()"),("27.5","Signals and exec()"),("27.6","Executing a Shell Command: system()"),("27.7","Implementing system()"),("27.8","Summary"),("27.9","Exercises")],
28: [("28.1","Process Accounting"),("28.2","The clone() System Call"),("28.3","Speed of Process Creation"),("28.4","Effect of exec() and fork() on Process Attributes"),("28.5","Summary"),("28.6","Exercise")],
29: [("29.1","Overview"),("29.2","Background Details of the Pthreads API"),("29.3","Thread Creation"),("29.4","Thread Termination"),("29.5","Thread IDs"),("29.6","Joining with a Terminated Thread"),("29.7","Detaching a Thread"),("29.8","Thread Attributes"),("29.9","Threads Versus Processes"),("29.10","Summary"),("29.11","Exercises")],
30: [("30.1","Protecting Accesses to Shared Variables: Mutexes"),("30.2","Signaling Changes of State: Condition Variables"),("30.3","Summary"),("30.4","Exercises")],
31: [("31.1","Thread Safety (and Reentrancy Revisited)"),("31.2","One-Time Initialization"),("31.3","Thread-Specific Data"),("31.4","Thread-Local Storage"),("31.5","Summary"),("31.6","Exercises")],
32: [("32.1","Canceling a Thread"),("32.2","Cancellation State and Type"),("32.3","Cancellation Points"),("32.4","Testing for Thread Cancellation"),("32.5","Cleanup Handlers"),("32.6","Asynchronous Cancelability"),("32.7","Summary")],
33: [("33.1","Thread Stacks"),("33.2","Threads and Signals"),("33.3","Threads and Process Control"),("33.4","Thread Implementation Models"),("33.5","Linux Implementations of POSIX Threads"),("33.6","Advanced Features of the Pthreads API"),("33.7","Summary"),("33.8","Exercises")],
34: [("34.1","Overview"),("34.2","Process Groups"),("34.3","Sessions"),("34.4","Controlling Terminals and Controlling Processes"),("34.5","Foreground and Background Process Groups"),("34.6","The SIGHUP Signal"),("34.7","Job Control"),("34.8","Summary"),("34.9","Exercises")],
35: [("35.1","Process Priorities (Nice Values)"),("35.2","Overview of Realtime Process Scheduling"),("35.3","Realtime Process Scheduling API"),("35.4","CPU Affinity"),("35.5","Summary"),("35.6","Exercises")],
36: [("36.1","Process Resource Usage"),("36.2","Process Resource Limits"),("36.3","Details of Specific Resource Limits"),("36.4","Summary"),("36.5","Exercises")],
37: [("37.1","Overview"),("37.2","Creating a Daemon"),("37.3","Guidelines for Writing Daemons"),("37.4","Using SIGHUP to Reinitialize a Daemon"),("37.5","Logging Messages and Errors Using syslog"),("37.6","Summary"),("37.7","Exercise")],
38: [("38.1","Is a Set-User-ID or Set-Group-ID Program Required?"),("38.2","Operate with Least Privilege"),("38.3","Be Careful When Executing a Program"),("38.4","Avoid Exposing Sensitive Information"),("38.5","Confine the Process"),("38.6","Beware of Signals and Race Conditions"),("38.7","Pitfalls When Performing File Operations and File I/O"),("38.8","Don't Trust Inputs or the Environment"),("38.9","Beware of Buffer Overruns"),("38.10","Beware of Denial-of-Service Attacks"),("38.11","Check Return Statuses and Fail Safely"),("38.12","Summary"),("38.13","Exercises")],
39: [("39.1","Rationale for Capabilities"),("39.2","The Linux Capabilities"),("39.3","Process and File Capabilities"),("39.4","The Modern Capabilities Implementation"),("39.5","Transformation of Process Capabilities During exec()"),("39.6","Effect on Process Capabilities of Changing User IDs"),("39.7","Changing Process Capabilities Programmatically"),("39.8","Creating Capabilities-Only Environments"),("39.9","Discovering the Capabilities Required by a Program"),("39.10","Older Kernels and Systems Without File Capabilities"),("39.11","Summary"),("39.12","Exercise")],
40: [("40.1","Overview of the utmp and wtmp Files"),("40.2","The utmpx API"),("40.3","The utmpx Structure"),("40.4","Retrieving Information from the utmp and wtmp Files"),("40.5","Retrieving the Login Name: getlogin()"),("40.6","Updating the utmp and wtmp Files for a Login Session"),("40.7","The lastlog File"),("40.8","Summary"),("40.9","Exercises")],
41: [("41.1","Object Libraries"),("41.2","Static Libraries"),("41.3","Overview of Shared Libraries"),("41.4","Creating and Using Shared Libraries-A First Pass"),("41.5","Useful Tools for Working with Shared Libraries"),("41.6","Shared Library Versions and Naming Conventions"),("41.7","Installing Shared Libraries"),("41.8","Compatible Versus Incompatible Libraries"),("41.9","Upgrading Shared Libraries"),("41.10","Specifying Library Search Directories in an Object File"),("41.11","Finding Shared Libraries at Run Time"),("41.12","Run-Time Symbol Resolution"),("41.13","Using a Static Library Instead of a Shared Library"),("41.14","Summary"),("41.15","Exercise")],
42: [("42.1","Dynamically Loaded Libraries"),("42.2","Controlling Symbol Visibility"),("42.3","Linker Version Scripts"),("42.4","Initialization and Finalization Functions"),("42.5","Preloading Shared Libraries"),("42.6","Monitoring the Dynamic Linker: LD_DEBUG"),("42.7","Summary"),("42.8","Exercises")],
43: [("43.1","A Taxonomy of IPC Facilities"),("43.2","Communication Facilities"),("43.3","Synchronization Facilities"),("43.4","Comparing IPC Facilities"),("43.5","Summary"),("43.6","Exercises")],
44: [("44.1","Overview"),("44.2","Creating and Using Pipes"),("44.3","Pipes as a Method of Process Synchronization"),("44.4","Using Pipes to Connect Filters"),("44.5","Talking to a Shell Command via a Pipe: popen()"),("44.6","Pipes and stdio Buffering"),("44.7","FIFOs"),("44.8","A Client-Server Application Using FIFOs"),("44.9","Nonblocking I/O"),("44.10","Semantics of read() and write() on Pipes and FIFOs"),("44.11","Summary"),("44.12","Exercises")],
45: [("45.1","API Overview"),("45.2","IPC Keys"),("45.3","Associated Data Structure and Object Permissions"),("45.4","IPC Identifiers and Client-Server Applications"),("45.5","Algorithm Employed by System V IPC get Calls"),("45.6","The ipcs and ipcrm Commands"),("45.7","Obtaining a List of All IPC Objects"),("45.8","IPC Limits"),("45.9","Summary"),("45.10","Exercises")],
46: [("46.1","Creating or Opening a Message Queue"),("46.2","Exchanging Messages"),("46.3","Message Queue Control Operations"),("46.4","Message Queue Associated Data Structure"),("46.5","Message Queue Limits"),("46.6","Displaying All Message Queues on the System"),("46.7","Client-Server Programming with Message Queues"),("46.8","A File-Server Application Using Message Queues"),("46.9","Disadvantages of System V Message Queues"),("46.10","Summary"),("46.11","Exercises")],
47: [("47.1","Overview"),("47.2","Creating or Opening a Semaphore Set"),("47.3","Semaphore Control Operations"),("47.4","Semaphore Associated Data Structure"),("47.5","Semaphore Initialization"),("47.6","Semaphore Operations"),("47.7","Handling of Multiple Blocked Semaphore Operations"),("47.8","Semaphore Undo Values"),("47.9","Implementing a Binary Semaphores Protocol"),("47.10","Semaphore Limits"),("47.11","Disadvantages of System V Semaphores"),("47.12","Summary"),("47.13","Exercises")],
48: [("48.1","Overview"),("48.2","Creating or Opening a Shared Memory Segment"),("48.3","Using Shared Memory"),("48.4","Example: Transferring Data via Shared Memory"),("48.5","Location of Shared Memory in Virtual Memory"),("48.6","Storing Pointers in Shared Memory"),("48.7","Shared Memory Control Operations"),("48.8","Shared Memory Associated Data Structure"),("48.9","Shared Memory Limits"),("48.10","Summary"),("48.11","Exercises")],
49: [("49.1","Overview"),("49.2","Creating a Mapping: mmap()"),("49.3","Unmapping a Mapped Region: munmap()"),("49.4","File Mappings"),("49.5","Synchronizing a Mapped Region: msync()"),("49.6","Additional mmap() Flags"),("49.7","Anonymous Mappings"),("49.8","Remapping a Mapped Region: mremap()"),("49.9","MAP_NORESERVE and Swap Space Overcommitting"),("49.10","The MAP_FIXED Flag"),("49.11","Nonlinear Mappings: remap_file_pages()"),("49.12","Summary"),("49.13","Exercises")],
50: [("50.1","Changing Memory Protection: mprotect()"),("50.2","Memory Locking: mlock() and mlockall()"),("50.3","Determining Memory Residence: mincore()"),("50.4","Advising Future Memory Usage Patterns: madvise()"),("50.5","Summary"),("50.6","Exercises")],
51: [("51.1","API Overview"),("51.2","Comparison of System V IPC and POSIX IPC"),("51.3","Summary")],
52: [("52.1","Overview"),("52.2","Opening, Closing, and Unlinking a Message Queue"),("52.3","Relationship Between Descriptors and Message Queues"),("52.4","Message Queue Attributes"),("52.5","Exchanging Messages"),("52.6","Message Notification"),("52.7","Linux-Specific Features"),("52.8","Message Queue Limits"),("52.9","Comparison of POSIX and System V Message Queues"),("52.10","Summary"),("52.11","Exercises")],
53: [("53.1","Overview"),("53.2","Named Semaphores"),("53.3","Semaphore Operations"),("53.4","Unnamed Semaphores"),("53.5","Comparisons with Other Synchronization Techniques"),("53.6","Semaphore Limits"),("53.7","Summary"),("53.8","Exercises")],
54: [("54.1","Overview"),("54.2","Creating Shared Memory Objects"),("54.3","Using Shared Memory Objects"),("54.4","Removing Shared Memory Objects"),("54.5","Comparisons Between Shared Memory APIs"),("54.6","Summary"),("54.7","Exercise")],
55: [("55.1","Overview"),("55.2","File Locking with flock()"),("55.3","Record Locking with fcntl()"),("55.4","Mandatory Locking"),("55.5","The /proc/locks File"),("55.6","Running Just One Instance of a Program"),("55.7","Older Locking Techniques"),("55.8","Summary"),("55.9","Exercises")],
56: [("56.1","Overview"),("56.2","Creating a Socket: socket()"),("56.3","Binding a Socket to an Address: bind()"),("56.4","Generic Socket Address Structures: struct sockaddr"),("56.5","Stream Sockets"),("56.6","Datagram Sockets"),("56.7","Summary")],
57: [("57.1","UNIX Domain Socket Addresses: struct sockaddr_un"),("57.2","Stream Sockets in the UNIX Domain"),("57.3","Datagram Sockets in the UNIX Domain"),("57.4","UNIX Domain Socket Permissions"),("57.5","Creating a Connected Socket Pair: socketpair()"),("57.6","The Linux Abstract Socket Namespace"),("57.7","Summary"),("57.8","Exercises")],
58: [("58.1","Internets"),("58.2","Networking Protocols and Layers"),("58.3","The Data-Link Layer"),("58.4","The Network Layer: IP"),("58.5","IP Addresses"),("58.6","The Transport Layer"),("58.7","Requests for Comments (RFCs)"),("58.8","Summary")],
59: [("59.1","Internet Domain Sockets"),("59.2","Network Byte Order"),("59.3","Data Representation"),("59.4","Internet Socket Addresses"),("59.5","Overview of Host and Service Conversion Functions"),("59.6","The inet_pton() and inet_ntop() Functions"),("59.7","Client-Server Example (Datagram Sockets)"),("59.8","Domain Name System (DNS)"),("59.9","The /etc/services File"),("59.10","Protocol-Independent Host and Service Conversion"),("59.11","Client-Server Example (Stream Sockets)"),("59.12","An Internet Domain Sockets Library"),("59.13","Obsolete APIs for Host and Service Conversions"),("59.14","UNIX Versus Internet Domain Sockets"),("59.15","Further Information"),("59.16","Summary"),("59.17","Exercises")],
60: [("60.1","Iterative and Concurrent Servers"),("60.2","An Iterative UDP echo Server"),("60.3","A Concurrent TCP echo Server"),("60.4","Other Concurrent Server Designs"),("60.5","The inetd (Internet Superserver) Daemon"),("60.6","Summary"),("60.7","Exercises")],
61: [("61.1","Partial Reads and Writes on Stream Sockets"),("61.2","The shutdown() System Call"),("61.3","Socket-Specific I/O System Calls: recv() and send()"),("61.4","The sendfile() System Call"),("61.5","Retrieving Socket Addresses"),("61.6","A Closer Look at TCP"),("61.7","Monitoring Sockets: netstat"),("61.8","Using tcpdump to Monitor TCP Traffic"),("61.9","Socket Options"),("61.10","The SO_REUSEADDR Socket Option"),("61.11","Inheritance of Flags and Options Across accept()"),("61.12","TCP Versus UDP"),("61.13","Advanced Features"),("61.14","Summary"),("61.15","Exercises")],
62: [("62.1","Overview"),("62.2","Retrieving and Modifying Terminal Attributes"),("62.3","The stty Command"),("62.4","Terminal Special Characters"),("62.5","Terminal Flags"),("62.6","Terminal I/O Modes"),("62.7","Terminal Line Speed (Bit Rate)"),("62.8","Terminal Line Control"),("62.9","Terminal Window Size"),("62.10","Terminal Identification"),("62.11","Summary"),("62.12","Exercises")],
63: [("63.1","Overview"),("63.2","I/O Multiplexing"),("63.3","Signal-Driven I/O"),("63.4","The epoll API"),("63.5","Waiting on Signals and File Descriptors"),("63.6","Summary"),("63.7","Exercises")],
64: [("64.1","Overview"),("64.2","UNIX 98 Pseudoterminals"),("64.3","Opening a Master: ptyMasterOpen()"),("64.4","Connecting Processes with a Pseudoterminal: ptyFork()"),("64.5","Pseudoterminal I/O"),("64.6","Implementing script(1)"),("64.7","Terminal Attributes and Window Size"),("64.8","BSD Pseudoterminals"),("64.9","Summary"),("64.10","Exercises")],
}

# Chapter directory mapping
DIR_MAP = {
1:'chapter-01-introduction',2:'chapter-02-basic-concepts',3:'chapter-03-system-programming-concepts',
4:'chapter-04-file-io-universal',5:'chapter-05-file-io-further',6:'chapter-06-processes',
7:'chapter-07-memory-allocation',8:'chapter-08-users-and-groups',9:'chapter-09-process-credentials',
10:'chapter-10-time',11:'chapter-11-system-limits',12:'chapter-12-system-process-info',
13:'chapter-13-file-io-buffering',14:'chapter-14-file-systems',15:'chapter-15-file-attributes',
16:'chapter-16-extended-attributes',17:'chapter-17-access-control-lists',18:'chapter-18-directories-links',
19:'chapter-19-monitoring-file-events',20:'chapter-20-signals-fundamentals',21:'chapter-21-signal-handlers',
22:'chapter-22-signals-advanced',23:'chapter-23-timers-sleeping',24:'chapter-24-process-creation',
25:'chapter-25-process-termination',26:'chapter-26-monitoring-child-processes',27:'chapter-27-program-execution',
28:'chapter-28-process-creation-exec-detail',29:'chapter-29-threads-intro',30:'chapter-30-thread-synchronization',
31:'chapter-31-thread-safety-tsd',32:'chapter-32-thread-cancellation',33:'chapter-33-threads-further',
34:'chapter-34-process-groups-sessions',35:'chapter-35-process-priorities-scheduling',36:'chapter-36-process-resources',
37:'chapter-37-daemons',38:'chapter-38-secure-privileged',39:'chapter-39-capabilities',
40:'chapter-40-login-accounting',41:'chapter-41-shared-libraries',42:'chapter-42-shared-libraries-advanced',
43:'chapter-43-ipc-overview',44:'chapter-44-pipes-fifos',45:'chapter-45-sysv-ipc-intro',
46:'chapter-46-sysv-message-queues',47:'chapter-47-sysv-semaphores',48:'chapter-48-sysv-shared-memory',
49:'chapter-49-memory-mappings',50:'chapter-50-virtual-memory',51:'chapter-51-posix-ipc-intro',
52:'chapter-52-posix-message-queues',53:'chapter-53-posix-semaphores',54:'chapter-54-posix-shared-memory',
55:'chapter-55-file-locking',56:'chapter-56-sockets-intro',57:'chapter-57-sockets-unix-domain',
58:'chapter-58-tcpip-fundamentals',59:'chapter-59-internet-domains',60:'chapter-60-server-design',
61:'chapter-61-sockets-advanced',62:'chapter-62-terminals',63:'chapter-63-alternative-io',
64:'chapter-64-pseudoterminals',
}

BASE = 'C:/Users/12392/Desktop/hft/03-linux-userspace-api'

total_book = 0
total_notes = 0
total_missing = 0
results = []

for ch in range(1, 65):
    sections = BOOK_TOC[ch]
    notes_dir = os.path.join(BASE, DIR_MAP[ch], 'notes')
    if os.path.isdir(notes_dir):
        notes = sorted([f for f in os.listdir(notes_dir) if f.endswith('.md')])
    else:
        notes = []

    book_count = len(sections)
    note_count = len(notes)
    missing = book_count - note_count
    total_book += book_count
    total_notes += note_count
    total_missing += max(0, missing)

    status = "OK" if missing <= 0 else f"MISSING {missing}"
    results.append((ch, book_count, note_count, missing, status))

    if missing > 0:
        print(f"Ch{ch:2d}: book={book_count:2d} notes={note_count:2d} {status}")
        for sec_num, sec_title in sections:
            print(f"  {sec_num:6s} {sec_title}")
        print(f"  --- existing notes ---")
        for n in notes:
            print(f"  {n}")
        print()

print(f"\n=== TOTALS ===")
print(f"Book sections: {total_book}")
print(f"Existing notes: {total_notes}")
print(f"Missing: {total_missing}")
print(f"Coverage: {total_notes}/{total_book} = {total_notes*100/total_book:.1f}%")
