#include "wrdac/clients/icubClient.h"
#include <time.h>

using namespace std;
using namespace yarp::os;
using namespace wysiwyd::wrdac;

class opcPopulater : public RFModule {
private:

    ICubClient *iCub;
    double      period;
    Port        rpc;

public:
    bool configure(yarp::os::ResourceFinder &rf);

    bool interruptModule()
    {
        return true;
    }

    bool close();

    double getPeriod()
    {
        return period;
    }


    bool updateModule();
    bool    populateSpecific(Bottle bInput);
    //RPC & scenarios
    bool respond(const Bottle& cmd, Bottle& reply);
};