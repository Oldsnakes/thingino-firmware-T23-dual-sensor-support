#ifndef Daynight_hpp
#define Daynight_hpp
#include "imp_hal.hpp"

#include <memory>
#include <thread>
#include <atomic>
#include "Config.hpp"
#include "Logger.hpp"
#include "globals.hpp"
#include "imp/imp_system.h"

#include <vector>
#include <string>

class Daynight {
    public:

        void detect();
        static void *run(void* arg);
        int init();
        int exit();

    private:
        int action(bool active);
};

#endif /* Motion_hpp */
