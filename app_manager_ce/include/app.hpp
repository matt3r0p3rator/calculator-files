#ifndef APP_HPP
#define APP_HPP

#include "ce_gui.hpp"

class App {
public:
    virtual ~App() = default;
    virtual const char* getName() const = 0;
    virtual void run(TIGui& gui) = 0;
};

#endif // APP_HPP
