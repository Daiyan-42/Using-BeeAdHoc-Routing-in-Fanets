/* fanet-aodv-bee.cc
 * FANET Simulation: BeeAdHoc vs AODV — NS-3 3.45
 *
 * Reuses the UAV topology, Wi-Fi settings, mobility, and traffic pattern from
 * fanet-aodv-dsr.cc, but compares only protocols that are available in the
 * current workspace build: AODV and BeeAdHoc.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"

#include "bee-adhoc.h"

#include <iomanip>
#include <sstream>

using namespace ns3;
using namespace ns3::beeadhoc;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("FanetAodvBee");

struct SimConfig
{
    uint32_t    numUavs      = 10;
    double      simTime      = 100.0;
    double      txPower      = 20.0;
    double      areaSize     = 300.0;
    double      fixedAlt     = 100.0;
    double      uavSpeed     = 5.0;
    bool        useEnergy    = true;
    std::string outputPrefix = "fanet";
};

struct SimResults
{
    std::string protocol;
    double   avgThroughput = 0.0;
    double   avgDelay      = 0.0;
    double   avgJitter     = 0.0;
    double   avgPDR        = 0.0;
    uint32_t totalTxPkts   = 0;
    uint32_t totalRxPkts   = 0;
};

class BeeAdHocHelper : public Ipv4RoutingHelper
{
public:
    BeeAdHocHelper* Copy() const override { return new BeeAdHocHelper(*this); }

    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override
    {
        Ptr<BeeAdHocRoutingProtocol> bee = CreateObject<BeeAdHocRoutingProtocol>();
        bee->SetEnergyThreshold(0.1);
        bee->SetSwarmThreshold(3);
        bee->SetInitialDanceNum(5);
        bee->SetPackerTimeout(Seconds(15.0));
        bee->SetDebugTracing(false);
        return bee;
    }
};

static NetDeviceContainer
CreateWifiDevices(NodeContainer& nodes, const SimConfig& cfg)
{
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",    StringValue("DsssRate11Mbps"),
                                 "ControlMode", StringValue("DsssRate1Mbps"));

    YansWifiChannelHelper ch;
    ch.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    ch.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                          "Exponent",          DoubleValue(2.0),
                          "ReferenceDistance", DoubleValue(1.0),
                          "ReferenceLoss",     DoubleValue(46.67));

    YansWifiPhyHelper phy;
    phy.SetChannel(ch.Create());
    phy.Set("TxPowerStart",  DoubleValue(cfg.txPower));
    phy.Set("TxPowerEnd",    DoubleValue(cfg.txPower));
    phy.Set("RxSensitivity", DoubleValue(-95.0));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    return wifi.Install(phy, mac, nodes);
}

static void
SetupMobility(NodeContainer& nodes, const SimConfig& cfg)
{
    MobilityHelper mob;
    mob.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX",       DoubleValue(50.0),
        "MinY",       DoubleValue(50.0),
        "DeltaX",     DoubleValue(80.0),
        "DeltaY",     DoubleValue(80.0),
        "GridWidth",  UintegerValue(4),
        "LayoutType", StringValue("RowFirst"));

    mob.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds",   RectangleValue(Rectangle(0, cfg.areaSize, 0, cfg.areaSize)),
        "Speed",    StringValue("ns3::UniformRandomVariable[Min=1|Max="
                                + std::to_string((int)cfg.uavSpeed) + "]"),
        "Distance", DoubleValue(50.0));

    mob.Install(nodes);

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<MobilityModel> mm = nodes.Get(i)->GetObject<MobilityModel>();
        Vector pos = mm->GetPosition();
        pos.z = cfg.fixedAlt;
        mm->SetPosition(pos);
    }
}

static void
InstallEnergy(NodeContainer& nodes, NetDeviceContainer& devices)
{
    BasicEnergySourceHelper esh;
    esh.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(200.0));

    WifiRadioEnergyModelHelper reh;
    reh.Set("TxCurrentA",   DoubleValue(0.0174));
    reh.Set("RxCurrentA",   DoubleValue(0.0197));
    reh.Set("IdleCurrentA", DoubleValue(0.0073));

    EnergySourceContainer src = esh.Install(nodes);
    reh.Install(devices, src);
}

static SimResults
CollectFlowMonitorResults(Ptr<FlowMonitor> monitor,
                          FlowMonitorHelper& fmh,
                          const std::string& proto)
{
    monitor->CheckForLostPackets();
    auto classifier = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    auto stats      = monitor->GetFlowStats();

    SimResults res;
    res.protocol = proto;

    double   tT=0, tD=0, tJ=0, tP=0;
    uint64_t tTx=0, tRx=0;
    uint32_t n=0;

    NS_LOG_UNCOND("\n─────────────────────────────────────────────────────");
    NS_LOG_UNCOND("  Protocol: " << proto << " | Per-Flow Stats");
    NS_LOG_UNCOND("─────────────────────────────────────────────────────");
    NS_LOG_UNCOND("  Total flows: " << stats.size());

    for (auto& kv : stats)
    {
        auto& s = kv.second;
        auto  t = classifier->FindFlow(kv.first);

        double dur    = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double tput   = (s.rxPackets > 0 && dur > 0) ? s.rxBytes * 8.0 / dur / 1e3 : 0;
        double delay  = (s.rxPackets > 0) ? s.delaySum.GetSeconds() / s.rxPackets * 1e3 : 0;
        double jitter = (s.rxPackets > 1)
                        ? s.jitterSum.GetSeconds() / (s.rxPackets - 1) * 1e3 : 0;
        double pdr    = (s.txPackets > 0) ? 100.0 * s.rxPackets / s.txPackets : 0;

        NS_LOG_UNCOND("Flow " << kv.first
            << "  " << t.sourceAddress << " -> " << t.destinationAddress
            << "\n  Tx=" << s.txPackets << "  Rx=" << s.rxPackets
            << "  Lost=" << s.lostPackets
            << "  Tput=" << std::fixed << std::setprecision(2) << tput << " Kbps"
            << "  Delay=" << delay << " ms"
            << "  Jitter=" << jitter << " ms"
            << "  PDR=" << pdr << "%");

        if (s.rxPackets > 0) {
            tT += tput;
            tD += delay;
            tJ += jitter;
            tP += pdr;
            tTx += s.txPackets;
            tRx += s.rxPackets;
            ++n;
        }
    }

    if (n > 0) {
        res.avgThroughput = tT / n;
        res.avgDelay      = tD / n;
        res.avgJitter     = tJ / n;
        res.avgPDR        = tP / n;
    }
    res.totalTxPkts = (uint32_t)tTx;
    res.totalRxPkts = (uint32_t)tRx;
    return res;
}

static void
InstallTraffic(const SimConfig& cfg,
               const Ipv4InterfaceContainer& ifaces,
               NodeContainer& nodes,
               double appStart)
{
    uint16_t port     = 9;
    uint32_t numFlows = std::min((uint32_t)3, cfg.numUavs / 2);

    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApps = sink.Install(nodes);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(cfg.simTime));

    NS_LOG_UNCOND("  Node IP assignments:");
    for (uint32_t i = 0; i < numFlows; ++i)
    {
        uint32_t    dstId   = cfg.numUavs - 1 - i;
        Ipv4Address dstAddr = ifaces.GetAddress(dstId);
        NS_LOG_UNCOND("  Flow " << i + 1 << ": node " << i
            << " (" << ifaces.GetAddress(i) << ")"
            << " -> node " << dstId << " (" << dstAddr << ")");

        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(dstAddr, port));
        onoff.SetConstantRate(DataRate("128Kbps"), 512);
        onoff.SetAttribute("OnTime",
            StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",
            StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        auto app = onoff.Install(nodes.Get(i));
        app.Start(Seconds(appStart + i * 1.0));
        app.Stop(Seconds(cfg.simTime - 2.0));
    }
}

static SimResults
RunAodv(const SimConfig& cfg)
{
    NS_LOG_UNCOND("\n>>> Starting AODV simulation...");

    NodeContainer nodes;
    nodes.Create(cfg.numUavs);

    auto devices = CreateWifiDevices(nodes, cfg);
    SetupMobility(nodes, cfg);
    if (cfg.useEnergy) {
        InstallEnergy(nodes, devices);
    }

    AodvHelper aodv;
    aodv.Set("ActiveRouteTimeout", TimeValue(Seconds(10.0)));
    aodv.Set("HelloInterval",      TimeValue(Seconds(1.0)));
    aodv.Set("AllowedHelloLoss",   UintegerValue(3));
    aodv.Set("GratuitousReply",    BooleanValue(true));
    aodv.Set("DestinationOnly",    BooleanValue(false));
    aodv.Set("EnableHello",        BooleanValue(true));
    aodv.Set("RreqRetries",        UintegerValue(5));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    auto ifaces = ipv4.Assign(devices);

    InstallTraffic(cfg, ifaces, nodes, 10.0);

    FlowMonitorHelper fmh;
    auto monitor = fmh.InstallAll();

    Simulator::Stop(Seconds(cfg.simTime));
    Simulator::Run();

    auto res = CollectFlowMonitorResults(monitor, fmh, "AODV");
    monitor->SerializeToXmlFile(cfg.outputPrefix + "-AODV-flowmon.xml", true, true);

    Simulator::Destroy();
    return res;
}

static SimResults
RunBee(const SimConfig& cfg)
{
    NS_LOG_UNCOND("\n>>> Starting BeeAdHoc simulation...");

    NodeContainer nodes;
    nodes.Create(cfg.numUavs);

    auto devices = CreateWifiDevices(nodes, cfg);
    SetupMobility(nodes, cfg);
    if (cfg.useEnergy) {
        InstallEnergy(nodes, devices);
    }

    BeeAdHocHelper bee;

    InternetStackHelper internet;
    internet.SetRoutingHelper(bee);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    auto ifaces = ipv4.Assign(devices);

    InstallTraffic(cfg, ifaces, nodes, 15.0);

    FlowMonitorHelper fmh;
    auto monitor = fmh.InstallAll();

    Simulator::Stop(Seconds(cfg.simTime));
    Simulator::Run();

    auto res = CollectFlowMonitorResults(monitor, fmh, "BeeAdHoc");
    monitor->SerializeToXmlFile(cfg.outputPrefix + "-BeeAdHoc-flowmon.xml", true, true);

    Simulator::Destroy();
    return res;
}

static void
PrintComparison(const SimResults& bee, const SimResults& aodv)
{
    auto fmt = [](double v, int p = 2) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(p) << v;
        return ss.str();
    };
    auto row = [](const std::string& metric,
                  const std::string& beeValue,
                  const std::string& aodvValue) {
        NS_LOG_UNCOND("║ " << std::left << std::setw(18) << metric
                      << " ║ " << std::setw(15) << beeValue
                      << " ║ " << std::setw(12) << aodvValue << " ║");
    };

    NS_LOG_UNCOND("\n╔══════════════════════════════════════════════════════╗");
    NS_LOG_UNCOND(  "║      FANET Routing Comparison: BeeAdHoc vs AODV     ║");
    NS_LOG_UNCOND(  "╠════════════════════╦═════════════════╦══════════════╣");
    NS_LOG_UNCOND(  "║ Metric             ║ BeeAdHoc        ║ AODV         ║");
    NS_LOG_UNCOND(  "╠════════════════════╬═════════════════╬══════════════╣");
    row("Throughput (Kbps)", fmt(bee.avgThroughput), fmt(aodv.avgThroughput));
    row("Avg Delay (ms)",    fmt(bee.avgDelay),      fmt(aodv.avgDelay));
    row("Avg Jitter (ms)",   fmt(bee.avgJitter),     fmt(aodv.avgJitter));
    row("PDR (%)",           fmt(bee.avgPDR),        fmt(aodv.avgPDR));
    row("Tx Packets",        std::to_string(bee.totalTxPkts), std::to_string(aodv.totalTxPkts));
    row("Rx Packets",        std::to_string(bee.totalRxPkts), std::to_string(aodv.totalRxPkts));
    NS_LOG_UNCOND(  "╚════════════════════╩═════════════════╩══════════════╝");
}

int main(int argc, char* argv[])
{
    SimConfig   cfg;
    std::string runMode = "both";

    CommandLine cmd;
    cmd.AddValue("numUavs",   "Number of UAV nodes", cfg.numUavs);
    cmd.AddValue("simTime",   "Simulation time (s)", cfg.simTime);
    cmd.AddValue("txPower",   "TX power (dBm)", cfg.txPower);
    cmd.AddValue("areaSize",  "Area size (m)", cfg.areaSize);
    cmd.AddValue("uavSpeed",  "Max UAV speed (m/s)", cfg.uavSpeed);
    cmd.AddValue("useEnergy", "Enable energy model", cfg.useEnergy);
    cmd.AddValue("runMode",   "bee | aodv | both", runMode);
    cmd.AddValue("output",    "Output file prefix", cfg.outputPrefix);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(42);
    RngSeedManager::SetRun(1);

    SimResults beeRes, aodvRes;

    if (runMode == "bee" || runMode == "both") {
        beeRes = RunBee(cfg);
    }

    if (runMode == "aodv" || runMode == "both") {
        aodvRes = RunAodv(cfg);
    }

    if (runMode == "both") {
        PrintComparison(beeRes, aodvRes);
    }

    return 0;
}
