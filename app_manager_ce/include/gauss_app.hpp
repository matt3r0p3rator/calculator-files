#ifndef GAUSS_APP_HPP
#define GAUSS_APP_HPP

#include "app.hpp"

class GaussApp : public App {
public:
    const char* getName() const override { return "Gauss-Jordan Elimination"; }
    void run(TIGui& gui) override;
};

#endif // GAUSS_APP_HPP
