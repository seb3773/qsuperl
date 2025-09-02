#ifndef TILER_H
#define TILER_H
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>

typedef struct {
    Window win;
    int16_t w, h, x, y;
    int16_t aw, ah, ax, ay;
} Geo;

static Window root;
static int sw, sh, shw, shh;
static Atom atom_net_active_window;
static Atom atom_net_wm_state;
static Atom atom_net_wm_state_fullscreen;
#define MIN_DIM     250
#define MAX_MARGIN  100

static int get_monitor_for_window(Display *dpy, Geo *g, int *mx, int *my, int *mw, int *mh) {
    if (!XineramaIsActive(dpy)) return 0;
    int num_screens = 0;
    XineramaScreenInfo *screens = XineramaQueryScreens(dpy, &num_screens);
    if (!screens) return 0;
    for (int i = 0; i < num_screens; i++) {
        XineramaScreenInfo s = screens[i];
        if (g->x >= s.x_org && g->x < s.x_org + s.width &&
            g->y >= s.y_org && g->y < s.y_org + s.height) {
            *mx = s.x_org;
            *my = s.y_org;
            *mw = s.width;
            *mh = s.height;
            XFree(screens);
            return 1;
        }
    }
    XFree(screens);
    return 0;
}


// static inline Window get_active_window(Display*dpy) {
//     Window w; int revert;
//     XGetInputFocus(dpy,&w,&revert);
//     return w;
// }

static Window get_active_window(Display *dpy) {
    if (atom_net_active_window == None) return 0;
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char *data = NULL;
    if (XGetWindowProperty(dpy, root, atom_net_active_window, 0, 1, False, XA_WINDOW, &type, &format, &nitems, &after, &data) != Success || !data)
        return 0;
   Window win = *(Window*)data;
   XFree(data);
    return win;
}




static int get_geometry(Display *dpy, Window win, Geo *g) {
    Window cur = win, parent, ret_root, *children = NULL;
    unsigned int nchildren;
    XWindowAttributes attr;
    int first = 1;
    if (!win) return 0;
    while (cur) {
        if (!XGetWindowAttributes(dpy, cur, &attr)) break;
        if (first) {
            g->win = win;
            g->w = attr.width; g->h = attr.height;
            first = 0;
        }
        g->ax = attr.x; g->ay = attr.y;
        g->aw = attr.width; g->ah = attr.height;
        if (!XQueryTree(dpy, cur, &ret_root, &parent, &children, &nchildren)) break;
        if (children) XFree(children);
        if (!parent || parent == ret_root) break;
        cur = parent;
    }
    g->x = g->ax; g->y = g->ay;
    return 1;
}


static void compute_dims(Geo *g, const char *action, int *x, int *y, int *w, int *h, int mw, int mh, int mx, int my) {
int ox = g->aw - g->w;
int oy = g->ah - g->h;
int fw = mw - ox;
int fh = mh - oy;
int hw = fw >> 1;
int hh = fh >> 1;
int local_x = g->x - mx;
int local_y = g->y - my;
int center_x = local_x + (g->w >> 1);
int center_y = local_y + (g->h >> 1);
    *x = *y = *w = *h = 0;
    switch (action[0]) {
    case 'c': // center
        *w = (fw * 3) >> 2;
        *h = (fh * 3) >> 2;
        *x = (fw - *w) >> 1;
        *y = (fh - *h) >> 1;
        return;
    case 'f': // fullscreen
        *w = fw; *h = fh;
        *x = 0;  *y = 0;
        return;
    case 'l': // left
        if (g->w == hw && center_x < hw) {
            *x = 0; *y = 0; *w = hw; *h = fh;
        } else {
            *x = 0; *w = hw;
            *y = (center_y < hh) ? 0 : hh;
            *h = hh;
        }
        return;
    case 'r': // right
        if (g->w == hw && center_x >= hw) {
            *x = hw; *y = 0; *w = hw; *h = fh;
        } else {
            *x = hw; *w = hw;
            *y = (center_y < hh) ? 0 : hh;
            *h = hh;
        }
        return;
    case 'u': // up
        if (g->h == hh && center_y < hh) {
            *y = 0; *x = 0; *h = hh; *w = fw;
        } else {
            *y = 0; *h = hh;
            *x = (center_x < hw) ? 0 : hw;
            *w = hw;
        }
        return;
    case 'd': // down
        if (g->h == hh && center_y >= hh) {
            *y = hh; *x = 0; *h = hh; *w = fw;
        } else {
            *y = hh; *h = hh;
            *x = (center_x < hw) ? 0 : hw;
            *w = hw;
        }
        return;
    }
}




static inline void move_resize(Display*dpy, Window w, int x,int y, int ww,int hh) {
    if (w) { XMoveResizeWindow(dpy,w,x,y,ww,hh); XFlush(dpy); }
}




static int is_window_fullscreen(Display *dpy, Window win) {
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char *data = NULL;
if (XGetWindowProperty(dpy, win, atom_net_wm_state, 0, 1024, False, XA_ATOM, &type, &format, &nitems, &after, &data) != Success || !data)
        return 0;
    Atom *atoms = (Atom *)data;
    for (unsigned long i = 0; i < nitems; i++) {
        if (atoms[i] == atom_net_wm_state_fullscreen) {
            XFree(data);
            return 1;
        }
    }
    XFree(data);
    return 0;
}





int tiler_init(Display *dpy) {
atom_net_active_window    = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True);
atom_net_wm_state         = XInternAtom(dpy, "_NET_WM_STATE", False);
atom_net_wm_state_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    int scr = DefaultScreen(dpy);
    root = RootWindow(dpy, scr);
    sw = DisplayWidth(dpy, scr); sh = DisplayHeight(dpy, scr);
    shw = sw / 2; shh = sh / 2;
    return 1;
}





static void resize_by_factor(Display *dpy, Geo *g, int num, int den) {
    int new_w = (g->w * num) / den;
    int new_h = (g->h * num) / den;
    if (num < den && (new_w < MIN_DIM || new_h < MIN_DIM)) return;
    int cx = g->x + g->w / 2;
    int cy = g->y + g->h / 2;
    int mx, my, mw, mh;
    if (!get_monitor_for_window(dpy, g, &mx, &my, &mw, &mh)) {
        mx = 0; my = 0;
         mw = DisplayWidth(dpy, DefaultScreen(dpy));
         mh = DisplayHeight(dpy, DefaultScreen(dpy));
    }
    int max_w = mw - MAX_MARGIN;
    int max_h = mh - MAX_MARGIN;
    if      (new_w < MIN_DIM) new_w = MIN_DIM;
    else if (new_w > max_w)   new_w = max_w;
    if      (new_h < MIN_DIM) new_h = MIN_DIM;
    else if (new_h > max_h)   new_h = max_h;
    int new_x = cx - new_w / 2;
    int new_y = cy - new_h / 2;
    if (new_x < mx)                     new_x = mx;
    if (new_x + new_w > mx + max_w)     new_x = mx + max_w - new_w;
    if (new_y < my)                     new_y = my;
    if (new_y + new_h > my + max_h)     new_y = my + max_h - new_h;
    move_resize(dpy, g->win, new_x, new_y, new_w, new_h);
}







static void move_to_monitor(Display *dpy, Window win, int target_idx) {
    Geo g = {0};
    if (!get_geometry(dpy, win, &g)) return;
    int n_scr = 0;
    XineramaScreenInfo *scr = XineramaQueryScreens(dpy, &n_scr);
    if (!scr || target_idx < 0 || target_idx >= n_scr) {
        if (scr) XFree(scr);
        return;
    }
    int cur = 0;
    for (int i = 0; i < n_scr; i++) {
        if (g.x >= scr[i].x_org &&
            g.x <  scr[i].x_org + scr[i].width &&
            g.y >= scr[i].y_org &&
            g.y <  scr[i].y_org + scr[i].height) {
            cur = i;
            break;
        }
    }
    int local_x = g.x - scr[cur].x_org;
    int local_y = g.y - scr[cur].y_org;
    int new_x = scr[target_idx].x_org + local_x;
    int new_y = scr[target_idx].y_org + local_y;
    int new_w = g.w;
    int new_h = g.h;
    if (new_x + new_w > scr[target_idx].x_org + scr[target_idx].width)
        new_x = scr[target_idx].x_org + scr[target_idx].width - new_w;
    if (new_y + new_h > scr[target_idx].y_org + scr[target_idx].height)
        new_y = scr[target_idx].y_org + scr[target_idx].height - new_h;
    if (new_x < scr[target_idx].x_org) new_x = scr[target_idx].x_org;
    if (new_y < scr[target_idx].y_org) new_y = scr[target_idx].y_org;
    move_resize(dpy, win, new_x, new_y, new_w, new_h);
    XFree(scr);
}




static void toggle_fullscreen(Display *dpy, Window win) {
    XEvent xev = {0};
    xev.xclient.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = atom_net_wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[1] = atom_net_wm_state_fullscreen;
    xev.xclient.data.l[2] = 0;
    int is_fs = is_window_fullscreen(dpy, win);
    xev.xclient.data.l[0] = is_fs ? 0 : 1;
    XSendEvent(dpy, root, False, SubstructureNotifyMask, &xev);
    XFlush(dpy);
}



void handle_action(Display *dpy, const char *action) {
    Window win = get_active_window(dpy);
    if (!win) return;
if (action[0] == '+' && !action[1]) {
    Geo g;
    if (get_geometry(dpy, win, &g)) resize_by_factor(dpy, &g, 110, 100);
    return;
}
if (action[0] == '-' && !action[1]) {
    Geo g;
    if (get_geometry(dpy, win, &g)) resize_by_factor(dpy, &g, 90, 100);
    return;
}
    if (!action[1] && ((unsigned char)action[0]-'0') <= 9u) {
        move_to_monitor(dpy, win, action[0]-'1');
        return;
    }
switch (action[0]) {
case 'c': if (!action[1] && 0) break;
	if (!strcmp(action, "center")) goto do_move;
	break;
case 'f': if (!strcmp(action, "fullscreen")) { toggle_fullscreen(dpy, win); return; }
	break;
case 'l': if (!strcmp(action, "left")) goto do_move;
	break;
case 'r': if (!strcmp(action, "right")) goto do_move;
	break;
case 'u': if (!strcmp(action, "up")) goto do_move;
	break;
case 'd': if (!strcmp(action, "down")) goto do_move;
	break;
    }
    return;
do_move:
    {
        Geo g;
        if (!get_geometry(dpy, win, &g)) return;
        int mx,my,mw,mh;
        if (!get_monitor_for_window(dpy, &g, &mx,&my,&mw,&mh)) return;
        int x,y,w,h;
        compute_dims(&g, action, &x,&y,&w,&h, mw,mh, mx,my);
        move_resize(dpy, win, x+mx, y+my, w, h);
    }
}



#endif
