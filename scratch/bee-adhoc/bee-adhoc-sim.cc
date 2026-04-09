/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * BeeAdHoc vs AODV Comparison Simulation
 *
 * Runs BOTH routing protocols back-to-back in the same process,
 * using identical topology seeds and traffic patterns, then prints
 * a side-by-side comparison table.
 *
 * Network configuration:
 *   - 20 nodes
 *   - 800 x 800 m² area
 *   - Random Waypoint mobility, speed [1, 10] m/s, pause 5s
 *   - 802.11b ad hoc WiFi, 250 m tx range
 *   - CBR/UDP: every node sends to a peer (offset nNodes/2)
 *   - 4 packets/s, 64-byte packets
 *   - Initial energy: 100 J per node
 *   - Simulation time: 120s
 *
 * Metrics reported (per protocol):
 *   - Packet Delivery Ratio (PDR %)
 *   - Throughput (kbit/s)
 *   - End-to-end Delay: avg, 90th, 95th, 100th percentile (ms)
 *   - Energy consumed (J) and energy per user data (mJ/kB)
 *   - Remaining network energy (%)
 *   - Packets TX / RX
 *
 * Build: place alongside bee-adhoc.h / bee-adhoc.cc in the same
 *   scratch directory, then:
 *     ./ns3 run "bee-adhoc-sim"
 *   or with args:
 *     ./ns3 run "bee-adhoc-sim --simTime=180 --run=2"
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
#include "ns3/ipv4-routing-helper.h"

#include "bee-adhoc.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace ns3;
using namespace ns3::beeadhoc;

NS_LOG_COMPONENT_DEFINE("BeeAdHocCompareSim");

// ============================================================
// BeeAdHoc routing helper
// ============================================================
class BeeAdHocHelper : public Ipv4RoutingHelper {
public:
    BeeAdHocHelper* Copy() const override { return new BeeAdHocHelper(*this); }

    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override {
        Ptr<BeeAdHocRoutingProtocol> bee = CreateObject<BeeAdHocRoutingProtocol>();

        bee->SetEnergyThreshold(0.1);
        bee->SetSwarmThreshold(3);
        bee->SetInitialDanceNum(5);
        bee->SetDebugTracing(true);
        return bee;
    }
};

// ============================================================
// Results struct — one per protocol run
// ============================================================
struct SimResults {
    std::string protocol;

    // Packet stats
    uint64_t txPkts  {0};
    uint64_t rxPkts  {0};
    uint64_t txBytes {0};
    uint64_t rxBytes {0};

    // Derived metrics
    double pdr           {0.0};  // %
    double throughputKbps{0.0};
    double delayAvg      {0.0};  // ms
    double delay90       {0.0};  // ms
    double delay95       {0.0};  // ms
    double delay100      {0.0};  // ms (worst-case)
    double PacketDropRatio {0.0};  // %

    // Energy
    double initEnergyTotal    {0.0};  // J
    double residualEnergyTotal{0.0};  // J
    double consumedJ          {0.0};  // J
    double energyPerKB        {0.0};  // mJ/kB
    double remainingPct       {0.0};  // %

    // Per-flow detail lines (for printing)
    std::vector<std::string> flowLines;
};

// ============================================================
// RunSimulation — executes one full simulation for a given protocol
// ============================================================
SimResults RunSimulation(
    const std::string& protocol,
    uint32_t nNodes,
    double   areaX,
    double   areaY,
    double   txRange,
    double   simTime,
    double   pauseTime,
    double   minSpeed,
    double   maxSpeed,
    uint32_t pktRate,
    uint32_t pktSize,
    double   initEnergy,
    uint32_t seed,
    uint32_t run,
    const std::string& outFilePrefix)
{
    SimResults res;
    res.protocol = protocol;
    res.initEnergyTotal = initEnergy * nNodes;

    std::cout << "\n"
              << "╔══════════════════════════════════════════════╗\n"
              << "║  Running: " << std::left << std::setw(34) << protocol << "║\n"
              << "╚══════════════════════════════════════════════╝\n";

    // ---- RNG — same seed for fair comparison ----
    SeedManager::SetSeed(seed);
    SeedManager::SetRun(run);

    // ---- Nodes ----
    NodeContainer nodes;
    nodes.Create(nNodes);

    // ---- WiFi — 802.11b ad hoc ----
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode",    StringValue("DsssRate11Mbps"),
        "ControlMode", StringValue("DsssRate1Mbps"));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    YansWifiPhyHelper phy;
    phy.Set("TxPowerStart",  DoubleValue(20.0));
    phy.Set("TxPowerEnd",    DoubleValue(20.0));
    phy.Set("RxSensitivity", DoubleValue(-85.0));

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::RangePropagationLossModel",
                               "MaxRange", DoubleValue(txRange));
    phy.SetChannel(channel.Create());

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // ---- Energy model ----
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(initEnergy));
    energy::EnergySourceContainer energySources = energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioHelper;
    radioHelper.Set("TxCurrentA",   DoubleValue(0.0174));
    radioHelper.Set("RxCurrentA",   DoubleValue(0.0174));
    radioHelper.Set("IdleCurrentA", DoubleValue(0.001));
    radioHelper.Install(devices, energySources);

    // ---- Mobility: Random Waypoint ----
    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max="
                         + std::to_string((int)areaX) + "]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max="
                         + std::to_string((int)areaY) + "]"));
    mobility.SetMobilityModel(
        "ns3::RandomWaypointMobilityModel",
        "Speed",  StringValue("ns3::UniformRandomVariable[Min="
                              + std::to_string((int)minSpeed) + "|Max="
                              + std::to_string((int)maxSpeed) + "]"),
        "Pause",  StringValue("ns3::ConstantRandomVariable[Constant="
                              + std::to_string((int)pauseTime) + "]"),
        "PositionAllocator",
        StringValue("ns3::RandomRectanglePositionAllocator"));
    mobility.Install(nodes);

    // ---- Internet stack + routing protocol ----
    InternetStackHelper internet;
    if (protocol == "aodv") {
        AodvHelper aodv;
        internet.SetRoutingHelper(aodv);
        internet.Install(nodes);
    }
    else {   // beeadhoc
        BeeAdHocHelper beeHelper;
        internet.SetRoutingHelper(beeHelper);
        internet.Install(nodes);
    }

    // ---- IP addressing ----
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // ---- Traffic: CBR/UDP, every node → peer (offset nNodes/2) ----
    uint16_t basePort = 9000;
    double   appStart = 15.0;   // give routing time to discover routes
    double   appStop  = simTime - 5.0;

    ApplicationContainer sinkApps, sourceApps;

    for (uint32_t i = 0; i < nNodes; i++) {
        uint32_t dst = (i + nNodes / 2) % nNodes;
        if (dst == i) dst = (dst + 1) % nNodes;

        // Sink
        PacketSinkHelper sink("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), basePort + i));
        sinkApps.Add(sink.Install(nodes.Get(dst)));

        // Source: constant-rate CBR
        OnOffHelper src("ns3::UdpSocketFactory",
            InetSocketAddress(interfaces.GetAddress(dst), basePort + i));
        uint64_t bps = (uint64_t)pktSize * 8 * pktRate;
        src.SetConstantRate(DataRate(bps));
        src.SetAttribute("PacketSize", UintegerValue(pktSize));
        src.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        src.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        sourceApps.Add(src.Install(nodes.Get(i)));
    }

    sinkApps.Start(Seconds(appStart - 1.0));
    sinkApps.Stop(Seconds(appStop   + 2.0));
    sourceApps.Start(Seconds(appStart));
    sourceApps.Stop(Seconds(appStop));

    // ---- Route pre-warm probes (BeeAdHoc especially benefits) ----
    for (uint32_t i = 0; i < nNodes; i++) {
        uint32_t dst = (i + nNodes / 2) % nNodes;
        if (dst == i) dst = (dst + 1) % nNodes;

        OnOffHelper probe("ns3::UdpSocketFactory",
            InetSocketAddress(interfaces.GetAddress(dst), basePort + i));
        probe.SetAttribute("PacketSize", UintegerValue(pktSize));
        probe.SetConstantRate(DataRate("1bps"));
        probe.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        probe.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=1000]"));
        ApplicationContainer probeApp = probe.Install(nodes.Get(i));
        probeApp.Start(Seconds(1.0));
        probeApp.Stop(Seconds(appStart - 0.5));
    }

    // ---- Flow Monitor ----
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // ---- Run ----
    std::cout << "  Simulating " << simTime << "s ...\n";
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ---- Collect results ----
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    auto& stats = monitor->GetFlowStats();

    std::vector<double> allDelays;
    double totalDelaySum = 0.0;
    uint64_t totalDelayPkts = 0;

    for (auto& [id, fs] : stats) {
        res.txPkts  += fs.txPackets;
        res.rxPkts  += fs.rxPackets;
        res.txBytes += fs.txBytes;
        res.rxBytes += fs.rxBytes;

        if (fs.rxPackets > 0) {
            double avgDelay = fs.delaySum.GetSeconds() / fs.rxPackets * 1000.0;
            allDelays.push_back(avgDelay);
            totalDelaySum  += fs.delaySum.GetSeconds() * 1000.0;
            totalDelayPkts += fs.rxPackets;
        }

        // Build per-flow line for printing later
        auto ft = classifier->FindFlow(id);
        double fpdr   = fs.txPackets > 0 ?
            (double)fs.rxPackets / fs.txPackets * 100.0 : 0.0;
        double fdelay = fs.rxPackets > 0 ?
            fs.delaySum.GetSeconds() / fs.rxPackets * 1000.0 : 0.0;

        std::ostringstream line;
        line << std::fixed << std::setprecision(1)
             << "  Flow " << std::setw(3) << id
             << "  " << ft.sourceAddress
             << " -> " << ft.destinationAddress
             << "  TX=" << std::setw(5) << fs.txPackets
             << "  RX=" << std::setw(5) << fs.rxPackets
             << "  PDR=" << std::setw(5) << fpdr << "%"
             << "  Delay=" << std::setw(7) << fdelay << "ms";
        res.flowLines.push_back(line.str());
    }

    // Derived metrics
    res.pdr = res.txPkts > 0 ?
        (double)res.rxPkts / res.txPkts * 100.0 : 0.0;
    
    res.PacketDropRatio = res.txPkts > 0 ?
        (double)(res.txPkts - res.rxPkts) / res.txPkts * 100.0 : 0.0;

    double trafficDuration = appStop - appStart;
    res.throughputKbps = res.rxBytes * 8.0 / trafficDuration / 1000.0;

    res.delayAvg = totalDelayPkts > 0 ?
        totalDelaySum / totalDelayPkts : 0.0;

    std::sort(allDelays.begin(), allDelays.end());
    auto pctIdx = [&](double pct) -> double {
        if (allDelays.empty()) return 0.0;
        size_t idx = (size_t)(allDelays.size() * pct);
        if (idx >= allDelays.size()) idx = allDelays.size() - 1;
        return allDelays[idx];
    };
    res.delay90  = pctIdx(0.90);
    res.delay95  = pctIdx(0.95);
    res.delay100 = allDelays.empty() ? 0.0 : allDelays.back();

    // Energy
    res.residualEnergyTotal = 0.0;
    for (uint32_t i = 0; i < energySources.GetN(); i++) {
        res.residualEnergyTotal += energySources.Get(i)->GetRemainingEnergy();
    }
    res.consumedJ     = res.initEnergyTotal - res.residualEnergyTotal;
    double rxKB       = res.rxBytes / 1000.0;
    res.energyPerKB   = rxKB > 0 ? (res.consumedJ * 1000.0) / rxKB : 0.0;
    res.remainingPct  = (res.residualEnergyTotal / res.initEnergyTotal) * 100.0;

    // Save per-run XML
    std::string xmlName = outFilePrefix + "-" + protocol
                        + "-run" + std::to_string(run) + ".xml";
    monitor->SerializeToXmlFile(xmlName, true, true);
    std::cout << "  Flow data saved to: " << xmlName << "\n";

    Simulator::Destroy();
    return res;
}

// ============================================================
// Print a single protocol's results
// ============================================================
static void PrintResults(const SimResults& r)
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout
        << "  Packet Delivery Ratio : " << r.pdr            << " %\n"
        << "  Packet Drop Ratio     : " << r.PacketDropRatio << " %\n"
        << "  Throughput            : " << r.throughputKbps << " kbit/s\n"
        << "  Delay (avg)           : " << r.delayAvg       << " ms\n"
        << "  Energy consumed       : " << r.consumedJ
                                        << " J / " << r.initEnergyTotal << " J\n"
        << "  Energy per user data  : " << r.energyPerKB    << " mJ/kB\n"
        << "  Remaining energy      : " << r.remainingPct   << " %\n"
        << "  TX packets            : " << r.txPkts         << "\n"
        << "  RX packets            : " << r.rxPkts         << "\n";
}

// ============================================================
// Print side-by-side comparison table
// ============================================================
static void PrintComparison(const SimResults& bee, const SimResults& aodv)
{
    // Helper: formats a "BeeAdHoc better / AODV better / tie" verdict
    // better = higher for PDR/throughput/remaining; lower for delay/energy
    auto verdict = [](double beeVal, double aodvVal, bool higherIsBetter) -> std::string {
        double diff = beeVal - aodvVal;
        double rel  = (aodvVal != 0.0) ? std::abs(diff / aodvVal) * 100.0 : 0.0;
        if (std::abs(diff) < 0.01) return "  TIE";
        if (higherIsBetter)
            return diff > 0 ? ("  BeeAdHoc +" + std::to_string((int)rel) + "%")
                            : ("  AODV     +" + std::to_string((int)rel) + "%");
        else
            return diff < 0 ? ("  BeeAdHoc +" + std::to_string((int)rel) + "%")
                            : ("  AODV     +" + std::to_string((int)rel) + "%");
    };

    const int W = 14;  // column width for values
    std::cout << "\n"
        << "╔══════════════════════════════════════════════════════════════════════╗\n"
        << "║            BEEADHOC  vs  AODV  —  COMPARISON SUMMARY               ║\n"
        << "╠══════════════════════════════════════════════════════════════════════╣\n"
        << "║  Metric                     BeeAdHoc        AODV        Winner      ║\n"
        << "╠══════════════════════════════════════════════════════════════════════╣\n";

    auto row = [&](const std::string& label, double bv, double av,
                   const std::string& unit, bool higherBetter) {
        std::string v = verdict(bv, av, higherBetter);
        std::cout << "║  " << std::left << std::setw(26) << label
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(W) << bv
                  << std::setw(W) << av
                  << "  " << std::left << std::setw(18) << (v + " " + unit)
                  << "║\n";
    };

    row("PDR (%)",              bee.pdr,            aodv.pdr,            "%",       true);
    row("Packet Drop Ratio (%)", bee.PacketDropRatio, aodv.PacketDropRatio, "%",       false);
    row("Throughput (kbit/s)",  bee.throughputKbps,  aodv.throughputKbps, "kbit/s",  true);
    row("Delay avg (ms)",       bee.delayAvg,        aodv.delayAvg,       "ms",      false);
    row("Delay 90th pct (ms)",  bee.delay90,         aodv.delay90,        "ms",      false);
    row("Delay 95th pct (ms)",  bee.delay95,         aodv.delay95,        "ms",      false);
    row("Delay 100th pct (ms)", bee.delay100,        aodv.delay100,       "ms",      false);
    row("Energy consumed (J)",  bee.consumedJ,       aodv.consumedJ,      "J",       false);
    row("Energy/data (mJ/kB)",  bee.energyPerKB,     aodv.energyPerKB,    "mJ/kB",   false);
    row("Remaining energy (%)", bee.remainingPct,    aodv.remainingPct,   "%",       true);
    row("TX packets",           (double)bee.txPkts,  (double)aodv.txPkts, "pkts",    false);
    row("RX packets",           (double)bee.rxPkts,  (double)aodv.rxPkts, "pkts",    true);

    std::cout
        << "╚══════════════════════════════════════════════════════════════════════╝\n";

    // Count wins
    int beeWins = 0, aodvWins = 0;
    auto countWin = [&](double bv, double av, bool higherBetter) {
        double diff = bv - av;
        if (std::abs(diff) < 0.01) return;
        bool beeWon = higherBetter ? (diff > 0) : (diff < 0);
        if (beeWon) beeWins++; else aodvWins++;
    };
    countWin(bee.pdr,            aodv.pdr,            true);
    countWin(bee.throughputKbps, aodv.throughputKbps, true);
    countWin(bee.delayAvg,       aodv.delayAvg,       false);
    countWin(bee.delay90,        aodv.delay90,        false);
    countWin(bee.delay95,        aodv.delay95,        false);
    countWin(bee.delay100,       aodv.delay100,       false);
    countWin(bee.consumedJ,      aodv.consumedJ,      false);
    countWin(bee.energyPerKB,    aodv.energyPerKB,    false);
    countWin(bee.remainingPct,   aodv.remainingPct,   true);
    countWin((double)bee.rxPkts, (double)aodv.rxPkts, true);
    countWin(bee.PacketDropRatio, aodv.PacketDropRatio, false);

    std::cout << "\n  Overall: BeeAdHoc wins " << beeWins
              << " metrics,  AODV wins " << aodvWins
              << " metrics  (out of 10 scored)\n";
}

// ============================================================
// Save comparison to CSV
// ============================================================
static void SaveCSV(const SimResults& bee, const SimResults& aodv,
                    const std::string& prefix, uint32_t run)
{
    std::string fname = prefix + "-comparison-run" + std::to_string(run) + ".csv";
    std::ofstream f(fname);
    f << "metric,beeadhoc,aodv\n"
      << "pdr_pct,"           << bee.pdr            << "," << aodv.pdr            << "\n"
      << "packet_drop_ratio," << bee.PacketDropRatio << "," << aodv.PacketDropRatio << "\n"
      << "throughput_kbps,"   << bee.throughputKbps  << "," << aodv.throughputKbps << "\n"
      << "delay_avg_ms,"      << bee.delayAvg        << "," << aodv.delayAvg       << "\n"
      << "energy_consumed_J," << bee.consumedJ       << "," << aodv.consumedJ      << "\n"
      << "energy_per_kB_mJkB,"<< bee.energyPerKB    << "," << aodv.energyPerKB    << "\n"
      << "remaining_energy_pct,"<< bee.remainingPct  << "," << aodv.remainingPct   << "\n"
      << "tx_packets,"        << bee.txPkts          << "," << aodv.txPkts         << "\n"
      << "rx_packets,"        << bee.rxPkts          << "," << aodv.rxPkts         << "\n"
      << "tx_bytes,"          << bee.txBytes         << "," << aodv.txBytes        << "\n"
      << "rx_bytes,"          << bee.rxBytes         << "," << aodv.rxBytes        << "\n";
    f.close();
    std::cout << "  CSV saved to: " << fname << "\n";
}

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[])
{
    // ---- Simulation parameters ----
    uint32_t nNodes     = 10;
    double   areaX      = 800.0;
    double   areaY      = 800.0;
    double   txRange    = 250.0;
    double   simTime    = 120.0;
    double   pauseTime  = 5.0;
    double   minSpeed   = 1.0;
    double   maxSpeed   = 5.0;
    uint32_t pktRate    = 4;      // packets/s
    uint32_t pktSize    = 64;     // bytes
    double   initEnergy = 100.0;  // J per node
    uint32_t seed       = 12345;
    uint32_t run        = 1;
    std::string outFile = "compare";
    bool     runBee     = true;
    bool     runAodv    = true;

    CommandLine cmd;
    cmd.AddValue("nodes",     "Number of nodes",             nNodes);
    cmd.AddValue("simTime",   "Simulation time (s)",          simTime);
    cmd.AddValue("pauseTime", "Mobility pause time (s)",      pauseTime);
    cmd.AddValue("maxSpeed",  "Max node speed (m/s)",         maxSpeed);
    cmd.AddValue("pktRate",   "CBR packets per second",       pktRate);
    cmd.AddValue("pktSize",   "Packet size (bytes)",          pktSize);
    cmd.AddValue("energy",    "Initial energy per node (J)",  initEnergy);
    cmd.AddValue("seed",      "RNG seed",                     seed);
    cmd.AddValue("run",       "RNG run number",               run);
    cmd.AddValue("out",       "Output filename prefix",        outFile);
    cmd.AddValue("bee",       "Run BeeAdHoc (1/0)",           runBee);
    cmd.AddValue("aodv",      "Run AODV (1/0)",               runAodv);
    cmd.Parse(argc, argv);

    std::cout << "\n"
        << "╔══════════════════════════════════════════════════════════════════════╗\n"
        << "║           BeeAdHoc vs AODV — Comparison Simulation                 ║\n"
        << "╠══════════════════════════════════════════════════════════════════════╣\n"
        << "║  Nodes      : " << std::left << std::setw(55) << nNodes     << "║\n"
        << "║  Area       : " << std::left << std::setw(55)
                              << (std::to_string((int)areaX) + " x " + std::to_string((int)areaY) + " m") << "║\n"
        << "║  SimTime    : " << std::left << std::setw(55) << (std::to_string((int)simTime) + " s") << "║\n"
        << "║  PauseTime  : " << std::left << std::setw(55) << (std::to_string((int)pauseTime) + " s") << "║\n"
        << "║  Speed      : " << std::left << std::setw(55)
                              << (std::to_string((int)minSpeed) + " - " + std::to_string((int)maxSpeed) + " m/s") << "║\n"
        << "║  PktRate    : " << std::left << std::setw(55) << (std::to_string(pktRate) + " pkt/s") << "║\n"
        << "║  PktSize    : " << std::left << std::setw(55) << (std::to_string(pktSize) + " bytes") << "║\n"
        << "║  InitEnergy : " << std::left << std::setw(55) << (std::to_string((int)initEnergy) + " J/node") << "║\n"
        << "║  Seed/Run   : " << std::left << std::setw(55) << (std::to_string(seed) + " / " + std::to_string(run)) << "║\n"
        << "╚══════════════════════════════════════════════════════════════════════╝\n";

    // ---- Run BeeAdHoc ----
    SimResults beeResults, aodvResults;

    if (runBee) {
        beeResults = RunSimulation(
            "beeadhoc", nNodes, areaX, areaY, txRange,
            simTime, pauseTime, minSpeed, maxSpeed,
            pktRate, pktSize, initEnergy, seed, run, outFile);

        std::cout << "\n── BeeAdHoc Results ──────────────────────────────\n";
        PrintResults(beeResults);

        std::cout << "\n  Per-flow details (BeeAdHoc):\n";
        for (const auto& line : beeResults.flowLines)
            std::cout << line << "\n";
    }

    // ---- Run AODV ----
    if (runAodv) {
        aodvResults = RunSimulation(
            "aodv", nNodes, areaX, areaY, txRange,
            simTime, pauseTime, minSpeed, maxSpeed,
            pktRate, pktSize, initEnergy, seed, run, outFile);

        std::cout << "\n── AODV Results ──────────────────────────────────\n";
        PrintResults(aodvResults);

        std::cout << "\n  Per-flow details (AODV):\n";
        for (const auto& line : aodvResults.flowLines)
            std::cout << line << "\n";
    }

    // ---- Comparison table ----
    if (runBee && runAodv) {
        PrintComparison(beeResults, aodvResults);
        SaveCSV(beeResults, aodvResults, outFile, run);
    }

    std::cout << "\nDone.\n\n";
    return 0;
}
