#ifdef XATTRLIB_EXPORTS
#define XATTRLIB_API extern "C" __declspec(dllexport)
#else
#define XATTRLIB_API extern "C" __declspec(dllimport)
#endif

#define MAX_EA_LENGTH 64*1024
#define MAX_EA_NAME 255

XATTRLIB_API BOOL ReadExtendedAttribute
(
  IN  LPCWSTR sFileName,
  IN  LPCSTR  sEaName,  // ASCII, not UNICODE!
  OUT PVOID   pBuf,
  OUT PUINT   puBufLen,
  OUT PBYTE   cFlags
);

XATTRLIB_API BOOL WriteExtendedAttribute
(
  IN LPCWSTR sFileName,
  IN LPCSTR  sAttrName, // ASCII, not UNICODE!
  IN PVOID   pBuf,
  IN UINT    puBufLen,
  IN BYTE    cFlags
);

XATTRLIB_API BOOL DeleteExtendedAttribute(IN LPCWSTR sFileName, IN LPCSTR sAttrName);

XATTRLIB_API BOOL GetExtendedAttributesList(
  IN  LPCWSTR sFileName,
  OUT PFILE_FULL_EA_INFORMATION pAttributeList,
  IN  ULONG puAttributeListLength
);
