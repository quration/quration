#include "qret/analysis/circuit_drawer.h"

#include <gtest/gtest.h>

#include <iostream>
#include <numbers>

using namespace qret;

TEST(CircuitDrawer, Generate) {
    auto drawer = CircuitDrawer(4, 10);
    drawer.Barrier("section 0");
    drawer.I(0);
    drawer.X(1);
    drawer.Y(1);
    drawer.Z(0);
    drawer.H(2);
    drawer.Measure(2, 0);
    drawer.Measure(0, 1);
    drawer.S(3);
    drawer.SDag(3);
    drawer.T(3);
    drawer.TDag(3);
    drawer.Barrier("section 1");
    drawer.RX(0, 0);
    drawer.RY(1, std::numbers::pi);
    drawer.RZ(2, std::numbers::pi * 2);
    drawer.RX(3, -std::numbers::pi);
    drawer.RX(0, -std::numbers::pi * 3);
    drawer.RX(1, std::numbers::pi / 4);
    drawer.RX(2, std::numbers::pi * 3 / 5);
    drawer.RX(3, -std::numbers::pi / 2);
    drawer.RX(0, -std::numbers::pi * 3 / 2);
    drawer.RX(1, std::numbers::pi * 0.1234567890123456789);
    drawer.RotateGlobalPhase(std::numbers::pi);
    drawer.Barrier("section 2");
    drawer.CX(0, 1);
    drawer.CY(3, 2);
    drawer.CZ(0, 2);
    drawer.CX(3, 0);
    drawer.CCX(0, 1, 3);
    drawer.CCY(1, 0, 2);
    drawer.CCZ(3, 1, 0);
    drawer.MCX(2, {0, 1, 3});
    drawer.Barrier("section 3");
    drawer.Call("call1", {0, 2}, {}, {});
    drawer.Call("call2", {3}, {0, 1}, {1});
    drawer.Call("call3", {}, {0}, {0});
    drawer.ClassicalFunction("classical1", {1}, {1});
    drawer.ClassicalFunction("classical2", {}, {0, 1});
    drawer.CRand({0.1, 10.0, 0.7, 5.0, 0.88}, {2, 3}, 1);
    drawer.CRand({0.1, 10.0, 0.7, 5.0, 0.88}, {4, 5}, 2);
    drawer.CRand({0.1, 10.0, 0.7, 5.0, 0.88}, {6, 7}, 3);
    drawer.CRand({0.1, 10.0, 0.7, 5.0, 0.88}, {8, 9}, 4);

    drawer.Print(
            std::cout,
            {"$\\ket{0}$", "$\\ket{1}$", "$\\ket{2}$", "$\\ket{3}$", "$R_0$", "$R_1$"}
    );
    std::cout << std::endl;
}
