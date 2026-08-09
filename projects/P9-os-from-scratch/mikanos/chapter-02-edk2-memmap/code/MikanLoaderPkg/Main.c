/**
 * Ch2 MikanLoader — EDK II 版 Hello + GetMemoryMap
 *
 * 本章范围（对齐原书 Day2）：
 *   1. Print(L"Hello, Mikan World!\n")
 *   2. gBS->GetMemoryMap() 拿到物理内存描述符数组
 *   3. 通过 Simple File System 在 ESP 根目录写出 \memmap（CSV）
 *
 * Ch3+ 才加：GOP 帧缓冲、加载 kernel.elf、ExitBootServices、跳内核。
 * 入口 UefiMain 由 UefiApplicationEntryPoint 库包装（见 Loader.inf）。
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include "memory_map.hpp"

// ---------------------------------------------------------------------------
// GetMemoryMap 薄封装 — 把固件 API 填进 MemoryMap 结构
// ---------------------------------------------------------------------------

EFI_STATUS
GetMemoryMapStruct (
  IN OUT struct MemoryMap  *Map
  )
{
  if (Map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Map->map_size = Map->buffer_size;
  return gBS->GetMemoryMap (
           &Map->map_size,
           (EFI_MEMORY_DESCRIPTOR *)Map->buffer,
           &Map->map_key,
           &Map->descriptor_size,
           &Map->descriptor_version
           );
}

// ---------------------------------------------------------------------------
// Type 枚举 → 可读字符串（导出 CSV 第三列）
// ---------------------------------------------------------------------------

CONST CHAR16 *
GetMemoryTypeUnicode (
  IN EFI_MEMORY_TYPE  Type
  )
{
  switch (Type) {
    case EfiReservedMemoryType:       return L"EfiReservedMemoryType";
    case EfiLoaderCode:               return L"EfiLoaderCode";
    case EfiLoaderData:               return L"EfiLoaderData";
    case EfiBootServicesCode:         return L"EfiBootServicesCode";
    case EfiBootServicesData:         return L"EfiBootServicesData";
    case EfiRuntimeServicesCode:      return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:      return L"EfiRuntimeServicesData";
    case EfiConventionalMemory:       return L"EfiConventionalMemory";
    case EfiUnusableMemory:           return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory:        return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:            return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO:           return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace:  return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode:                  return L"EfiPalCode";
    case EfiPersistentMemory:         return L"EfiPersistentMemory";
    case EfiMaxMemoryType:            return L"EfiMaxMemoryType";
    default:                          return L"InvalidMemoryType";
  }
}

// ---------------------------------------------------------------------------
// 遍历描述符数组，写入 CSV（格式见 Ch2 §4 笔记）
// ---------------------------------------------------------------------------

EFI_STATUS
SaveMemoryMapCsv (
  IN struct MemoryMap      *Map,
  IN EFI_FILE_PROTOCOL     *File
  )
{
  EFI_STATUS  Status;
  CHAR8       Line[256];
  UINTN       Len;

  CONST CHAR8  *Header =
    "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  Len    = AsciiStrLen (Header);
  Status = File->Write (File, &Len, (VOID *)Header);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Print (L"map->buffer = %08lx, map->map_size = %08lx\n", Map->buffer, Map->map_size);

  EFI_PHYSICAL_ADDRESS  Iter;
  UINTN                 Index;

  for (Iter = (EFI_PHYSICAL_ADDRESS)Map->buffer, Index = 0;
       Iter < (EFI_PHYSICAL_ADDRESS)Map->buffer + Map->map_size;
       Iter += Map->descriptor_size, Index++)
  {
    EFI_MEMORY_DESCRIPTOR  *Desc = (EFI_MEMORY_DESCRIPTOR *)Iter;

    Len = AsciiSPrint (
            Line,
            sizeof (Line),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            Index,
            Desc->Type,
            GetMemoryTypeUnicode (Desc->Type),
            Desc->PhysicalStart,
            Desc->NumberOfPages,
            Desc->Attribute & 0xffffflu
            );
    Status = File->Write (File, &Len, Line);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

// ---------------------------------------------------------------------------
// 打开 Loader 所在 ESP 的根目录（FAT 卷根）
// ---------------------------------------------------------------------------

EFI_STATUS
OpenRootDir (
  IN  EFI_HANDLE            ImageHandle,
  OUT EFI_FILE_PROTOCOL     **Root
  )
{
  EFI_STATUS                       Status;
  EFI_LOADED_IMAGE_PROTOCOL        *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;

  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&Fs,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Fs->OpenVolume (Fs, Root);
}

// ---------------------------------------------------------------------------

VOID
Halt (
  VOID
  )
{
  while (1) {
    __asm__ __volatile__ ("hlt");
  }
}

// ---------------------------------------------------------------------------
// UefiMain — Ch2 主线
// ---------------------------------------------------------------------------

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Print (L"Hello, Mikan World!\n");

  //
  // 1. 向固件要内存图 — buffer 在栈上，Boot 期临时用
  //
  CHAR8             MemmapBuf[4096 * 4];
  struct MemoryMap  Memmap = {
    sizeof (MemmapBuf),
    MemmapBuf,
    0, 0, 0, 0
  };

  Status = GetMemoryMapStruct (&Memmap);
  if (EFI_ERROR (Status)) {
    Print (L"failed to get memory map: %r\n", Status);
    Halt ();
  }

  //
  // 2. 打开 ESP 根目录，创建 \memmap
  //
  EFI_FILE_PROTOCOL  *RootDir;
  Status = OpenRootDir (ImageHandle, &RootDir);
  if (EFI_ERROR (Status)) {
    Print (L"failed to open root directory: %r\n", Status);
    Halt ();
  }

  EFI_FILE_PROTOCOL  *MemmapFile;
  Status = RootDir->Open (
                      RootDir,
                      &MemmapFile,
                      L"\\memmap",
                      EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                      0
                      );
  if (EFI_ERROR (Status)) {
    Print (L"failed to open file '\\memmap': %r\n", Status);
    Halt ();
  }

  Status = SaveMemoryMapCsv (&Memmap, MemmapFile);
  if (EFI_ERROR (Status)) {
    Print (L"failed to save memory map: %r\n", Status);
    Halt ();
  }

  Status = MemmapFile->Close (MemmapFile);
  if (EFI_ERROR (Status)) {
    Print (L"failed to close memmap file: %r\n", Status);
    Halt ();
  }

  Print (L"memmap saved. Ch2 done — Ch3+ adds GOP / kernel.elf / ExitBootServices.\n");

  Halt ();
  return EFI_SUCCESS;  // unreachable
}
