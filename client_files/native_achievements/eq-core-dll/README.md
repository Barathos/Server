# Achievements Native DLL

This folder is the Achievements feature-owned native client DLL source.

The DLL is a minimal `dinput8.dll` proxy for the 32-bit EverQuest client. It keeps only the DirectInput forwarding, Detours helper, and passive EQ client ABI definitions needed to host the Achievements native window:

- Opens and refreshes `EQUI_NativeAchievementWnd.xml`
- Rewrites `/ach`, `/achievement`, and common misspellings to the `#ach` command family
- Parses `ACH|...` transport lines from the server
- Displays categories, achievements, details, objectives, and reward previews
- Cleans up the Achievement window on UI reload/reset

The project intentionally does not build inherited gameplay-patch, diagnostic, parser, command, overlay, or unrelated feature modules. Some retained headers still contain EQ/MQ-derived structure and offset names because the achievement window needs the client UI ABI, but those definitions are passive.

Do not add other feature windows or transports here. If another feature needs native client behavior, it should own its own feature DLL until a proper native-client-base split exists.

## Build

Open one of:

```text
eq-core-dll-visualstudio2022.sln
eq-core-dll-visualstudio2019.sln
```

Build `Release|Win32`. The output is:

```text
bin/dinput8.dll
```

Deploy only to the matching Achievements client folder:

```text
D:\EQClients\EQClient-Achievements\dinput8.dll
```
