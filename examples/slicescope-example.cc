#include "ns3/core-module.h"
#include "ns3/slicescope-helper.h"

/**
 * \file
 *
 * Explain here what the example does.
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SliceScopeExample");

int
main(int argc, char* argv[])
{
    bool verbose = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "Tell application to log if true", verbose);

    cmd.Parse(argc, argv);

    LogComponentEnable("SliceScopeExample", LOG_LEVEL_INFO);
    NS_LOG_INFO("Hello World");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
