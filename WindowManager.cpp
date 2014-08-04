extern "C" {
#include <X11/Xutil.h>
}
#include <iostream>
#include <cassert>
#include "WindowManager.hh"

bool newin::WindowManager::_wm_detected;
std::mutex newin::WindowManager::_wm_detected_mutex;

std::unique_ptr<newin::WindowManager> newin::WindowManager::Create() {
    Display* display = XOpenDisplay(nullptr);
    assert(display != nullptr);
    return std::unique_ptr<WindowManager>(new WindowManager(display));
}

newin::WindowManager::WindowManager(Display* display) : _display(display), _root(DefaultRootWindow(_display)) {
    assert(display != nullptr);
    resetEvents();
}

newin::WindowManager::~WindowManager() {
    XCloseDisplay(_display);
}

void newin::WindowManager::run() noexcept {
    {std::lock_guard<std::mutex> lock(_wm_detected_mutex);

	_wm_detected = false;
	XSetErrorHandler(&WindowManager::OnWMDetected);
	XSelectInput(_display, _root, SubstructureRedirectMask | SubstructureNotifyMask);
	XSync(_display, false);
	if (_wm_detected) {
	    std::cerr << "Detected another window manager on display " << XDisplayString(_display) << std::endl;
	    assert(!_wm_detected);
	}
    }
    XSetErrorHandler(&WindowManager::OnXError);

    {// reparenting..
	Window root_return, parent_return;
	Window* childrens;
	unsigned int num_top_level_windows;

	XGrabServer(_display);
	XQueryTree(_display, _root, &root_return, &parent_return, &childrens, &num_top_level_windows); // get window tree
	for (unsigned int i = 0; i < num_top_level_windows; ++i) {
	    frame(childrens[i]); // reset parent "already here" window to our root window of our window manager
	}
	XFree(childrens);
	XUngrabServer(_display);
    } // end reparenting...

    std::cout << "entering inside event loop" << std::endl;
    while (true) {
	XEvent e;
	std::cout << "ev get..." << std::endl;
	XNextEvent(_display, &e);
	std::cout << "Received event: " << e.type  << std::endl;
	_autoExec(e.type)(e);
    }
}

int newin::WindowManager::OnWMDetected(Display*, XErrorEvent* e) {
    if (e->error_code == BadAccess) {
	_wm_detected = true;
    } else {
	_wm_detected = false;
    }
    return 0;
}

int newin::WindowManager::OnXError(Display*, XErrorEvent* e) {
    std::cerr << "XError:" << e->error_code << std::endl;
    return 0;
}

void newin::WindowManager::frame(Window w) { // ugly un-understandable function copied from a tutorial =/
    /** apparently map the frammebuffer of the client window on the X vieuport */
  // Visual properties of the frame to create.
  const unsigned int BORDER_WIDTH = 0;
  const unsigned long BORDER_COLOR = 0xffffff;
  const unsigned long BG_COLOR = 0xffffff;

  assert(!_clients.count(w));

  // 1. Retrieve attributes of window to frame.
  XWindowAttributes x_window_attrs;
  assert(XGetWindowAttributes(_display, w, &x_window_attrs));
  // 2. Create frame.
  const Window frame = XCreateSimpleWindow( _display, _root, x_window_attrs.x, x_window_attrs.y, x_window_attrs.width, x_window_attrs.height, BORDER_WIDTH, BORDER_COLOR, BG_COLOR);
  // 3. Select events on frame.
  XSelectInput( _display, frame, SubstructureRedirectMask | SubstructureNotifyMask);
  // 4. Add client to save set, so that it will be restored and kept alive if we crash.
  XAddToSaveSet(_display, w);
  // 5. Reparent client window.
  XReparentWindow( _display, w, frame, 0, 0);  // Offset of client window within frame.
  // 6. Map frame.
  XMapWindow(_display, frame);
  // 7. Save frame handle.
  _clients[w] = frame;

  /** reminder -> Mod1Mask=alt Mod4Mask = super */

  /** Button mean mouse button ans Key mean keyboard button... */
  //   a. Move windows with alt + left button.
  XGrabButton( _display, Button1, Mod1Mask, w, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  //   b. Resize windows with alt + right button.
  XGrabButton( _display, Button3, Mod1Mask, w, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  //   c. Kill windows with alt + f4. (but don't know what argument call the destruction...)
  XGrabKey( _display, XKeysymToKeycode(_display, XK_F4), Mod1Mask, w, false, GrabModeAsync, GrabModeAsync);
  //   d. Switch windows with alt + tab. (same... I don't know how to bind the selection...)
  XGrabKey( _display, XKeysymToKeycode(_display, XK_Tab), Mod1Mask, w, false, GrabModeAsync, GrabModeAsync);

  /** btw after some test key dont effecively do anything... */
}

void newin::WindowManager::unframe(Window w) {
  assert(_clients.count(w));

  // We reverse the steps taken in Frame().
  const Window frame = _clients[w];
  // 1. Unmap frame.
  XUnmapWindow(_display, frame);
  // 2. Reparent client window.
  XReparentWindow( _display, w, _root, 0, 0);  // Offset of client window within root.
  // 3. Remove client window from save set, as it is now unrelated to us.
  XRemoveFromSaveSet(_display, w);
  // 4. Destroy frame.
  XDestroyWindow(_display, frame);
  // 5. Drop reference to frame handle.
  _clients.erase(w);

  std::cout << "Unframed window " << w << " [" << frame << "]";
}

void newin::WindowManager::_keyPress(const XEvent&) { std::cout << "key pressed" << std::endl; } /** seems called when XGrabButton is set for the frame... */
void newin::WindowManager::_create(const XEvent&) { std::cout << "creat notification recieved" << std::endl; }
void newin::WindowManager::_destroy(const XEvent&) { std::cout << "destruction notification recieved" << std::endl; }
void newin::WindowManager::_map(const XEvent&) { std::cout << "map notify" << std::endl; }
void newin::WindowManager::_reparent(const XEvent&) { std::cout << "reparent notify" << std::endl; }

void newin::WindowManager::_configureRequest(const XEvent& re) {
    const XConfigureRequestEvent& e = re.xconfigurerequest;
    XWindowChanges changes;
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;
    if (_clients.count(e.window)) {
	const Window f = _clients[e.window];
	XConfigureWindow(_display, f, e.value_mask, &changes);
	std::cout << "Resize [" << f << "] to " << e.width << e.height;
    }
    XConfigureWindow(_display, e.window, e.value_mask, &changes);
    std::cout << "Resize " << e.window << " to " << e.width << e.height;
}

void newin::WindowManager::_unmap(const XEvent& ev) {
    const XUnmapEvent& e = ev.xunmap;
    std::cout << "unmap" << std::endl; 
    if (!_clients.count(e.window)) {
	std::cout << "Ignore UnmapNotify for non-client window " << e.window << std::endl;
	return;
    }
    if (e.event == _root) {
	std::cout << "Ignore UnmapNotify for reparented pre-existing window " << e.window << std::endl;
	return;
    }
    unframe(e.window);
}

void newin::WindowManager::_mapReq(const XEvent& ev) { /** when a window is beeing constructed a mapping of their framebuffer to Xserver is required */
    const XMapRequestEvent& e = ev.xmaprequest;
    std::cout << "mapping request" << std::endl; 
    frame(e.window);
    XMapWindow(_display, e.window);
}

void newin::WindowManager::onEvent(int eventType, std::function<void(const XEvent&)> callBack) {
    callbacks[eventType] = callBack;
}

void newin::WindowManager::clearEvents() {
    callbacks.clear();
}

void newin::WindowManager::resetEvents() {
    clearEvents();
    callbacks[KeyPress] = _genBind(&::newin::WindowManager::_keyPress);
    callbacks[CreateNotify] = _genBind(&::newin::WindowManager::_create);
    callbacks[DestroyNotify] = _genBind(&::newin::WindowManager::_destroy);
    callbacks[MapNotify] = _genBind(&::newin::WindowManager::_map);
    callbacks[UnmapNotify] = _genBind(&::newin::WindowManager::_unmap);
    callbacks[MapRequest] = _genBind(&::newin::WindowManager::_mapReq);
    callbacks[ConfigureNotify] = _genBind(&::newin::WindowManager::_configureRequest);
    callbacks[ReparentNotify] = _genBind(&::newin::WindowManager::_reparent);
}

/** from IDevice */
void newin::WindowManager::exec(AConfigurator* c) {
    std::cout << "executing cmd: " << c->getExecObject().getId() << " with args:" << std::endl;
    while (!c->empty()) {
	std::cout << c->getExecObject().getId() << std::endl;
	_autoExec(c->getExecObject().getId());
	c->popFirst();
    }
}

std::vector<int> newin::WindowManager::getViableIds()const {
    std::vector<int> cap;
    cap.reserve(36);
    int i = 2;
    while (i <= 35) {
	cap.push_back(i);
    }
    return cap;
}

std::string newin::WindowManager::getKeyworkById(int)const {
    return "none";
}
