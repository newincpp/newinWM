#ifndef WINDOWMANAGER_H_
# define WINDOWMANAGER_H_

extern "C" {
#include <X11/Xlib.h>
}
#include <mutex>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include "IDevice.hh"

namespace newin {
    class WindowManager : public IDevice {
	private:
	    inline std::function<void(const XEvent&)> _genBind(void (newin::WindowManager::* f)(const XEvent&)) { return std::bind(f, this, std::placeholders::_1); }
	    WindowManager(Display*);
	    static int OnXError(Display* display, XErrorEvent* e);
	    static int OnWMDetected(Display* display, XErrorEvent* e);
	    static bool _wm_detected;
	    static std::mutex _wm_detected_mutex;
	    Display* _display;
	    const Window _root;

	    // Maps top-level windows to their frame windows.
	    std::unordered_map<Window, Window> _clients;
	    void frame(Window);
	    void unframe(Window);

	    std::map<int, std::function<void(const XEvent&)> > callbacks;
	    void _keyPress(const XEvent&);	    //2
	    void _create(const XEvent&);  	    //16
	    void _destroy(const XEvent&); 	    //17
	    void _map(const XEvent&);	  	    //19 
	    void _unmap(const XEvent&);	  	    //18
	    void _mapReq(const XEvent&);  	    //20
	    void _configureRequest(const XEvent&);// 22
	    void _reparent(const XEvent&);

	    inline std::function<void(const XEvent&)> _autoExec(int x) {
		std::map<int, std::function<void(const XEvent&)> >::const_iterator i = callbacks.find(x);
		if (i != callbacks.end()) {
		    return i->second;
		} else {
		    std::cout << "\e[32;mevent ignored\e[0m" << std::endl;
		    return [](const XEvent&) {};
		}
	    }

	public:
	     ~WindowManager();
	    static std::unique_ptr<WindowManager> Create();
	    void run()noexcept;
	    void onEvent(int,std::function<void(const XEvent&)>);
	    void clearEvents();
	    void resetEvents();
	    /** from IDevice */
	    virtual void exec(AConfigurator*)override;
	    virtual std::vector<int> getViableIds()const override;
	    virtual std::string getKeyworkById(int)const override;
    };
}

#endif /* !WINDOWMANAGER_H_ */

/*
#define KeyPress		2
#define KeyRelease		3
#define ButtonPress		4
#define ButtonRelease		5
#define MotionNotify		6
#define EnterNotify		7
#define LeaveNotify		8
#define FocusIn			9
#define FocusOut		10
#define KeymapNotify		11
#define Expose			12
#define GraphicsExpose		13
#define NoExpose		14
#define VisibilityNotify	15
#define CreateNotify		16
#define DestroyNotify		17
#define UnmapNotify		18
#define MapNotify		19
#define MapRequest		20
#define ReparentNotify		21
#define ConfigureNotify		22
#define ConfigureRequest	23
#define GravityNotify		24
#define ResizeRequest		25
#define CirculateNotify		26
#define CirculateRequest	27
#define PropertyNotify		28
#define SelectionClear		29
#define SelectionRequest	30
#define SelectionNotify		31
#define ColormapNotify		32
#define ClientMessage		33
#define MappingNotify		34
#define GenericEvent		35
#define LASTEvent		36
*/
