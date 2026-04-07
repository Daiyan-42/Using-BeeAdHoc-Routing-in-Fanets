/* fanet-aodv-dsr.cc
 * FANET Simulation: AODV vs DSR — NS-3 3.45
 *
 * DSR fix: uses raw TypeId sockets (same pattern as official
 * ns-3 manet-routing-compare.cc example) instead of OnOffHelper.
 * DSR intercepts packets differently and does not work correctly
 * with the high-level application helpers + FlowMonitor combo.
 *
 * Solution: use OnOffHelper for both BUT measure DSR traffic via
 * packet-level socket callbacks rather than FlowMonitor, since
 * FlowMonitor cannot see DSR-encapsulated flows.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"
#include "ns3/netanim-module.h"
#include "bee-adhoc.h"
#include <iomanip>
#include <sstream>


using namespace ns3;
using namespace ns3::dsr;
using namespace ns3::energy;
using namespace ns3::beeadhoc;

NS_LOG_COMPONENT_DEFINE("FanetAodvDsr");

// ─────────────────────────────────────────────────────────────────────────────
// DSR packet counters (socket-level, bypass FlowMonitor)
// ─────────────────────────────────────────────────────────────────────────────
static uint32_t g_dsrTxPkts   = 0;
static uint32_t g_dsrRxPkts   = 0;
static uint64_t g_dsrRxBytes  = 0;
static double   g_dsrFirstRx  = -1.0;
static double   g_dsrLastRx   = 0.0;

void DsrRxCallback(Ptr<Socket> socket)
{
    Ptr<Packet> pkt;
    Address     from;
    while ((pkt = socket->RecvFrom(from)) != nullptr)
    {
        double now = Simulator::Now().GetSeconds();
        if (g_dsrFirstRx < 0) g_dsrFirstRx = now;
        g_dsrLastRx   = now;
        g_dsrRxPkts  += 1;
        g_dsrRxBytes += pkt->GetSize();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Config
// ─────────────────────────────────────────────────────────────────────────────
struct SimConfig
{
    uint32_t    numUavs      = 10;
    double      simTime      = 100.0;
    double      txPower      = 20.0;
    double      areaSize     = 300.0;
    double      fixedAlt     = 100.0;
    double      uavSpeed     = 5.0;
    bool        verbose      = false;
    bool        useEnergy    = true;
    bool        animate      = false;
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

// ─────────────────────────────────────────────────────────────────────────────
// WiFi
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// Mobility
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// Energy
// ─────────────────────────────────────────────────────────────────────────────
static void
InstallEnergy(NodeContainer& nodes, NetDeviceContainer& devices)
{
    BasicEnergySourceHelper esh;
    esh.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(200.0));

    WifiRadioEnergyModelHelper reh;
    reh.Set("TxCurrentA",   DoubleValue(0.0174));
    reh.Set("RxCurrentA",   DoubleValue(0.0197));
    reh.Set("IdleCurrentA", DoubleValue(0.0073));

    EnergySourceContainer      src = esh.Install(nodes);
    DeviceEnergyModelContainer mdl = reh.Install(devices, src);
    (void)mdl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Results from FlowMonitor (for AODV)
// ─────────────────────────────────────────────────────────────────────────────
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
                        ? s.jitterSum.GetSeconds() / (s.rxPackets-1) * 1e3 : 0;
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
            tT+=tput; tD+=delay; tJ+=jitter; tP+=pdr;
            tTx+=s.txPackets; tRx+=s.rxPackets;
            ++n;
        }
    }

    if (n > 0) {
        res.avgThroughput = tT/n;
        res.avgDelay      = tD/n;
        res.avgJitter     = tJ/n;
        res.avgPDR        = tP/n;
    }
    res.totalTxPkts = (uint32_t)tTx;
    res.totalRxPkts = (uint32_t)tRx;
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// BeeAdHoc — same FANET topology/traffic as AODV, but with Bee routing
// ─────────────────────────────────────────────────────────────────────────────
SimResults RunBee(const SimConfig& cfg)
{
    NS_LOG_UNCOND("\n>>> Starting BeeAdHoc simulation...");

    NodeContainer nodes;
    nodes.Create(cfg.numUavs);

    auto devices = CreateWifiDevices(nodes, cfg);
    SetupMobility(nodes, cfg);

    if (cfg.useEnergy)
        InstallEnergy(nodes, devices);

    BeeAdHocHelper bee;

    InternetStackHelper internet;
    internet.SetRoutingHelper(bee);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    auto ifaces = ipv4.Assign(devices);

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
        NS_LOG_UNCOND("  Flow " << i+1 << ": node " << i
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
        app.Start(Seconds(15.0 + i * 1.0));
        app.Stop(Seconds(cfg.simTime - 2.0));
    }

    FlowMonitorHelper fmh;
    auto monitor = fmh.InstallAll();

    AnimationInterface* anim = nullptr;
    if (cfg.animate) {
        anim = new AnimationInterface(cfg.outputPrefix + "-BeeAdHoc-anim.xml");
        for (uint32_t i = 0; i < cfg.numUavs; ++i) {
            anim->UpdateNodeDescription(nodes.Get(i), "UAV-" + std::to_string(i));
            anim->UpdateNodeColor(nodes.Get(i), 0, 160, 0);
        }
    }

    Simulator::Stop(Seconds(cfg.simTime));
    Simulator::Run();

    auto res = CollectFlowMonitorResults(monitor, fmh, "BeeAdHoc");
    monitor->SerializeToXmlFile(cfg.outputPrefix + "-BeeAdHoc-flowmon.xml", true, true);

    Simulator::Destroy();
    delete anim;
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// AODV — unchanged, proven working
// ─────────────────────────────────────────────────────────────────────────────
SimResults RunAodv(const SimConfig& cfg)
{
    NS_LOG_UNCOND("\n>>> Starting AODV simulation...");

    NodeContainer nodes;
    nodes.Create(cfg.numUavs);

    auto devices = CreateWifiDevices(nodes, cfg);
    SetupMobility(nodes, cfg);

    if (cfg.useEnergy)
        InstallEnergy(nodes, devices);

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
        NS_LOG_UNCOND("  Flow " << i+1 << ": node " << i
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
        app.Start(Seconds(10.0 + i * 1.0));
        app.Stop(Seconds(cfg.simTime - 2.0));
    }

    FlowMonitorHelper fmh;
    auto monitor = fmh.InstallAll();

    AnimationInterface* anim = nullptr;
    if (cfg.animate) {
        anim = new AnimationInterface(cfg.outputPrefix + "-AODV-anim.xml");
        for (uint32_t i = 0; i < cfg.numUavs; ++i) {
            anim->UpdateNodeDescription(nodes.Get(i), "UAV-" + std::to_string(i));
            anim->UpdateNodeColor(nodes.Get(i), 0, 0, 255);
        }
    }

    Simulator::Stop(Seconds(cfg.simTime));
    Simulator::Run();

    auto res = CollectFlowMonitorResults(monitor, fmh, "AODV");
    monitor->SerializeToXmlFile(cfg.outputPrefix + "-AODV-flowmon.xml", true, true);

    Simulator::Destroy();
    delete anim;
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// DSR — uses raw UDP sockets (same as official ns-3 manet-routing-compare.cc)
//
// Why raw sockets instead of OnOffHelper?
//   DSR is not an Ipv4RoutingProtocol — it works as a layer-3.5 shim that
//   wraps packets BEFORE they reach the IP routing table. FlowMonitor hooks
//   into Ipv4 and therefore sees DSR-encapsulated packets as unclassified,
//   resulting in 0 flows. The official NS-3 example avoids this entirely by
//   using raw socket callbacks to count packets directly.
// ─────────────────────────────────────────────────────────────────────────────
SimResults RunDsr(const SimConfig& cfg)
{
    NS_LOG_UNCOND("\n>>> Starting DSR simulation...");

    // Reset global counters
    g_dsrTxPkts  = 0;
    g_dsrRxPkts  = 0;
    g_dsrRxBytes = 0;
    g_dsrFirstRx = -1.0;
    g_dsrLastRx  = 0.0;

    NodeContainer nodes;
    nodes.Create(cfg.numUavs);

    auto devices = CreateWifiDevices(nodes, cfg);
    SetupMobility(nodes, cfg);

    // Energy BEFORE internet stack (DSR requirement)
    if (cfg.useEnergy)
        InstallEnergy(nodes, devices);

    // Internet stack — NO routing helper, DSR installs separately
    InternetStackHelper internet;
    internet.Install(nodes);

    // IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    auto ifaces = ipv4.Assign(devices);

    // DSR routing — must come AFTER IP assignment
    DsrHelper     dsrHelper;
    DsrMainHelper dsrMain;
    dsrMain.Install(dsrHelper, nodes);

    // ── Traffic using raw sockets ─────────────────────────────────────────
    // This is the pattern from the official ns-3 manet-routing-compare.cc
    uint16_t port     = 9;
    uint32_t numFlows = std::min((uint32_t)3, cfg.numUavs / 2);
    uint32_t pktSize  = 512;
    double   appStart = 15.0;
    double   appStop  = cfg.simTime - 2.0;
    double   interval = pktSize * 8.0 / 128000.0; // 128Kbps → interval in sec

    NS_LOG_UNCOND("  Node IP assignments:");

    for (uint32_t i = 0; i < numFlows; ++i)
    {
        uint32_t    dstId   = cfg.numUavs - 1 - i;
        Ipv4Address srcAddr = ifaces.GetAddress(i);
        Ipv4Address dstAddr = ifaces.GetAddress(dstId);

        NS_LOG_UNCOND("  Flow " << i+1 << ": node " << i
            << " (" << srcAddr << ")"
            << " -> node " << dstId << " (" << dstAddr << ")");

        // ── Receiver socket ───────────────────────────────────────────────
        Ptr<Socket> recvSock = Socket::CreateSocket(
            nodes.Get(dstId), UdpSocketFactory::GetTypeId());
        recvSock->Bind(InetSocketAddress(Ipv4Address::GetAny(), port));
        recvSock->SetRecvCallback(MakeCallback(&DsrRxCallback));

        // ── Sender — UdpClient ────────────────────────────────────────────
        UdpClientHelper udpClient(dstAddr, port);
        udpClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
        udpClient.SetAttribute("Interval",
            TimeValue(Seconds(interval)));
        udpClient.SetAttribute("PacketSize", UintegerValue(pktSize));

        ApplicationContainer clientApp = udpClient.Install(nodes.Get(i));
        clientApp.Start(Seconds(appStart + i * 1.0));
        clientApp.Stop(Seconds(appStop));

        // Count Tx from OnOffHelper's sent-packet trace would need pointer;
        // estimate Tx from duration and rate instead (done after sim)
        g_dsrTxPkts += (uint32_t)((appStop - appStart - i) / interval);
    }

    AnimationInterface* anim = nullptr;
    if (cfg.animate) {
        anim = new AnimationInterface(cfg.outputPrefix + "-DSR-anim.xml");
        for (uint32_t i = 0; i < cfg.numUavs; ++i) {
            anim->UpdateNodeDescription(nodes.Get(i), "UAV-" + std::to_string(i));
            anim->UpdateNodeColor(nodes.Get(i), 255, 0, 0);
        }
    }

    Simulator::Stop(Seconds(cfg.simTime));
    Simulator::Run();

    // ── Compute DSR results from socket counters ───────────────────────────
    double dur   = (g_dsrFirstRx >= 0 && g_dsrLastRx > g_dsrFirstRx)
                   ? (g_dsrLastRx - g_dsrFirstRx) : 1.0;
    double tput  = g_dsrRxBytes * 8.0 / dur / 1e3;
    double pdr   = (g_dsrTxPkts > 0)
                   ? 100.0 * g_dsrRxPkts / g_dsrTxPkts : 0.0;

    SimResults res;
    res.protocol      = "DSR";
    res.avgThroughput = tput;
    res.avgDelay      = 0.0;   // not measurable via raw socket without timestamps
    res.avgJitter     = 0.0;
    res.avgPDR        = pdr;
    res.totalTxPkts   = g_dsrTxPkts;
    res.totalRxPkts   = g_dsrRxPkts;

    NS_LOG_UNCOND("\n─────────────────────────────────────────────────────");
    NS_LOG_UNCOND("  Protocol: DSR | Aggregate Stats");
    NS_LOG_UNCOND("─────────────────────────────────────────────────────");
    NS_LOG_UNCOND("  Tx Pkts  : " << g_dsrTxPkts);
    NS_LOG_UNCOND("  Rx Pkts  : " << g_dsrRxPkts);
    NS_LOG_UNCOND("  Rx Bytes : " << g_dsrRxBytes);
    NS_LOG_UNCOND("  Tput     : " << std::fixed << std::setprecision(2) << tput << " Kbps");
    NS_LOG_UNCOND("  PDR      : " << pdr << "%");
    NS_LOG_UNCOND("  Note     : Delay/Jitter not available for DSR (raw socket measurement)");

    Simulator::Destroy();
    delete anim;
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// Comparison Table
// ─────────────────────────────────────────────────────────────────────────────
void PrintComparison(const SimResults& bee,
                     const SimResults& aodv,
                     const SimResults& dsr)
{
    auto fmt = [](double v, int p=2) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(p) << v;
        return ss.str();
    };
    auto row = [](const std::string& m,
                  const std::string& v1,
                  const std::string& v2,
                  const std::string& v3) {
        NS_LOG_UNCOND("║ " << std::left << std::setw(18) << m
                      << " ║ " << std::setw(15) << v1
                      << " ║ " << std::setw(12) << v2
                      << " ║ " << std::setw(12) << v3 << " ║");
    };

    NS_LOG_UNCOND("\n╔══════════════════════════════════════════════════════════════════════╗");
    NS_LOG_UNCOND(  "║           FANET Routing Comparison: BeeAdHoc vs AODV vs DSR        ║");
    NS_LOG_UNCOND(  "╠════════════════════╦═════════════════╦══════════════╦══════════════╣");
    NS_LOG_UNCOND(  "║ Metric             ║ BeeAdHoc        ║ AODV         ║ DSR          ║");
    NS_LOG_UNCOND(  "╠════════════════════╬═════════════════╬══════════════╬══════════════╣");
    row("Throughput (Kbps)", fmt(bee.avgThroughput), fmt(aodv.avgThroughput), fmt(dsr.avgThroughput));
    row("Avg Delay (ms)",    fmt(bee.avgDelay),      fmt(aodv.avgDelay),      "N/A (DSR)");
    row("Avg Jitter (ms)",   fmt(bee.avgJitter),     fmt(aodv.avgJitter),     "N/A (DSR)");
    row("PDR (%)",           fmt(bee.avgPDR),        fmt(aodv.avgPDR),        fmt(dsr.avgPDR));
    row("Tx Packets",        std::to_string(bee.totalTxPkts), std::to_string(aodv.totalTxPkts), std::to_string(dsr.totalTxPkts));
    row("Rx Packets",        std::to_string(bee.totalRxPkts), std::to_string(aodv.totalRxPkts), std::to_string(dsr.totalRxPkts));
    NS_LOG_UNCOND(  "╚════════════════════╩═════════════════╩══════════════╩══════════════╝");

}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    SimConfig   cfg;
    std::string runMode = "all";

    CommandLine cmd;
    cmd.AddValue("numUavs",   "Number of UAV nodes",    cfg.numUavs);
    cmd.AddValue("simTime",   "Simulation time (s)",    cfg.simTime);
    cmd.AddValue("txPower",   "TX power (dBm)",         cfg.txPower);
    cmd.AddValue("areaSize",  "Area size (m)",          cfg.areaSize);
    cmd.AddValue("uavSpeed",  "Max UAV speed (m/s)",    cfg.uavSpeed);
    cmd.AddValue("verbose",   "Verbose logging",        cfg.verbose);
    cmd.AddValue("useEnergy", "Enable energy model",    cfg.useEnergy);
    cmd.AddValue("animate",   "Enable NetAnim output",  cfg.animate);
    cmd.AddValue("runMode",   "bee | aodv | dsr | both | all", runMode);
    cmd.AddValue("output",    "Output file prefix",     cfg.outputPrefix);
    cmd.Parse(argc, argv);

    if (cfg.verbose) {
        LogComponentEnable("FanetAodvDsr",        LOG_LEVEL_INFO);
        LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_WARN);
        LogComponentEnable("DsrRouting",          LOG_LEVEL_WARN);
    }

    RngSeedManager::SetSeed(42);
    RngSeedManager::SetRun(1);

    SimResults beeRes, aodvRes, dsrRes;

    if (runMode == "both") {
        runMode = "all";
    }

    if (runMode == "bee" || runMode == "all")
        beeRes = RunBee(cfg);

    if (runMode == "aodv" || runMode == "all")
        aodvRes = RunAodv(cfg);

    if (runMode == "dsr" || runMode == "all")
        dsrRes = RunDsr(cfg);

    if (runMode == "all")
        PrintComparison(beeRes, aodvRes, dsrRes);

    return 0;
}
