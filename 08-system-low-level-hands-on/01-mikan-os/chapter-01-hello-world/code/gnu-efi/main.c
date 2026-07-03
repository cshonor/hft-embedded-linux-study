#include <Uefi.h>

EFI_STATUS EFIAPI efi_main(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
  SystemTable->ConOut->OutputString(
      SystemTable->ConOut,
      L"Bare C BOOTX64.EFI Hello World!\r\n");
  return EFI_SUCCESS;
}
