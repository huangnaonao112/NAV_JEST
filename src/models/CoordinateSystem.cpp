#include "CoordinateSystem.h"

CoordinateSystem* CoordinateSystem::instance() {
    static CoordinateSystem inst;
    return &inst;
}