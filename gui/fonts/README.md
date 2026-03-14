# Fonts

Drop any `.ttf` font file here and reference it in `gui/src/main_gui.cpp`:

```cpp
ImGuiIO& io = ImGui::GetIO();
io.Fonts->AddFontFromFileTTF("../fonts/MyFont.ttf", 14.0f);
```

## Recommended free fonts

| Font | Download | Notes |
|---|---|---|
| JetBrains Mono | https://www.jetbrains.com/legalnotices/jetbrains-mono/ | OFL — great for chat UI |
| Inter | https://fonts.google.com/specimen/Inter | OFL — clean UI font |
| Fira Code | https://github.com/tonsky/FiraCode | OFL — ligatures, monospaced |
| Noto Sans | https://fonts.google.com/noto | OFL — wide Unicode coverage |

All fonts listed above are under the SIL Open Font License and can be
distributed with SafeSocket-GUI without restriction.

## Default

If no font is explicitly loaded, Dear ImGui falls back to its embedded
ProggyClean bitmap font (13 px). This is perfectly readable and zero-config.
