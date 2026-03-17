#ifndef APP_HPP
#define APP_HPP

#include "ti_gui.hpp"
#include <string>

class App {
public:
    virtual ~App() = default;
    virtual std::string getName() const = 0;
    virtual void run(TIGui& gui) = 0;
};

#endif // APP_HPP
