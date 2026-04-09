/* fanet-aodv-bee.cc
 * FANET Simulation: BeeAdHoc vs AODV — NS-3 3.45
 *
 * FIXES applied vs original:
 *  1. numFlows cap raised from 3 → cfg.numUavs/2 (up to 5 flows for 10 UAVs)
 *  2. BeeAdHoc app start pushed to 20 s (more route-discovery settle time)
 *  3. PackerTimeout reduced to 10 s (was 15 s — less than simTime-appStart)
 *  4. WallClock watchdog: kills Simulator::Run() if wall time exceeds maxWall
 *  5. Simulator::Stop() guard event re-posted just before Run() as insurance
 *  6. CollectFlowMonitorResults now includes zero-rx flows in Tx totals
 *  7. Energy depletion callback logs node ID when a node runs out of energy
 *  8. RandomWalk Speed string now uses double-safe formatting
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

#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace ns3;
using namespace ns3::beeadhoc;
using namespace ns3::energy;

//NS_LOG_COMPONENT_DEFINE("FanetAodvBee");

// ──────────────────────────────────────────────────────────────────────────────
// Configuration & Results
// ──────────────────────────────────────────────────────────────────────────────

struct SimConfig
{
    uint32_t    numUavs      = 50;
    double      simTime      = 120.0;   // simulation seconds
    double      txPower      = 20.0;    // dBm
    double      areaSize     = 1500.0;   // metres
    double      fixedAlt     = 100.0;   // metres (z)
    double      uavSpeed     = 10.0;     // m/s max
    bool        useEnergy    = true;
    std::string outputPrefix = "fanet";
    double      maxWallSec   = 300.0;   // watchdog: abort after this many real seconds
};

struct SimResults
{
    std::string protocol;
    double   avgThroughput     = 0.0;
    double   avgDelay          = 0.0;
    double   avgJitter         = 0.0;
    double   avgPDR            = 0.0;
    uint32_t totalTxPkts       = 0;
    uint32_t totalRxPkts       = 0;
    bool     timedOut          = false;
    double   avgEnergyConsumed = 0.0;   // Joules consumed per node (avg)
    double   avgEnergyRemaining= 0.0;   // Joules remaining per node (avg)
};

// ──────────────────────────────────────────────────────────────────────────────
// BeeAdHoc Routing Helper
// ──────────────────────────────────────────────────────────────────────────────

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
        // FIX: timeout must be well below (simTime - appStart) so the protocol
        //      doesn't keep waiting for routes past Simulator::Stop().
        bee->SetPackerTimeout(Seconds(10.0));
        bee->SetDebugTracing(false);
        return bee;
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// Wall-clock watchdog
// Runs in a background thread; calls Simulator::Stop() if the sim hangs.
// ──────────────────────────────────────────────────────────────────────────────

static std::atomic<bool> g_simDone{false};

static void
StartWatchdog(double maxWallSec)
{
    std::thread([maxWallSec]() {
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::duration<double>(maxWallSec);
        while (!g_simDone.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (std::chrono::steady_clock::now() >= deadline) {
                NS_LOG_UNCOND("\n[WATCHDOG] Wall-clock limit ("
                              << maxWallSec << " s) exceeded — forcing Simulator::Stop()");
                Simulator::Stop();
                return;
            }
        }
    }).detach();
}

// ──────────────────────────────────────────────────────────────────────────────
// Wi-Fi
// ──────────────────────────────────────────────────────────────────────────────

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

// ──────────────────────────────────────────────────────────────────────────────
// Mobility
// ──────────────────────────────────────────────────────────────────────────────

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

    // FIX: use ostringstream for safe double → string conversion in attribute string
    std::ostringstream speedStr;
    speedStr << "ns3::UniformRandomVariable[Min=1.0|Max="
             << std::fixed << std::setprecision(1) << cfg.uavSpeed << "]";

    mob.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds",   RectangleValue(Rectangle(0, cfg.areaSize, 0, cfg.areaSize)),
        "Speed",    StringValue(speedStr.str()),
        "Distance", DoubleValue(50.0));

    mob.Install(nodes);

    // Lift all nodes to fixed altitude
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<MobilityModel> mm = nodes.Get(i)->GetObject<MobilityModel>();
        Vector pos = mm->GetPosition();
        pos.z = cfg.fixedAlt;
        mm->SetPosition(pos);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Energy
// ──────────────────────────────────────────────────────────────────────────────

static void
EnergyDepletedCallback(uint32_t nodeId)
{
    NS_LOG_UNCOND("[Energy] Node " << nodeId
                  << " depleted at t=" << Simulator::Now().GetSeconds() << " s");
}

// Free function wrapper so MakeBoundCallback can bind nodeId and satisfy the
// RemainingEnergy trace signature (double oldE, double newE).
static void
EnergyTraceWrapper(uint32_t nodeId, double oldE, double newE)
{
    if (newE <= 0.0 && oldE > 0.0) {
        EnergyDepletedCallback(nodeId);
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

    // Register depletion callbacks
    for (uint32_t i = 0; i < src.GetN(); ++i)
    {
        uint32_t nodeId = nodes.Get(i)->GetId();
        Ptr<BasicEnergySource> es = DynamicCast<BasicEnergySource>(src.Get(i));
        if (es)
        {
            es->TraceConnectWithoutContext(
                "RemainingEnergy",
                MakeBoundCallback(&EnergyTraceWrapper, nodeId));
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Traffic
// ──────────────────────────────────────────────────────────────────────────────

static void
InstallTraffic(const SimConfig& cfg,
               const Ipv4InterfaceContainer& ifaces,
               NodeContainer& nodes,
               double appStart)
{
    uint16_t port = 9;

    // FIX: allow up to numUavs/2 flows (was capped at 3)
    uint32_t numFlows = cfg.numUavs / 2;   // 5 flows for 10 UAVs

    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApps = sink.Install(nodes);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(cfg.simTime));

    NS_LOG_UNCOND("  Node IP assignments (" << numFlows << " flows):");
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
        app.Start(Seconds(appStart + i * 0.5));          // stagger by 0.5 s
        app.Stop(Seconds(cfg.simTime - 2.0));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Energy Metrics Collection
// Called after Simulator::Run(), before Simulator::Destroy().
// Reads each node's BasicEnergySource to compute per-node consumed/remaining,
// then stores the network-wide averages in the SimResults struct.
// ──────────────────────────────────────────────────────────────────────────────

static void
CollectEnergyMetrics(NodeContainer& nodes, SimResults& res)
{
    const double initialEnergyJ = 200.0;   // must match BasicEnergySourceInitialEnergyJ

    double totalConsumed  = 0.0;
    double totalRemaining = 0.0;
    uint32_t count = 0;

    NS_LOG_UNCOND("\n─────────────────────────────────────────────────────");
    NS_LOG_UNCOND("  Protocol: " << res.protocol << " | Per-Node Energy Stats");
    NS_LOG_UNCOND("─────────────────────────────────────────────────────");

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<EnergySourceContainer> esc =
            nodes.Get(i)->GetObject<EnergySourceContainer>();
        if (!esc || esc->GetN() == 0) { continue; }

        Ptr<BasicEnergySource> es =
            DynamicCast<BasicEnergySource>(esc->Get(0));
        if (!es) { continue; }

        double remaining = es->GetRemainingEnergy();
        double consumed  = initialEnergyJ - remaining;

        NS_LOG_UNCOND("  Node " << nodes.Get(i)->GetId()
            << "  Consumed=" << std::fixed << std::setprecision(4) << consumed  << " J"
            << "  Remaining=" << remaining << " J");

        totalConsumed  += consumed;
        totalRemaining += remaining;
        ++count;
    }

    if (count > 0)
    {
        res.avgEnergyConsumed  = totalConsumed  / count;
        res.avgEnergyRemaining = totalRemaining / count;
    }

    NS_LOG_UNCOND("  → Avg Consumed : " << std::fixed << std::setprecision(4)
                  << res.avgEnergyConsumed  << " J");
    NS_LOG_UNCOND("  → Avg Remaining: " << res.avgEnergyRemaining << " J");
}

// ──────────────────────────────────────────────────────────────────────────────
// Flow Monitor Collection
// ──────────────────────────────────────────────────────────────────────────────

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

    double   tT = 0, tD = 0, tJ = 0, tP = 0;
    uint64_t tTx = 0, tRx = 0;
    uint32_t nActive = 0;   // flows with rxPackets > 0

    NS_LOG_UNCOND("\n─────────────────────────────────────────────────────");
    NS_LOG_UNCOND("  Protocol: " << proto << " | Per-Flow Stats");
    NS_LOG_UNCOND("─────────────────────────────────────────────────────");
    NS_LOG_UNCOND("  Total flows detected: " << stats.size());

    for (auto& kv : stats)
    {
        auto& s = kv.second;
        auto  t = classifier->FindFlow(kv.first);

        double dur    = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double tput   = (s.rxPackets > 0 && dur > 0)
                        ? s.rxBytes * 8.0 / dur / 1e3 : 0.0;
        double delay  = (s.rxPackets > 0)
                        ? s.delaySum.GetSeconds() / s.rxPackets * 1e3 : 0.0;
        double jitter = (s.rxPackets > 1)
                        ? s.jitterSum.GetSeconds() / (s.rxPackets - 1) * 1e3 : 0.0;
        double pdr    = (s.txPackets > 0)
                        ? 100.0 * s.rxPackets / s.txPackets : 0.0;

        NS_LOG_UNCOND("Flow " << kv.first
            << "  " << t.sourceAddress << " -> " << t.destinationAddress
            << "\n  Tx=" << s.txPackets << "  Rx=" << s.rxPackets
            << "  Lost=" << s.lostPackets
            << "  Tput=" << std::fixed << std::setprecision(2) << tput << " Kbps"
            << "  Delay=" << delay << " ms"
            << "  Jitter=" << jitter << " ms"
            << "  PDR=" << pdr << "%");

        // FIX: always accumulate Tx/Rx counts (not gated on rxPackets > 0)
        tTx += s.txPackets;
        tRx += s.rxPackets;

        if (s.rxPackets > 0) {
            tT += tput;
            tD += delay;
            tJ += jitter;
            tP += pdr;
            ++nActive;
        }
    }

    if (nActive > 0) {
        res.avgThroughput = tT / nActive;
        res.avgDelay      = tD / nActive;
        res.avgJitter     = tJ / nActive;
        res.avgPDR        = tP / nActive;
    }
    res.totalTxPkts = static_cast<uint32_t>(tTx);
    res.totalRxPkts = static_cast<uint32_t>(tRx);
    return res;
}

// ──────────────────────────────────────────────────────────────────────────────
// AODV Run
// ──────────────────────────────────────────────────────────────────────────────

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

    InstallTraffic(cfg, ifaces, nodes, 10.0);   // apps start at 10 s

    FlowMonitorHelper fmh;
    auto monitor = fmh.InstallAll();

    // FIX: re-post Stop() event as last-resort insurance
    // Cast resolves the overload ambiguity between Stop() and Stop(Time).
    Simulator::Stop(Seconds(cfg.simTime));
    Simulator::Schedule(Seconds(cfg.simTime),
                        static_cast<void (*)()>(&Simulator::Stop));

    g_simDone = false;
    StartWatchdog(cfg.maxWallSec);

    Simulator::Run();
    g_simDone = true;

    auto res = CollectFlowMonitorResults(monitor, fmh, "AODV");
    monitor->SerializeToXmlFile(cfg.outputPrefix + "-AODV-flowmon.xml", true, true);

    if (cfg.useEnergy) {
        CollectEnergyMetrics(nodes, res);
    }

    Simulator::Destroy();
    return res;
}

// ──────────────────────────────────────────────────────────────────────────────
// BeeAdHoc Run
// ──────────────────────────────────────────────────────────────────────────────

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

    // FIX: start at 20 s so the bee protocol has time to discover routes
    InstallTraffic(cfg, ifaces, nodes, 20.0);

    FlowMonitorHelper fmh;
    auto monitor = fmh.InstallAll();

    // FIX: re-post Stop() event as last-resort insurance
    // Cast resolves the overload ambiguity between Stop() and Stop(Time).
    Simulator::Stop(Seconds(cfg.simTime));
    Simulator::Schedule(Seconds(cfg.simTime),
                        static_cast<void (*)()>(&Simulator::Stop));

    g_simDone = false;
    StartWatchdog(cfg.maxWallSec);

    Simulator::Run();
    g_simDone = true;

    auto res = CollectFlowMonitorResults(monitor, fmh, "BeeAdHoc");
    monitor->SerializeToXmlFile(cfg.outputPrefix + "-BeeAdHoc-flowmon.xml", true, true);

    if (cfg.useEnergy) {
        CollectEnergyMetrics(nodes, res);
    }

    Simulator::Destroy();
    return res;
}

// ──────────────────────────────────────────────────────────────────────────────
// Comparison Table
// ──────────────────────────────────────────────────────────────────────────────

static void
PrintComparison(const SimResults& bee, const SimResults& aodv)
{
    auto fmt = [](double v, int p = 2) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(p) << v;
        return ss.str();
    };
    auto row = [](const std::string& metric,
                  const std::string& beeVal,
                  const std::string& aodvVal) {
        NS_LOG_UNCOND("║ " << std::left << std::setw(18) << metric
                      << " ║ " << std::setw(15) << beeVal
                      << " ║ " << std::setw(12) << aodvVal << " ║");
    };

    NS_LOG_UNCOND("\n╔══════════════════════════════════════════════════════╗");
    NS_LOG_UNCOND(  "║      FANET Routing Comparison: BeeAdHoc vs AODV     ║");
    if (bee.timedOut || aodv.timedOut) {
        NS_LOG_UNCOND("║  WARNING: one or more runs terminated by watchdog   ║");
    }
    NS_LOG_UNCOND(  "╠════════════════════╦═════════════════╦══════════════╣");
    NS_LOG_UNCOND(  "║ Metric             ║ BeeAdHoc        ║ AODV         ║");
    NS_LOG_UNCOND(  "╠════════════════════╬═════════════════╬══════════════╣");
    row("Throughput (Kbps)", fmt(bee.avgThroughput),  fmt(aodv.avgThroughput));
    row("Avg Delay (ms)",    fmt(bee.avgDelay),        fmt(aodv.avgDelay));
    row("Avg Jitter (ms)",   fmt(bee.avgJitter),       fmt(aodv.avgJitter));
    row("PDR (%)",           fmt(bee.avgPDR),          fmt(aodv.avgPDR));
    row("Tx Packets",        std::to_string(bee.totalTxPkts),
                             std::to_string(aodv.totalTxPkts));
    row("Rx Packets",        std::to_string(bee.totalRxPkts),
                             std::to_string(aodv.totalRxPkts));
    row("Avg E-Consumed (J)", fmt(bee.avgEnergyConsumed, 4),  fmt(aodv.avgEnergyConsumed, 4));
    row("Avg E-Remain  (J)", fmt(bee.avgEnergyRemaining, 4), fmt(aodv.avgEnergyRemaining, 4));
    NS_LOG_UNCOND(  "╚════════════════════╩═════════════════╩══════════════╝");
}

// ──────────────────────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    SimConfig   cfg;
    std::string runMode = "both";

    CommandLine cmd;
    cmd.AddValue("numUavs",     "Number of UAV nodes",           cfg.numUavs);
    cmd.AddValue("simTime",     "Simulation time (s)",           cfg.simTime);
    cmd.AddValue("txPower",     "TX power (dBm)",                cfg.txPower);
    cmd.AddValue("areaSize",    "Area size (m)",                  cfg.areaSize);
    cmd.AddValue("uavSpeed",    "Max UAV speed (m/s)",            cfg.uavSpeed);
    cmd.AddValue("useEnergy",   "Enable energy model",            cfg.useEnergy);
    cmd.AddValue("runMode",     "bee | aodv | both",              runMode);
    cmd.AddValue("output",      "Output file prefix",             cfg.outputPrefix);
    cmd.AddValue("maxWallSec",  "Watchdog wall-clock limit (s)",  cfg.maxWallSec);
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