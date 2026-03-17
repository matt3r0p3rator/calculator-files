#ifndef HELLO_APP_HPP
#define HELLO_APP_HPP

#include "app.hpp"

class HelloApp : public App {
public:
    std::string getName() const override { return "Hello World Demo"; }
    void run(TIGui& gui) override;
};

#endif // HELLO_APP_HPP