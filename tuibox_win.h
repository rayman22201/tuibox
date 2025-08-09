/*
 * tuibox_win.h: Windows compatibility layer for tuibox
 */

#ifndef TUIBOX_WIN_H
#define TUIBOX_WIN_H

#ifdef _WIN32

#include <windows.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1

/* Windows console mode storage */
typedef struct {
    DWORD input_mode;
    DWORD output_mode;
    HANDLE hIn;
    HANDLE hOut;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
} win_console_state;

/* Window size structure compatibility */
typedef struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
} winsize;

/* termios compatibility structure for Windows */
typedef struct termios {
    win_console_state win_state;
} termios;

/* Initialize Windows console for raw mode and ANSI support */
static int win_init_console(struct termios *tio) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    
    if (hOut == INVALID_HANDLE_VALUE || hIn == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    /* Save current console modes */
    tio->win_state.hOut = hOut;
    tio->win_state.hIn = hIn;
    GetConsoleMode(hIn, &tio->win_state.input_mode);
    GetConsoleMode(hOut, &tio->win_state.output_mode);
    
    /* Enable virtual terminal processing for ANSI escape sequences */
    DWORD outMode = tio->win_state.output_mode;
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    #ifdef DISABLE_NEWLINE_AUTO_RETURN
    outMode |= DISABLE_NEWLINE_AUTO_RETURN;
    #endif
    if (!SetConsoleMode(hOut, outMode)) {
        /* Fallback for older Windows versions - use original mode */
        SetConsoleMode(hOut, tio->win_state.output_mode);
    }
    
    /* Set raw input mode and enable mouse input */
    DWORD inMode = tio->win_state.input_mode;
    
    /* Enable mouse and window input */
    inMode |= ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
    
    /* Try to disable Quick Edit Mode if supported */
    #ifdef ENABLE_EXTENDED_FLAGS
    inMode |= ENABLE_EXTENDED_FLAGS;
    #endif
    #ifdef ENABLE_QUICK_EDIT_MODE  
    inMode &= ~ENABLE_QUICK_EDIT_MODE;
    #endif
    
    if (!SetConsoleMode(hIn, inMode)) {
        /* Fallback - just enable mouse input */
        inMode = tio->win_state.input_mode | ENABLE_MOUSE_INPUT;
        SetConsoleMode(hIn, inMode);
    }
    
    /* Set UTF-8 code page for better character support */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    return 0;
}

/* Restore console to original state */
static int win_restore_console(struct termios *tio) {
    SetConsoleMode(tio->win_state.hIn, tio->win_state.input_mode);
    SetConsoleMode(tio->win_state.hOut, tio->win_state.output_mode);
    return 0;
}

/* Get window size */
static int win_get_window_size(struct winsize *ws) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return -1;
    }
    
    ws->ws_col = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    ws->ws_row = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    ws->ws_xpixel = 0;
    ws->ws_ypixel = 0;
    
    return 0;
}

/* Global nonblocking flag for Windows input function */
static int g_nonblocking_mode = 0;

/* Set nonblocking mode for input handling */
static void win_set_nonblocking(int nonblocking) {
    g_nonblocking_mode = nonblocking;
}

/* Windows input reading with mouse support */
static int win_read_input(char *buf, int size) {
    if (!buf || size <= 0) {
        return 0;
    }
    static char pending_buf[256];
    static int pending_len = 0;
    static int pending_pos = 0;
    static DWORD last_call_time = 0;
    
    /* Return pending data first */
    if (pending_len > 0) {
        int to_copy = (pending_len < size) ? pending_len : size;
        memcpy(buf, pending_buf + pending_pos, to_copy);
        pending_pos += to_copy;
        pending_len -= to_copy;
        if (pending_len == 0) {
            pending_pos = 0;
        }
        return to_copy;
    }
    
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE) {
        return 0;
    }
    INPUT_RECORD irec;
    DWORD events_read;
    
    while (1) {
        /* Check if input is available first */
        DWORD pending_events;
        if (!GetNumberOfConsoleInputEvents(hIn, &pending_events)) {
            return 0;
        }
        
        /* If no events available */
        if (pending_events == 0) {
            if (g_nonblocking_mode) {
                /* Return immediately (non-blocking behavior) */
                return 0;
            } else {
                /* Wait a bit and try again (blocking behavior) */
                Sleep(10);
                continue;
            }
        }
        
        /* Input is available, read it */
        if (!ReadConsoleInput(hIn, &irec, 1, &events_read) || events_read == 0) {
            return 0;
        }
        
        switch (irec.EventType) {
            case KEY_EVENT:
                if (irec.Event.KeyEvent.bKeyDown) {
                    char ch = irec.Event.KeyEvent.uChar.AsciiChar;
                    if (ch) {
                        buf[0] = ch;
                        return 1;
                    } else {
                        /* Handle special keys */
                        WORD vk = irec.Event.KeyEvent.wVirtualKeyCode;
                        switch (vk) {
                            case VK_UP:
                                strcpy(buf, "\x1b[A");
                                return 3;
                            case VK_DOWN:
                                strcpy(buf, "\x1b[B");
                                return 3;
                            case VK_RIGHT:
                                strcpy(buf, "\x1b[C");
                                return 3;
                            case VK_LEFT:
                                strcpy(buf, "\x1b[D");
                                return 3;
                            case VK_ESCAPE:
                                buf[0] = '\x1b';
                                return 1;
                            default:
                                break;
                        }
                    }
                }
                break;
                
            case MOUSE_EVENT: {
                MOUSE_EVENT_RECORD *mer = &irec.Event.MouseEvent;
                int x = mer->dwMousePosition.X + 1;  /* Convert to 1-based */
                int y = mer->dwMousePosition.Y + 1;  /* Convert to 1-based */
                
                /* Convert Windows mouse events to ANSI escape sequences */
                /* Format: ESC[<button;x;y[mM] where m=release, M=press */
                
                if (mer->dwEventFlags == 0) {
                    /* Button press/release */
                    if (mer->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
                        /* Left button press */
                        sprintf(pending_buf, "\x1b[<0;%d;%dM", x, y);
                    } else {
                        /* Button release */
                        sprintf(pending_buf, "\x1b[<0;%d;%dm", x, y);
                    }
                    pending_len = (int)strlen(pending_buf);
                } else if (mer->dwEventFlags & MOUSE_MOVED) {
                    /* Mouse move with button held */
                    if (mer->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
                        sprintf(pending_buf, "\x1b[<32;%d;%dM", x, y);
                        pending_len = (int)strlen(pending_buf);
                    } else {
                        /* Regular mouse move - ignore for now, continue loop */
                        continue;
                    }
                } else if (mer->dwEventFlags & MOUSE_WHEELED) {
                    /* Mouse wheel */
                    int delta = (short)HIWORD(mer->dwButtonState);
                    if (delta > 0) {
                        /* Wheel up */
                        sprintf(pending_buf, "\x1b[<64;%d;%dM", x, y);
                    } else {
                        /* Wheel down */
                        sprintf(pending_buf, "\x1b[<65;%d;%dM", x, y);
                    }
                    pending_len = (int)strlen(pending_buf);
                } else {
                    /* Other mouse events - ignore and continue */
                    continue;
                }
                
                if (pending_len > 0) {
                    int to_copy = (pending_len < size) ? pending_len : size;
                    memcpy(buf, pending_buf, to_copy);
                    pending_pos = to_copy;
                    pending_len -= to_copy;
                    if (pending_len == 0) {
                        pending_pos = 0;
                    }
                    return to_copy;
                }
                break;
            }
                
            case WINDOW_BUFFER_SIZE_EVENT:
                /* Window resize - ignore and continue */
                continue;
                
            default:
                /* Unknown event - ignore and continue */
                continue;
        }
    }
    
    return 0;
}

/* Compatibility macros */
#define tcgetattr(fd, tio) win_init_console(tio)
#define tcsetattr(fd, opt, tio) win_restore_console(tio)
#define ioctl(fd, cmd, ws) win_get_window_size(ws)
#define TIOCGWINSZ 0
#define TCSAFLUSH 0
#define read(fd, buf, size) win_read_input(buf, size)
#define win_set_nonblocking_mode(flag) win_set_nonblocking(flag)

#endif /* _WIN32 */
#endif /* TUIBOX_WIN_H */