// xattrlib.cpp: определяет экспортированные функции для приложения DLL.
//

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

XATTRLIB_API BOOL ReadEA(
  IN LPCWSTR sFileName,
  IN LPCSTR  sEaName,  // ASCII, не UNICODE!
  OUT PVOID  pBuf,      // Буфер необходимо выделить перед вызовом функции!
  OUT PUINT  puBufLen
)  // Сюда пишется длина записанных в буфер данных.
{
  IO_STATUS_BLOCK iosb;
  NTSTATUS ntst;
  
  // Проверка буферов
  if (NULL == pBuf || NULL == puBufLen)
  {
    return FALSE;
  }

  // Открывается файл на чтение
  HANDLE hFile = OpenFileForRead(sFileName,
    (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_DIRECTORY));

  if (INVALID_HANDLE_VALUE == hFile)
  {
    return FALSE;
  }

  // Выделяется память под все EA
  PFILE_FULL_EA_INFORMATION ffei
    = (PFILE_FULL_EA_INFORMATION) GlobalAlloc(GPTR, MAX_EA_LENGTH);

  while (TRUE)
  {
    // Функция вызывается в режиме 1 вызов - 1 атрибут
    ntst =  NtQueryEaFile(hFile, &iosb, ffei,
                      MAX_EA_LENGTH, TRUE, NULL, NULL, NULL, FALSE);
    if (!NT_SUCCESS(ntst))
    {
      break;
    }
    if (0 == _strnicmp(ffei->EaName, sEaName, MAX_EA_NAME))
    {
      // Копируются данные атрибута во внешний буфер
      memcpy(pBuf, ffei->EaName + ffei->EaNameLength + 1, ffei->EaValueLength);

      // Во внешнюю переменную отправляется длина скопированных данных
      *puBufLen = ffei->EaValueLength;

      GlobalFree(ffei);
      CloseHandle(hFile);
      return TRUE;
    }
  }

  GlobalFree(ffei);
  CloseHandle(hFile);
  return FALSE;
}

XATTRLIB_API BOOL DeleteEA(IN LPCWSTR sFileName, IN LPCSTR sAttrName)
{
  return WriteEA(sFileName, sAttrName, NULL, 0, 0);
}

XATTRLIB_API BOOL WriteEA(
  IN LPCWSTR sFileName,
  IN LPCSTR  sAttrName, // ASCII, не UNICODE!
  IN PVOID   pBuf,
  IN UINT    puBufLen,
  IN BYTE    cFlags
)
{
  IO_STATUS_BLOCK iosb;
  NTSTATUS ntst;

  // Проверка длины имени атрибута
  if (strlen(sAttrName) > MAX_EA_NAME)
  {
    return FALSE;
  }

  // TODO: проверить на максимальную длину как следует
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

  // Выделяется память под все EA
  PFILE_FULL_EA_INFORMATION ffei
    = (PFILE_FULL_EA_INFORMATION) GlobalAlloc(GPTR, MAX_EA_LENGTH);

  ffei->EaNameLength = (UCHAR) strlen(sAttrName);
  ffei->EaValueLength = puBufLen;
  ffei->Flags = cFlags;

  memcpy(ffei->EaName, sAttrName, ffei->EaNameLength);
  memcpy(ffei->EaName + ffei->EaNameLength + 1, pBuf, ffei->EaValueLength);

  ntst = NtSetEaFile(hFile, &iosb, ffei, MAX_EA_LENGTH);

  CloseHandle(hFile);

  return NT_SUCCESS(ntst);
}
