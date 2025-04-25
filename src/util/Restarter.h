#ifndef RESTARTER_H
#define RESTARTER_H

#include <ESP.h>
#include "Logger.h"

class Restarter
{
public:
    static void restart()
    {
        Logger::log("Restarting ESP now");
        delay(100);
//        ESP.restart();
    }
};

#endif //RESTARTER_H
