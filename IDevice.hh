#ifndef IDEVICE_H_
# define IDEVICE_H_
#include <vector>
#include <utility>
#include <iostream>
#include <string>

class AConfigurator {
    private:
	class _execObject {
	    private:
		std::vector<std::string> _args;
		int _cmd;
	    public:
		_execObject() : _cmd(0) {}
		/** add something to do to the "todo list"*/
		void pushArg(const std::string& s) { _args.push_back(s); }
		void setId(int c) { _cmd = c; }
		int getId() const { return _cmd; }
	};
	std::vector<_execObject> _todoList;
    public:
	typedef _execObject ExecObject;
	AConfigurator() {
	}
	virtual void pushToDo(const ExecObject& e) {
	    _todoList.push_back(e);
	}
	virtual const ExecObject& getExecObject() const {
	    if (_todoList.empty()) {
		std::cerr << "WARNING: todolist is empty" << std::endl;
	    }
	    return _todoList[0];
	}
	virtual void popFirst() {
	    if (_todoList.size()) {
		_todoList.erase(_todoList.begin());
	    }
	}
	virtual bool empty() const {
	    return _todoList.empty();
	}
};

class IDevice {
    public:
	virtual void exec(AConfigurator*) = 0;
	/** every command has an ID this function will return all viable ID */
	virtual std::vector<int> getViableIds()const = 0;
	/** to know what an ID shoud do */
	virtual std::string getKeyworkById(int)const = 0;
	virtual ~IDevice() {}
};

#endif /* !IDEVICE_H_ */
