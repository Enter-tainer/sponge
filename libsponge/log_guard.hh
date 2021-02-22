#include <iostream>
#include <string>
class LogGuard {
    std::string _name;

  public:
    LogGuard(std::string name) : _name(name) { std::cerr << name << " called\n"; };
    ~LogGuard() { std::cerr << "leaving " << _name << "\n"; };
};
