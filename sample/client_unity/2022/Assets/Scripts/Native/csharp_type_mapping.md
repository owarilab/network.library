# C to C# Type Mapping (libqs)

- `void*`, `QS_*_CONTEXT*`, `QS_EVENT_PARAMETER` -> `System.IntPtr`
- `size_t` -> `System.UIntPtr` (or `nuint` in newer C#)
- `ssize_t` -> `System.IntPtr` then cast to `long` as needed
- `uint8_t*` -> `System.IntPtr` (copy with `Marshal.Copy`)
- `char*`, `const char*` return -> `System.IntPtr` + `Marshal.PtrToStringAnsi`
- `char*`, `const char*` arg -> `string` with `MarshalAs(UnmanagedType.LPStr)` or `IntPtr`
- `int32_t` -> `int`
- `uint32_t` -> `uint`
- `int64_t` -> `long`
- `uint64_t` -> `ulong`
- `uint16_t` -> `ushort`
- `time_t` -> `long` (64-bit platforms)
- `QS_WEBSOCKET_STATE`, `QS_WEBSOCKET_ERROR` -> C# `enum : int`

## Calling Convention

- Windows (`UNITY_EDITOR_WIN`, `UNITY_STANDALONE_WIN`): `CallingConvention.StdCall`
- Other platforms (Linux): `CallingConvention.Cdecl`

## String/Buffer Ownership Notes

- Pointers returned from `libqs_api_get_*` are owned by native side; consume immediately.
- Avoid caching returned pointer values across update ticks.
- For outbound strings created in C#, prefer marshaling `string` directly via LPStr when possible.
