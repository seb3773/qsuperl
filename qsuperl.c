//
 //                                  / \ / \ / \ / \ / \ / \ / \ 
//                                   (q | s | u | p | e | r | l )
 //                                  \_/ \_/ \_/ \_/ \_/ \_/ \_/  by seb3773
//
//    **highly modified** xcape fork for my usage with q4os TDE.
//     =========================================
// 
// The main purpose of this program is to provide in TDE the same functionnalities for the Super_L (windows) key as found in windows 10, optionnally providing
// [ctrl+shit+esc] and [ctrl+alt+del] interception to launch custom binaries of your choice (a task manager and a logout page for example^^), without needing khotkeys
// daemon (or any other 'classic' hotkeys daemon) : it's higly specialized (targetting only some keys combinations), so it's a small binary very light on ressources.
// In addition, as it is convenient to control it with Super_L key combinations, "tiler" from theasmitkid is integrated too, avoiding the need of another running daemon if you
// want this functionnality (all credits go to him for this part).
//    **Note: it's designed to work with TDE, this is the desktop environment I'm using, but nothing prevents it to work with an other x11 DE, if you try, please report bugs if any :)
// 
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//
// >> new keys combination in argument are only associated to Super_L :
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//            Just use argument "-e" like that for example: 
//            qsuperl -e 'Control_L|Super_L|Z'                ( --- equivalent to-->      xcape -e 'Super_L=Control_L|Super_L|Z'       as only Super_L is monitored)
//            This will map the windows key to another key combination for the menu shortcut ([ctrl left]+[superL]+[Z] keys in this example), so don't forget to set Super_L as a 'dead key'.
//            The result, is that you can have a single press on Super_L to open the menu as it is replaced by the sequence you had set (configure TDE keyboard menu shortcut sequence accordingly)
//            and have combinations like Super_L+<other keys> working too (configure them to your liking in TDE Keyboard Shortcuts sequences, for ex.: [Super_L]+[r] to run program etc... )
//              * This is best explained here on Q4OS Forum:      https://www.q4os.org/forum/viewtopic.php?id=5550
//
//
// >> window tiler from theasmitkid is integrated and mapped to win+enter, win+space and win+<cursur arrows>, activated when argument "-w" if specified :
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//            I really like his tiler idea and ingenious implementation, and as we are already monitoring keys, why not intercept Super_L+<usual keys> for tiling too ?
//            so:    - no need for a separate daemon, nor fifo file & - no need for the khotkeys daemon (except if you use it for something else, they can work together)
//            The program logic is implemented in tiler.h, to have easier maintenance if something need to be changed in this part.
//              * Reference here: https://www.q4os.org/forum/viewtopic.php?id=5551
//                     *functions added to the original tiler: Super_L+1 : send window to display '1' (works with regular '1' key or numeric pad '1')
//                                                                                     Super_L+2 : send window to display '2' (if any)    (works with regular '2' key or numeric pad '2')
//                                                                                     Super_L+3 : send window to display '3' (if any)     (works with regular '3' key or numeric pad '3')
//                                                                                     Super_L+<plus key (+) on numeric pad> : increase windows size by 10%
//                                                                                     Super_L+<minus key (-) on numeric pad> : decrease windows size by 10%
// 
// 
// >> These 'classic windows' keys sequences can optionnaly be intercepted too:
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//           >> ctrl+shift+esc to launch a taskmanager (or everything you want) : -x 'full binary path'
//           >> ctrl+alt+suppr to launch a program of your choice: -c 'full binary path'
//                               ** the option -k can be specified if you want the application to be launched with kstart (kde only; can be usefull to force the WM to bring the application to front in certain cases.)
//
// ***** If any of the needed keys are already grabbed by an other program (for example khotkeys or any other hotkeys daemon), an error message will be displayed
//          in a simple X windows, with the details of the key(s) grab failed; if all the needed keys are already grabbed, the program will exit (because it won't be able to do anything...)
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
//     Examples:
//     ------------
//          qsuperl -e 'Control_L|Super_L|Z'                                                                              # only Super_L 
//          qsuperl -e 'Control_L|Super_L|Z' -w                                                                        # Super_L + window tiler
//          qsuperl -e 'Control_L|Super_L|Z' -x /usr/bin/htop                                                # Super_L + Ctrl+Shift+Esc
//          qsuperl -e 'Control_L|Super_L|Z' -w -x /usr/bin/htop -c /usr/bin/logout -k       # everything (Super_L, Ctrl+Shift+Esc, Ctrl+Alt+Del, window tiler, use kstart to launch the applications associated with -x & -c)
//
//          the command line I'm using on my system:     qsuperl -e 'Control_L|Super_L|Z' -w -x '/usr/bin/qxtask' -c '/usr/local/bin/win10-cad'
//
//     Compilation (and binary size reduction with strip & sstrip):
//     ------------------------------------------------------------------
//          gcc -DARCH_X86_64 -O2 -Wl,-O1,-z,norelro,--gc-sections,--build-id=none,--as-needed,--icf=all,--hash-style=gnu,--discard-all,--strip-all -fstrict-aliasing -flto -fuse-ld=gold -ffunction-sections -fdata-sections -fno-stack-protector -fno-ident -fno-builtin -fno-plt  -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-exceptions -fomit-frame-pointer -fvisibility=hidden -fno-math-errno -fmerge-all-constants $(pkg-config --cflags xtst x11) qsuperl.c -s -o qsuperl $(pkg-config --libs xtst x11) -pthread -lXinerama  && strip --strip-all --remove-section=.comment --remove-section=.note ./qsuperl  && sstrip ./qsuperl
// 
//==================================================================================================================================
//==================================================================================================================================

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>
#include <stdint.h>
#include <limits.h>
#include "tiler.h"

#define MAX_FAILED_GRABS 128
#define MAX_TO_KEYS 8
#define MAX_KEYCODE 255
#define KEYMAP_PRESSED 0x01
#define KEYMAP_USED    0x02
#define WINDOW_TILER_ENABLED 1
#define KSTART_ENABLED (1 << 4)
#define CTRL_SHIFT_ESC_ARMED  0x01
#define CTRL_ALT_DEL_ARMED    0x02
#define WIN_SPACE_ARMED       0x04
#define WIN_ENTER_ARMED       0x08
#define WIN_LEFT_ARMED        0x10
#define WIN_RIGHT_ARMED       0x20
#define WIN_UP_ARMED          0x40
#define WIN_DOWN_ARMED        0x80
#define WIN_1_ARMED          0x100
#define WIN_2_ARMED          0x200
#define WIN_3_ARMED          0x400
#define WIN_PLUS_ARMED       0x0800
#define WIN_MINUS_ARMED      0x1000

typedef struct {
KeyCode to_keys[MAX_TO_KEYS];
uint8_t generated_counts[MAX_KEYCODE + 1];
struct timespec down_at;
uint8_t num_to_keys;
uint8_t flags;
} KeyMap_t;

typedef struct {
Display *data_conn;
Display *ctrl_conn;
XRecordContext record_ctx;
pthread_t sigwait_thread;
sigset_t sigset;
KeyMap_t map;
struct timespec timeout; 
KeyCode super_kc, enter_kc, space_kc, ctrl_kc, shift_kc, alt_kc, esc_kc, del_kc;
char *taskmgr_path;
char *cad_path;
uint8_t flags;
uint16_t armed_flags;
} XCape_t;

static char *taskmgr_path = NULL;
static char *cad_path = NULL;
static char   *failed_grabs[MAX_FAILED_GRABS];
static int     failed_count     = 0;
static int     attempted_count  = 0;
static int     grab_failed      = 0;
static char    grab_desc[128];
static int attempted_invocations = 0;
static int failed_invocations   = 0;
static XErrorHandler prev_error_handler = NULL;


static int handle_bad_access(Display *dpy, XErrorEvent *ev);
static void show_grab_error(Display *dpy);
static void try_grab_key(Display *dpy, Window root, KeyCode kc, unsigned int mod);


#ifdef ARCH_X86_64
static inline void sys_write(int fd, const char *buf, size_t len) {
asm volatile("syscall" :: "a"(1), "D"(fd), "S"(buf), "d"(len) : "rcx", "r11", "memory");
}
#elif defined(ARCH_ARM)
static inline void sys_write(int fd, const char *buf, size_t len) {
register long r0 asm("r0") = fd;
register const char *r1 asm("r1") = buf;
register long r2 asm("r2") = len;
register long r7 asm("r7") = 4;
asm volatile("swi 0" :: "r"(r0), "r"(r1), "r"(r2), "r"(r7) : "memory");
}
#else
#error "arch must be defined:  -DARCH_X86_64 ou -DARCH_ARM."
#endif





static int handle_bad_access(Display *dpy, XErrorEvent *ev) {
    if (ev->error_code == BadAccess) {
        grab_failed = 1;
        return 0;
    }
    if (prev_error_handler)
        return prev_error_handler(dpy, ev);
    return 0;
}




static void show_grab_error(Display *dpy) {
    int scr = DefaultScreen(dpy);
    Window root = RootWindow(dpy, scr);
    unsigned long white = WhitePixel(dpy, scr);
    unsigned long black = BlackPixel(dpy, scr);
    int W = 400, H = 120;
    Window w = XCreateSimpleWindow(dpy, root, 100, 100, W, H, 1, black, white);
    XSelectInput(dpy, w, ExposureMask | ButtonPressMask);
    XMapWindow(dpy, w);
    GC gc = XCreateGC(dpy, w, 0, NULL);
    XSetForeground(dpy, gc, black);
    XFontStruct *font = XLoadQueryFont(dpy, "fixed");
    if (font) XSetFont(dpy, gc, font->fid);
    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);
        if (ev.type == Expose) {
            const char *msg1 = "Can't grab key :";
            XDrawString(dpy, w, gc, 10, 20, msg1, strlen(msg1));
            XDrawString(dpy, w, gc, 10, 40, grab_desc, strlen(grab_desc));
            const char *ok = " OK ";
            int bw = 60, bh = 25;
            int bx = (W - bw) / 2, by = H - bh - 10;
            XDrawRectangle(dpy, w, gc, bx, by, bw, bh);
            int tw = XTextWidth(font, ok, strlen(ok));
            XDrawString(dpy, w, gc, bx + (bw - tw)/2, by + bh/2 + 5, ok, strlen(ok));
        }
        else if (ev.type == ButtonPress) {
            XUnloadFont(dpy, font->fid);
            XFreeGC(dpy, gc);
            XDestroyWindow(dpy, w);
            XFlush(dpy);
            break;
        }
    }
}




static void try_grab_key(Display *dpy, Window  root, KeyCode kc, unsigned int mod) {
    attempted_invocations++;
    char tmp[64] = "";
    if (mod & ControlMask) strcat(tmp, "Ctrl+");
    if (mod & ShiftMask)   strcat(tmp, "Shift+");
    if (mod & Mod1Mask)    strcat(tmp, "Alt+");
    if (mod & Mod4Mask)    strcat(tmp, "Super+");
    KeySym ks = XkbKeycodeToKeysym(dpy, kc, 0, 0);
    const char *name = XKeysymToString(ks);
    if (!name) name = "(unknown)";
    strcat(tmp, name);
    strncpy(grab_desc, tmp, sizeof(grab_desc)-1);
    grab_desc[sizeof(grab_desc)-1] = '\0';
    prev_error_handler = XSetErrorHandler(handle_bad_access);
    grab_failed = 0;
    XGrabKey(dpy, kc, mod, root, True, GrabModeAsync, GrabModeAsync);
    XSync(dpy, False);
    XSetErrorHandler(prev_error_handler);
    if (grab_failed) {
        failed_invocations++;
        int already = 0;
        for (int i = 0; i < failed_count; i++) {
            if (strcmp(failed_grabs[i], grab_desc) == 0) {
                already = 1;
                break;
            }
        }
        if (!already && failed_count < MAX_FAILED_GRABS) {
            failed_grabs[failed_count++] = strdup(grab_desc);
        }
        grab_failed = 0;
    }
}




static void show_failed_grabs(Display *dpy) {
    int scr = DefaultScreen(dpy);
    Window root = RootWindow(dpy, scr);
    unsigned long black = BlackPixel(dpy, scr);
    unsigned long white = WhitePixel(dpy, scr);
    Colormap cmap = DefaultColormap(dpy, scr);
    XColor red_col, exact;
    if (!XAllocNamedColor(dpy, cmap, "red", &red_col, &exact)) { red_col.pixel = black; }
    int line_h = 20;
    int extra_lines = 3;
    int W = 500;
    int bh = 30;
    int margin_bot = 10; 
    int H = 60 + (failed_count + extra_lines) * line_h + bh + margin_bot;
    Window w = XCreateSimpleWindow(dpy, root, 200, 200, W, H, 2, black, white);
    XStoreName(dpy, w, "qsuperl error");
    XSelectInput(dpy, w, ExposureMask | ButtonPressMask);
    XMapWindow(dpy, w);
    GC gc = XCreateGC(dpy, w, 0, NULL);
    XSetForeground(dpy, gc, black);
    XFontStruct *font = XLoadQueryFont(dpy, "fixed");
    if (font) XSetFont(dpy, gc, font->fid);
    const char *ok_label = "OK";
    int bw = 60, bx = (W - bw) / 2;
    int btn_x = bx, btn_y = 0;
    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);
        if (ev.type == Expose) {
            int y = 20;
            const char *title = "Failed to grab the following key combinations:";
            XSetForeground(dpy, gc, black);
            XDrawString(dpy, w, gc, 10, y, title, strlen(title));
            y += line_h;
            XSetForeground(dpy, gc, red_col.pixel);
            for (int i = 0; i < failed_count; i++, y += line_h) {
                XDrawString(dpy, w, gc, 20, y, failed_grabs[i], strlen(failed_grabs[i]));
            }
            y += 2 * line_h;
            XSetForeground(dpy, gc, black);
            if (failed_invocations == attempted_invocations) {
                const char *msg = "   >> All grabs failed. Exiting after OK.";
                XDrawString(dpy, w, gc, 10, y, msg, strlen(msg));
            }
            y += line_h;
            btn_y = y;
            XDrawRectangle(dpy, w, gc, btn_x, btn_y, bw, bh);
            int tw = XTextWidth(font, ok_label, strlen(ok_label));
            int tx = btn_x + (bw - tw) / 2;
            int ty = btn_y + (bh + font->ascent) / 2;
            XDrawString(dpy, w, gc, tx, ty, ok_label, strlen(ok_label));
        }
        else if (ev.type == ButtonPress) {
            int x = ev.xbutton.x, yb = ev.xbutton.y;
            if (x >= btn_x && x <= btn_x + bw && yb>= btn_y && yb<= btn_y + bh) {
                break;
            }
        }
    }
    if (font)   XUnloadFont(dpy, font->fid);
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, w);
    XFlush(dpy);
    for (int i = 0; i < failed_count; i++) { free(failed_grabs[i]); }
    failed_count = 0;
    attempted_invocations= 0;
    failed_invocations  = 0;
}







void *sig_handler(void *user_data) {
XCape_t *self = (XCape_t*)user_data;
int sig;
sigwait(&self->sigset, &sig);
XLockDisplay(self->ctrl_conn);
XRecordDisableContext(self->ctrl_conn, self->record_ctx);
XSync(self->ctrl_conn, False);
XUnlockDisplay(self->ctrl_conn);
  return NULL;
}




void launch_program(XCape_t *self, const char *path) {
    if (!fork()) {
        if (self->flags & KSTART_ENABLED) {
            char *argv[] = { "kstart", "--activate", (char *)path, NULL };
            execvp("kstart", argv);
        } else { execl(path, path, (char *)NULL); }
        _exit(127);
    }
}




static void intercept(XPointer user_data, XRecordInterceptData *data) {
    XCape_t *self = (XCape_t*)user_data;
    static int mouse_buttons_down = 0;
   static KeyCode left_kc = 0, right_kc = 0, up_kc = 0, down_kc= 0, one_kc = 0, two_kc = 0, three_kc = 0, kp_one_kc = 0, 
                             kp_two_kc = 0, kp_three_kc = 0, kp_plus_kc = 0, kp_minus_kc = 0;
    static uint8_t ctrl_down = 0, shift_down = 0, alt_down = 0;
    static KeyCode last_key_pressed = 0;
    if (!left_kc) {
        left_kc  = XKeysymToKeycode(self->ctrl_conn, XK_Left);
        right_kc = XKeysymToKeycode(self->ctrl_conn, XK_Right);
        up_kc    = XKeysymToKeycode(self->ctrl_conn, XK_Up);
        down_kc  = XKeysymToKeycode(self->ctrl_conn, XK_Down);
       one_kc    = XKeysymToKeycode(self->ctrl_conn, XK_1);
       two_kc    = XKeysymToKeycode(self->ctrl_conn, XK_2);
       three_kc  = XKeysymToKeycode(self->ctrl_conn, XK_3);
        kp_one_kc    = XKeysymToKeycode(self->ctrl_conn, XK_KP_1);
        kp_two_kc    = XKeysymToKeycode(self->ctrl_conn, XK_KP_2);
        kp_three_kc  = XKeysymToKeycode(self->ctrl_conn, XK_KP_3);
        kp_plus_kc  = XKeysymToKeycode(self->ctrl_conn, XK_KP_Add);
        kp_minus_kc = XKeysymToKeycode(self->ctrl_conn, XK_KP_Subtract);
    }
    XLockDisplay(self->ctrl_conn);
    if (data->category != XRecordFromServer) goto exit;
    register int key_event = data->data[0];
    register KeyCode key_code = data->data[1];
    if (key_code <= MAX_KEYCODE && self->map.generated_counts[key_code] > 0) {
        self->map.generated_counts[key_code]--;
        goto exit;
    }
    if (key_event == ButtonPress) {
        mouse_buttons_down++;
    } else if (key_event == ButtonRelease) {
        if (--mouse_buttons_down < 0) mouse_buttons_down = 0;
    }
    if (key_code == self->ctrl_kc)  ctrl_down  = (key_event == KeyPress);
    if (key_code == self->shift_kc) shift_down = (key_event == KeyPress);
    if (key_code == self->alt_kc)   alt_down   = (key_event == KeyPress);
    if (key_event == KeyPress) {
        last_key_pressed = key_code;
        if ((self->map.flags & KEYMAP_PRESSED) && key_code != self->super_kc) {
            self->map.flags |= KEYMAP_USED;
        }
        if (self->taskmgr_path && ctrl_down && shift_down && 
            key_code == self->esc_kc && (self->armed_flags & CTRL_SHIFT_ESC_ARMED)) {
          launch_program(self, self->taskmgr_path);
             self->armed_flags &= ~CTRL_SHIFT_ESC_ARMED;
            goto cleanup_return;
        }
        if (self->cad_path && ctrl_down && alt_down && 
            key_code == self->del_kc && (self->armed_flags & CTRL_ALT_DEL_ARMED)) {
            launch_program(self, self->cad_path);
            self->armed_flags &= ~CTRL_ALT_DEL_ARMED;
            goto cleanup_return;
        }
        if ((self->flags & WINDOW_TILER_ENABLED) && (self->map.flags & KEYMAP_PRESSED)) {
            const char *action = NULL;
            uint16_t mask = 0;
            if (key_code == self->enter_kc && (self->armed_flags & WIN_ENTER_ARMED)) {
                action = "fullscreen"; mask = WIN_ENTER_ARMED;
            } else if (key_code == self->space_kc && (self->armed_flags & WIN_SPACE_ARMED)) {
                action = "center"; mask = WIN_SPACE_ARMED;
            } else if (key_code == left_kc && (self->armed_flags & WIN_LEFT_ARMED)) {
                action = "left"; mask = WIN_LEFT_ARMED;
            } else if (key_code == right_kc && (self->armed_flags & WIN_RIGHT_ARMED)) {
                action = "right"; mask = WIN_RIGHT_ARMED;
            } else if (key_code == up_kc && (self->armed_flags & WIN_UP_ARMED)) {
                action = "up"; mask = WIN_UP_ARMED;
            } else if (key_code == down_kc && (self->armed_flags & WIN_DOWN_ARMED)) {
                action = "down"; mask = WIN_DOWN_ARMED;
            }
      // Super+1, Super+2, Super+3
          else if ((key_code == one_kc    || key_code == kp_one_kc) && (self->armed_flags & WIN_1_ARMED)) {
            action = "1";               mask = WIN_1_ARMED; }
        else if ((key_code == two_kc    || key_code == kp_two_kc) && (self->armed_flags & WIN_2_ARMED)) {
            action = "2";               mask = WIN_2_ARMED; }
        else if ((key_code == three_kc  || key_code == kp_three_kc) && (self->armed_flags & WIN_3_ARMED)) {
            action = "3";               mask = WIN_3_ARMED; }
        else if (key_code == kp_plus_kc && (self->armed_flags & WIN_PLUS_ARMED)) {
            action = "+";             mask = WIN_PLUS_ARMED; }
        else if (key_code == kp_minus_kc && (self->armed_flags & WIN_MINUS_ARMED)) {
            action = "-";             mask = WIN_MINUS_ARMED; }
            if (action) {
                  handle_action(self->ctrl_conn, action);
                  self->armed_flags &= ~mask;
                  self->map.flags |= KEYMAP_USED;
                 goto exit;
                }
         }
     }
  if (key_event == KeyRelease) {
        if (self->taskmgr_path && !ctrl_down && !shift_down) { self->armed_flags |= CTRL_SHIFT_ESC_ARMED; }
        if (self->cad_path && !ctrl_down && !alt_down) { self->armed_flags |= CTRL_ALT_DEL_ARMED; }
        if (self->flags & WINDOW_TILER_ENABLED) {
          if (key_code == self->enter_kc) self->armed_flags |= WIN_ENTER_ARMED;
          else if (key_code == self->space_kc) self->armed_flags |= WIN_SPACE_ARMED;
          else if (key_code == left_kc) self->armed_flags |= WIN_LEFT_ARMED;
          else if (key_code == right_kc) self->armed_flags |= WIN_RIGHT_ARMED;
          else if (key_code == up_kc) self->armed_flags |= WIN_UP_ARMED;
          else if (key_code == down_kc) self->armed_flags |= WIN_DOWN_ARMED;
       //rearm Super+1/2/3
        else if (key_code == one_kc    || key_code == kp_one_kc)    self->armed_flags |= WIN_1_ARMED;
        else if (key_code == two_kc    || key_code == kp_two_kc)    self->armed_flags |= WIN_2_ARMED;
        else if (key_code == three_kc  || key_code == kp_three_kc)  self->armed_flags |= WIN_3_ARMED;
        else if (key_code == kp_plus_kc)   self->armed_flags |= WIN_PLUS_ARMED;
        else if (key_code == kp_minus_kc)  self->armed_flags |= WIN_MINUS_ARMED;
            else if (key_code == self->super_kc) {
   self->armed_flags |= (WIN_SPACE_ARMED | WIN_ENTER_ARMED | WIN_LEFT_ARMED | WIN_RIGHT_ARMED | WIN_UP_ARMED |
                                        WIN_DOWN_ARMED | WIN_1_ARMED | WIN_2_ARMED | WIN_3_ARMED | WIN_PLUS_ARMED | WIN_MINUS_ARMED);
         }
        }
   }
    // Super_L
    if (key_code == self->super_kc) {
        if (key_event == KeyPress) {
            self->map.flags |= KEYMAP_PRESSED;
            clock_gettime(CLOCK_MONOTONIC, &self->map.down_at);
            if (mouse_buttons_down > 0) self->map.flags |= KEYMAP_USED;
        } else {
            if (!(self->map.flags & KEYMAP_USED)) {
                struct timespec now, delta;
                clock_gettime(CLOCK_MONOTONIC, &now);
                delta.tv_sec = now.tv_sec - self->map.down_at.tv_sec;
                delta.tv_nsec = now.tv_nsec - self->map.down_at.tv_nsec;
                if (delta.tv_nsec < 0) {
                    delta.tv_sec--;
                    delta.tv_nsec += 1000000000L;
                }
                if (delta.tv_sec < self->timeout.tv_sec ||
                    (delta.tv_sec == self->timeout.tv_sec && delta.tv_nsec < self->timeout.tv_nsec)) {
                    for (int i = 0; i < self->map.num_to_keys; i++) {
                       KeyCode k = self->map.to_keys[i];
                        XTestFakeKeyEvent(self->ctrl_conn, k, True, 0);
                        if (k <= MAX_KEYCODE) self->map.generated_counts[k]++;
                    }
                    for (int i = 0; i < self->map.num_to_keys; i++) {
                    KeyCode k = self->map.to_keys[i];
                    XTestFakeKeyEvent(self->ctrl_conn, k, False, 0);
                     if (k <= MAX_KEYCODE) self->map.generated_counts[k]++;
                    }
                        XFlush(self->ctrl_conn);
                }
            }
            self->map.flags &= ~(KEYMAP_PRESSED | KEYMAP_USED);
        }
    }
exit:
    XUnlockDisplay(self->ctrl_conn);
    XRecordFreeData(data);
    return;
cleanup_return:
    XRecordFreeData(data);
    XUnlockDisplay(self->ctrl_conn);
}






int main(int argc, char **argv) {
XCape_t *self = calloc(1, sizeof(XCape_t));
self->flags = 0;
self->armed_flags = (taskmgr_path ? CTRL_SHIFT_ESC_ARMED : 0)
                   | (cad_path ? CTRL_ALT_DEL_ARMED : 0)
                   | (WIN_SPACE_ARMED  | WIN_ENTER_ARMED | WIN_LEFT_ARMED  | WIN_RIGHT_ARMED
                   | WIN_UP_ARMED  | WIN_DOWN_ARMED | WIN_1_ARMED  | WIN_2_ARMED | WIN_3_ARMED)
                   | WIN_PLUS_ARMED | WIN_MINUS_ARMED;
int dummy, ch;
char *mapping = NULL;
XRecordRange *rec_range = XRecordAllocRange();
XRecordClientSpec client_spec = XRecordAllClients;
self->timeout.tv_sec = 0;
self->timeout.tv_nsec = 300000000;
rec_range->device_events.first = KeyPress;
rec_range->device_events.last = ButtonRelease;
   while ((ch = getopt(argc, argv, "e:x:c:wk")) != -1) {
       switch (ch) {
         case 'e': mapping = optarg; break;
         case 'x': taskmgr_path = optarg; break;
         case 'c': cad_path = optarg; break;
         case 'w': self->flags |= WINDOW_TILER_ENABLED; break;
          case 'k': self->flags |= KSTART_ENABLED; break;
         default: return EXIT_SUCCESS;
      }
    }
self->taskmgr_path = taskmgr_path;
self->cad_path = cad_path;
self->data_conn = XOpenDisplay(NULL);
self->ctrl_conn = XOpenDisplay(NULL);
    if (!self->data_conn || !self->ctrl_conn) {
        static const char msg[] = "X11 connect error\n";
        sys_write(2, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
if (!XQueryExtension(self->ctrl_conn, "XTEST", &dummy, &dummy, &dummy) ||
!XRecordQueryVersion(self->ctrl_conn, &dummy, &dummy) ||
!XkbQueryExtension(self->ctrl_conn, &dummy, &dummy, &dummy, &dummy, &dummy)) {
exit(EXIT_FAILURE);
}
    self->super_kc = XKeysymToKeycode(self->ctrl_conn, XStringToKeysym("Super_L"));
    self->enter_kc = XKeysymToKeycode(self->ctrl_conn, XK_Return);
    self->space_kc = XKeysymToKeycode(self->ctrl_conn, XK_space);
    self->ctrl_kc = XKeysymToKeycode(self->ctrl_conn, XK_Control_L);
    self->shift_kc = XKeysymToKeycode(self->ctrl_conn, XK_Shift_L);
    self->alt_kc = XKeysymToKeycode(self->ctrl_conn, XK_Alt_L);
    self->esc_kc = XKeysymToKeycode(self->ctrl_conn, XK_Escape);
    self->del_kc = XKeysymToKeycode(self->ctrl_conn, XK_Delete);
    KeyCode kc_left = XKeysymToKeycode(self->ctrl_conn, XK_Left);
    KeyCode kc_right = XKeysymToKeycode(self->ctrl_conn, XK_Right);
    KeyCode kc_up = XKeysymToKeycode(self->ctrl_conn, XK_Up);
    KeyCode kc_down = XKeysymToKeycode(self->ctrl_conn, XK_Down);
   KeyCode kc_one   = XKeysymToKeycode(self->ctrl_conn, XK_1);
   KeyCode kc_two   = XKeysymToKeycode(self->ctrl_conn, XK_2);
   KeyCode kc_three = XKeysymToKeycode(self->ctrl_conn, XK_3);
    KeyCode kc_kp_one   = XKeysymToKeycode(self->ctrl_conn, XK_KP_1);
    KeyCode kc_kp_two   = XKeysymToKeycode(self->ctrl_conn, XK_KP_2);
    KeyCode kc_kp_three = XKeysymToKeycode(self->ctrl_conn, XK_KP_3);
    KeyCode kc_kp_plus   = XKeysymToKeycode(self->ctrl_conn, XK_KP_Add);
    KeyCode kc_kp_minus  = XKeysymToKeycode(self->ctrl_conn, XK_KP_Subtract);
    self->map.num_to_keys = 0;
    if (mapping) {
        int count = 1;
        for (const char *p = mapping; *p; p++) if (*p == '|') count++;
        if (count > MAX_TO_KEYS) {
            static const char msg[] = "Too many keys\n";
            sys_write(2, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        char *mapping_copy = strdup(mapping);
        if (mapping_copy) {
            char *key, *mapping_ptr = mapping_copy;
            int i = 0;
            while ((key = strsep(&mapping_ptr, "|")) && i < MAX_TO_KEYS) {
                KeySym ks = XStringToKeysym(key);
                if (ks == NoSymbol) continue;
                KeyCode kc = XKeysymToKeycode(self->ctrl_conn, ks);
                if (kc == 0 || kc > MAX_KEYCODE) continue;
                self->map.to_keys[i++] = kc;
            }
            self->map.num_to_keys = i;
            free(mapping_copy);
        }
    }
    Window root = DefaultRootWindow(self->ctrl_conn);
    static const int masks[] = {0, LockMask, Mod2Mask, LockMask | Mod2Mask};
    #define GRAB_KEY(kc,mod) \
      for (int i = 0; i < 4; i++) \
        try_grab_key(self->ctrl_conn, root, kc, (mod)|masks[i])
  if (self->flags & WINDOW_TILER_ENABLED) {
    tiler_init(self->ctrl_conn);
       GRAB_KEY(self->enter_kc, Mod4Mask);
       GRAB_KEY(kc_left, Mod4Mask);
       GRAB_KEY(kc_right, Mod4Mask);
       GRAB_KEY(kc_up, Mod4Mask);
       GRAB_KEY(kc_down, Mod4Mask);
       GRAB_KEY(self->space_kc, Mod4Mask);
       GRAB_KEY(kc_one,    Mod4Mask);
       GRAB_KEY(kc_kp_one, Mod4Mask);
       GRAB_KEY(kc_two,    Mod4Mask);
       GRAB_KEY(kc_kp_two, Mod4Mask);
       GRAB_KEY(kc_three,  Mod4Mask);
       GRAB_KEY(kc_kp_three,Mod4Mask);
       GRAB_KEY(kc_kp_plus,   Mod4Mask);
       GRAB_KEY(kc_kp_minus,  Mod4Mask);
    }
if (self->taskmgr_path) GRAB_KEY(self->esc_kc, ControlMask | ShiftMask);
if (self->cad_path) GRAB_KEY(self->del_kc, ControlMask | Mod1Mask);
    if (failed_count > 0) {
        show_failed_grabs(self->ctrl_conn);
        if (attempted_invocations > 0 && failed_invocations   == attempted_invocations) {
          exit(EXIT_FAILURE);
        }
    }
daemon(0, 0);
sigemptyset(&self->sigset);
sigaddset(&self->sigset, SIGINT);
sigaddset(&self->sigset, SIGTERM);
pthread_sigmask(SIG_BLOCK, &self->sigset, NULL);
pthread_create(&self->sigwait_thread, NULL, sig_handler, self);
self->record_ctx = XRecordCreateContext(self->ctrl_conn, 0, &client_spec, 1, &rec_range, 1);
XSync(self->ctrl_conn, False);
XRecordEnableContext(self->data_conn, self->record_ctx, intercept, (XPointer)self);
pthread_join(self->sigwait_thread, NULL);
XRecordFreeContext(self->ctrl_conn, self->record_ctx);
XFree(rec_range);
XCloseDisplay(self->ctrl_conn);
XCloseDisplay(self->data_conn);
free(self);
return EXIT_SUCCESS;
}

