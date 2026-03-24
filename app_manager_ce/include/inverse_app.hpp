#ifndef INVERSE_APP_HPP
#define INVERSE_APP_HPP

#include "app.hpp"
#include "inverse_solver.hpp"

class InverseApp : public App {
public:
    const char* getName() const override { return "Matrix Inverse"; }
    void run(TIGui& gui) override;
};

#endif // INVERSE_APP_HPP
