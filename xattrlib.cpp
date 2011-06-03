#include "stdafx.h"
#include "xattrlib.h"

static HANDLE OpenFileForWrite(IN LPCWSTR sFileName, IN BOOL bBackup)
{
  return CreateFile(
    sFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
    (bBackup)
    ? FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS
    : FILE_FLAG_OPEN_REPARSE_POINT, 0);
}

static HANDLE OpenFileForRead(IN LPCWSTR sFileName, IN BOOL bBackup)
{
  return CreateFile(
    sFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
    (bBackup)
    ? FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS
    : FILE_FLAG_OPEN_REPARSE_POINT, 0);
}

static BOOL WriteExtendedAttributeInternal(
  IN LPCWSTR sFileName,
  IN LPCSTR  sAttrName, // ASCII, not UNICODE!
  IN PVOID   pBuf,
  IN UINT    puBufLen,
  IN BYTE    cFlags
);

XATTRLIB_API BOOL GetExtendedAttributesList(
  IN  LPCWSTR sFileName,
  OUT PFILE_FULL_EA_INFORMATION pAttributeList,
  IN  ULONG puAttributeListLength
)
{
  IO_STATUS_BLOCK iosb;
  NTSTATUS ntst;

  // Buffer check
  if (NULL == pAttributeList || 0 == puAttributeListLength)
  {
    return FALSE;
  }

  // Open for read. 'Backup' mode if opening a directory
  HANDLE hFile = OpenFileForRead(sFileName,
    (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_DIRECTORY));
  
  if (INVALID_HANDLE_VALUE == hFile)
  {
    return FALSE;
  }

  // Return multiple entries
  ntst =  NtQueryEaFile(hFile, &iosb, pAttributeList,
                    puAttributeListLength, FALSE, NULL, NULL, NULL, FALSE);

  CloseHandle(hFile);
  return NT_SUCCESS(ntst);
}

XATTRLIB_API BOOL ReadExtendedAttribute(
  IN LPCWSTR sFileName,
  IN LPCSTR  sEaName,  // ASCII, not UNICODE!
  OUT PVOID  pBuf,
  OUT PUINT  puBufLen,
  OUT PBYTE  cFlags
)
{
  IO_STATUS_BLOCK iosb;
  NTSTATUS ntst;
  
  // Buffer check
  if (NULL == pBuf || NULL == puBufLen || NULL == cFlags)
  {
    return FALSE;
  }

  // Open for read. 'Backup' mode if opening a directory
  HANDLE hFile = OpenFileForRead(sFileName,
    (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_DIRECTORY));

  if (INVALID_HANDLE_VALUE == hFile)
  {
    return FALSE;
  }

  // Mem alloc for EA
  PFILE_FULL_EA_INFORMATION ffei
    = (PFILE_FULL_EA_INFORMATION) GlobalAlloc(GPTR, MAX_EA_LENGTH);

  while (TRUE)
  {
    // Return single entry mode
    ntst =  NtQueryEaFile(hFile, &iosb, ffei,
                      MAX_EA_LENGTH, TRUE, NULL, NULL, NULL, FALSE);
    if (!NT_SUCCESS(ntst))
    {
      break;
    }
    if (0 == _strnicmp(ffei->EaName, sEaName, MAX_EA_NAME))
    {
      // Copy attribute value
      memcpy(pBuf, ffei->EaName + ffei->EaNameLength + 1, ffei->EaValueLength);
      
      // Return a value length
      *puBufLen = ffei->EaValueLength;
      *cFlags = ffei->Flags;

      GlobalFree(ffei);
      CloseHandle(hFile);
      return TRUE;
    }
  }

  GlobalFree(ffei);
  CloseHandle(hFile);
  return FALSE;
}

XATTRLIB_API BOOL DeleteExtendedAttribute(IN LPCWSTR sFileName, IN LPCSTR sAttrName)
{
  return WriteExtendedAttributeInternal(sFileName, sAttrName, NULL, 0, 0);
}

XATTRLIB_API BOOL WriteExtendedAttribute(
  IN LPCWSTR sFileName,
  IN LPCSTR  sAttrName, // ASCII, not UNICODE!
  IN PVOID   pBuf,
  IN UINT    puBufLen,
  IN BYTE    cFlags
)
{
  return (NULL == pBuf) ? FALSE : WriteExtendedAttributeInternal(
    sFileName,
    sAttrName,
    pBuf,
    puBufLen,
    cFlags
  );
}

static BOOL WriteExtendedAttributeInternal(
  IN LPCWSTR sFileName,
  IN LPCSTR  sAttrName, // ASCII, not UNICODE!
  IN PVOID   pBuf,
  IN UINT    puBufLen,
  IN BYTE    cFlags
)
{
  IO_STATUS_BLOCK iosb;
  NTSTATUS ntst;

  // Attribute name length check
  if (strlen(sAttrName) > MAX_EA_NAME)
  {
    return FALSE;
  }

  // TODO: make a proper length check
  if (puBufLen > MAX_EA_LENGTH)
  {
    return FALSE;
  }

  HANDLE hFile = OpenFileForWrite(sFileName,
    (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_DIRECTORY));

  if (INVALID_HANDLE_VALUE == hFile)
  {
    return FALSE;
  }

  // Mem alloc for EA
  PFILE_FULL_EA_INFORMATION ffei
    = (PFILE_FULL_EA_INFORMATION) GlobalAlloc(GPTR, MAX_EA_LENGTH);

  ffei->EaNameLength = (UCHAR) strlen(sAttrName);
  ffei->EaValueLength = puBufLen;
  ffei->Flags = cFlags;

  // Copy attribute name
  memcpy(ffei->EaName, sAttrName, ffei->EaNameLength);

  // Copy attribute value
  memcpy(ffei->EaName + ffei->EaNameLength + 1, pBuf, ffei->EaValueLength);

  // Set an EA to the file
  ntst = NtSetEaFile(hFile, &iosb, ffei, MAX_EA_LENGTH);

  CloseHandle(hFile);

  return NT_SUCCESS(ntst);
}
