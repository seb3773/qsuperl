# qsuperl

A lightweight, specialized fork of [xcape](https://github.com/alols/xcape) for Q4OS TDE, enhancing the Super_L (Windows) key to mimic Windows 10 functionality. Maps Super_L to custom key combinations (e.g., `Ctrl_L+Super_L+Z`) for menu shortcuts, allowing single Super_L presses to open the menu (set Super_L as a dead key in TDE) and Super_L+<key> combinations for other actions (e.g., `Super_L+R` to run programs). Integrates [theasmitkid's window tiler](https://www.q4os.org/forum/viewtopic.php?id=5551) for Super_L-based window tiling. Designed for TDE but compatible with other X11 desktop environments. No external hotkey daemons required.

## Features
- Custom Super_L key mappings (e.g., `Super_L` to `Ctrl_L+Super_L+Z` for menu shortcuts)
- Optional `Ctrl+Shift+Esc` and `Ctrl+Alt+Del` interception for custom binaries
- Integrated window tiler with `Super_L+Enter`, `Super_L+Space`, and `Super_L+Arrow` bindings
  - Additional functions added to the original tiler:
    - `Super_L+<plus key (+) on numeric pad>`: Increase window size by 10%
    - `Super_L+<minus key (-) on numeric pad>`: Decrease window size by 10%  

    *For multiple display configurations (up to 3 physical screens):*
    - `Super_L+1`: Send window to display '1' (works with regular '1' key or numeric pad '1')
    - `Super_L+2`: Send window to display '2' (if any) (works with regular '2' key or numeric pad '2')
    - `Super_L+3`: Send window to display '3' (if any) (works with regular '3' key or numeric pad '3')
- Lightweight, resource-efficient, no dependency on khotkeys or similar daemons
- If any of the needed keys are already grabbed by another program (for example khotkeys or any other hotkeys daemon), an error message will be displayed in a simple X window, with the details of the key(s) grab failed; if all the needed keys are already grabbed, the program will exit (because it won't be able to do anything)

## Arguments
- `-e 'KEY_COMBINATION'`: Maps Super_L to a custom key combination (e.g., `Control_L|Super_L|Z`) for menu shortcuts
- `-x 'BINARY_PATH'`: Intercepts `Ctrl+Shift+Esc` to launch a specified binary (e.g., `/usr/bin/htop` for a task manager)
- `-c 'BINARY_PATH'`: Intercepts `Ctrl+Alt+Del` to launch a specified binary (e.g., `/usr/bin/logout` for a logout page)
- `-w`: Enables the integrated window tiler with Super_L-based key bindings
- `-k`: Launches applications specified with `-x` and `-c` using `kstart` (KDE only; useful to force the window manager to bring the application to the front in certain cases)

## Usage
Example commands:
```bash
qsuperl -e 'Control_L|Super_L|Z'  # Map Super_L to menu shortcut
qsuperl -e 'Control_L|Super_L|Z' -x /usr/bin/htop  # Super_L + Ctrl+Shift+Esc
qsuperl -e 'Control_L|Super_L|Z' -w  # Enable window tiler
qsuperl -e 'Control_L|Super_L|Z' -x /usr/bin/htop -c /usr/bin/logout -k  # Map Super_L, enable window tiler, intercept Ctrl+Shift+Esc and Ctrl+Alt+Del, launch with kstart
```


Building: gcc -DARCH_X86_64 -O2 -Wl,-O1,-z,norelro,--gc-sections,--build-id=none,--as-needed,--icf=all,--hash-style=gnu,--discard-all,--strip-all -fstrict-aliasing -flto -fuse-ld=gold -ffunction-sections -fdata-sections -fno-stack-protector -fno-ident -fno-builtin -fno-plt  -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-exceptions -fomit-frame-pointer -fvisibility=hidden -fno-math-errno -fmerge-all-constants $(pkg-config --cflags xtst x11) qsuperl.c -s -o qsuperl $(pkg-config --libs xtst x11) -pthread -lXinerama

## Resources
- [Q4OS Forum: Menu Shortcuts](https://www.q4os.org/forum/viewtopic.php?id=5550)
- [Q4OS Forum: Window Tiler](https://www.q4os.org/forum/viewtopic.php?id=5551)
