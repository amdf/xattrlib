#ifdef XATTRLIB_EXPORTS
#define XATTRLIB_API extern "C" __declspec(dllexport)
#else
#define XATTRLIB_API extern "C" __declspec(dllimport)
#endif

#define MAX_EA_LENGTH 64*1024
#define MAX_EA_NAME 255

XATTRLIB_API BOOL ReadEA(
  IN LPCWSTR sFileName,
  IN LPCSTR  sEaName,  // ASCII, не UNICODE!
  OUT PVOID  pBuf,      // Буфер необходимо выделить перед вызовом функции!
  OUT PUINT  puBufLen
);

XATTRLIB_API BOOL WriteEA(
  IN LPCWSTR sFileName,
  IN LPCSTR  sAttrName, // ASCII, не UNICODE!
  IN PVOID   pBuf,
  IN UINT    puBufLen,
  IN BYTE    cFlags
);

XATTRLIB_API BOOL DeleteEA(IN LPCWSTR sFileName, IN LPCSTR sAttrName);
