# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

### Build and Run

#### Linux/Unix/macOS
```bash
# Build all demos
make

# Run demos to test changes
./demo_basic    # Test basic UI elements and events
./demo_bounce   # Test animation and rendering
./demo_drag     # Test mouse interaction

# Clean build artifacts
make clean
```

#### Windows (MSVC)
```cmd
# Build all demos (from Developer Command Prompt, use native cmd not Cygwin)
nmake /f Makefile.nmake

# Or build individually with cl.exe
cl /O2 /W3 /D_CRT_SECURE_NO_WARNINGS /D_WIN32 /Fe"demos\demo_basic.exe" demos\demo_basic.c

# Run demos (built in demos directory)
demos\demo_basic.exe
demos\demo_bounce.exe
demos\demo_drag.exe

# Clean build artifacts
nmake /f Makefile.nmake clean
```

### Development
```bash
# Debug build (modify Makefile CFLAGS to use -Og -pipe -g instead of -O3)
make

# Compile single demo for testing
cc -O3 -pipe demos/demo_basic.c -lm -o demo_basic
```

## Architecture

### Single-Header Library Design
The entire library is contained in `tuibox.h` (679 lines), with Windows compatibility provided by `tuibox_win.h`. This includes:
- Core UI management structures (`ui_t`, `ui_box_t`, `ui_evt_t`)
- Embedded vec.h for dynamic arrays
- ANSI escape sequence handling
- Mouse and keyboard event processing

### Core Data Flow
```
User Input → ui_loop() → ui_update() → Event Callbacks → ui_draw() → Terminal Output
```

### Key Structures
- **ui_t**: Main UI container managing terminal state, boxes, and events
- **ui_box_t**: Individual UI elements with position, render cache, and callbacks
  - `draw`: Rendering function pointer
  - `onclick`/`onhover`: Event callbacks
  - `cache`: Rendered content cache
  - `watch`: Dirty flag pointer for cache invalidation

### Rendering System
- Uses ANSI escape sequences directly (no ncurses dependency)
- Caching system: Elements only re-render when their `watch` value changes
- Supports 24-bit truecolor
- Coordinate system: 1-indexed, top-left origin

### Event Handling
- Mouse events via terminal escape sequences (`\033[<`)
- Keyboard events through `ui_key()` registration
- Callbacks receive box pointer and can access custom data via `data1`/`data2`

## Important Patterns

### Adding UI Elements
Always use `ui_add()` with proper callbacks:
```c
ui_add(&u, x, y, width, height, screen_id, render_func, click_handler, hover_handler, data);
```

### Cache Management
Elements with a `watch` pointer only re-render when the watched value changes. Set `watch = NULL` for always-redraw behavior.

### Screen Management
Multiple screens supported via `screen` field. Switch screens by setting `u.screen`.

### Memory Management
- `ui_new()` initializes and enters raw mode
- `ui_free()` **must** be called to restore terminal state
- Dynamic arrays (vec_t) handle box/event storage automatically

## Testing Changes
Test modifications using the three demo programs:
1. `demo_basic` - Verify basic UI rendering and events work
2. `demo_bounce` - Check animation and custom render loops
3. `demo_drag` - Ensure mouse tracking and dragging function correctly

## Critical Notes
- Terminal must support ANSI escape sequences and mouse reporting
- Always call `ui_free()` before exit to restore terminal
- Coordinates are 1-indexed (terminal convention)
- `UI_CENTER_X`/`UI_CENTER_Y` constants (-1) trigger auto-centering
- Windows support requires Windows 10+ for full ANSI escape sequence support
- On Windows, use Developer Command Prompt for nmake or ensure cl.exe is in PATH

## Non-blocking Input Support
The library supports both blocking and non-blocking input modes:

### Usage
```c
ui_t u;
ui_new(0, &u);

/* Default: blocking input (read() waits for input) */
/* Use in ui_loop for event-driven programs */

/* Enable non-blocking input (read() returns immediately if no input) */
ui_set_nonblocking(&u, 1);
/* Use in animation loops or custom input polling */

/* Restore blocking mode */
ui_set_nonblocking(&u, 0);
```

### Platform Implementation
- **Unix/Linux/macOS**: Uses `fcntl()` with `O_NONBLOCK` flag on `stdin`
- **Windows**: Uses `GetNumberOfConsoleInputEvents()` to check input availability
- **Cross-platform**: Same API works on both platforms