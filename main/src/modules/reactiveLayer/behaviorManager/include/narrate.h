#include <string>
#include <iostream>
#include <iomanip>
#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <map>


#include "behavior.h"

using namespace std;
using namespace yarp::os;
using namespace yarp::sig;



class Narrate: public Behavior
{
private:
    void run(Bottle args=Bottle());
    
public:
    Narrate(Mutex* mut): Behavior(mut) {
        ;
    }
        
    void configure();
    void close_extra_ports() {
        ;
    }
};
