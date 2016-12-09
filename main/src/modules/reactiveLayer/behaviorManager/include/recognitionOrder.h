#include <string>
#include <iostream>
#include <iomanip>
#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <map>


#include "behavior.h"

class recognitionOrder: public Behavior
{
private:
    void run(const yarp::os::Bottle &args);
    yarp::os::Port port_to_homeo;
    std::string port_to_homeo_name;
    std::string homeoPort;

public:
    recognitionOrder(yarp::os::Mutex* mut, yarp::os::ResourceFinder &rf, std::string behaviorName): Behavior(mut, rf, behaviorName) {
        ;
    }
       
    void configure();

    void close_extra_ports() {
        ;
    }
    bool manual;
};

