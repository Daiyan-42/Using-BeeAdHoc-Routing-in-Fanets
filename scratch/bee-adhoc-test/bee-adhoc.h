/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * BeeAdHoc NS-3 3.46 Implementation
 * Faithful to: Wedde, Farooq et al., GECCO 2005
 *
 * Paper Architecture (Section 3):
 * Each node has a HIVE with three components:
 *   1. Packing Floor  — interface to transport layer (TCP/UDP)
 *   2. Entrance       — interface to MAC layer, handles all in/out packets
 *   3. Dance Floor    — heart of routing; stores foragers, makes route decisions
 *
 * Four Agent Types (Section 2):
 *   1. Packers   — created when data arrives, find forager or launch scout
 *   2. Scouts    — discover routes (broadcast forward, unicast backward)
 *   3. Foragers  — carry data; two types: delay and lifetime
 *                  lifetime forager quality = avg remaining battery on path
 *   4. Swarms    — bundles of returning foragers for UDP (no ACK return path)
 *
 * Key differences from naive DSR-like implementation:
 *   - Destination sends back ALL received scouts (not just best)
 *   - Source picks route based on DANCE NUMBER (forager quality * waiting packers)
 *   - Forager quality = average residual energy (not minimum)
 *   - Foragers return to source (piggyback on ACK or via swarm)
 *   - Dance number controls how many data packets use a route (cloning)
 *   - No explicit HELLO/RERR — route validity monitored via forager count
 */

#ifndef BEE_ADHOC_H
#define BEE_ADHOC_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/energy-source-container.h"
#include "ns3/device-energy-model-container.h"
#include "ns3/basic-energy-source.h"
#include "ns3/address-utils.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"

#include <map>
#include <vector>
#include <list>
#include <deque>
#include <string>

namespace ns3 {
namespace beeadhoc {

// ============================================================
// Control Packet Type Byte (first byte of every UDP payload)
// ============================================================
enum BeePacketType : uint8_t {
    PKT_FORWARD_SCOUT  = 1,  // Src -> Dst  (broadcast, accumulates path)
    PKT_BACKWARD_SCOUT = 2,  // Dst -> Src  (unicast, reversed path)
    PKT_FORAGER        = 3,  // Data carrier (source-routed)
    PKT_SWARM          = 4,  // Bundle of returning foragers (UDP case)
};

// ============================================================
// Forager Type (Section 2.3 of paper)
// ============================================================
enum ForagerType : uint8_t {
    FORAGER_DELAY    = 0,  // optimises for minimum delay
    FORAGER_LIFETIME = 1,  // optimises for network lifetime (battery)
};

// ============================================================
// Scout ID — unique per route discovery instance
// ============================================================
struct ScoutId {
    Ipv4Address src;
    uint32_t    seqno;
    bool operator< (const ScoutId& o) const {
        if (src < o.src) return true;
        if (src == o.src) return seqno < o.seqno;
        return false;
    }
    bool operator==(const ScoutId& o) const {
        return src == o.src && seqno == o.seqno;
    }
};

// ============================================================
// Forager Entry — stored on the Dance Floor
// Paper Section 3.3: dance floor stores foragers with dance
// number and age/lifetime parameters.
// ============================================================
struct ForagerEntry {
    Ipv4Address              dst;
    std::vector<Ipv4Address> route;      // complete source route src->...->dst
    ForagerType              type;
    double                   quality;    // avg residual energy for LIFETIME type
    uint32_t                 danceNum;   // how many more data packets can use this
    Time                     createdAt;  // for age-based expiry
    Time                     lifetime;   // max age before expiry

    bool IsExpired() const {
        return (Simulator::Now() - createdAt) > lifetime;
    }
};

// ============================================================
// Packer — created when data arrives from transport layer
// Paper Section 2.1: packers find a forager or launch a scout
// ============================================================
struct PackerEntry {
    Ptr<Packet>                                  packet;
    Ipv4Header                                   ipHdr;
    Ipv4RoutingProtocol::UnicastForwardCallback  ucb;
    Ipv4RoutingProtocol::ErrorCallback           ecb;
    Time                                         createdAt;
    Time                                         waitTimeout;  // before launching scout
};

// ============================================================
// Forward Scout Header
// Accumulates: route[], avgEnergy, hopCount, TTL
// Paper: scout identified by (id, sourceNode) pair
// ============================================================
class ForwardScoutHeader : public Header {
public:
    ForwardScoutHeader();
    ~ForwardScoutHeader() override;

    static TypeId GetTypeId();
    TypeId   GetInstanceTypeId() const override;
    void     Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void     Serialize(Buffer::Iterator i) const override;
    uint32_t Deserialize(Buffer::Iterator i) override;

    // Setters
    void SetSrc(Ipv4Address a)     { m_src = a; }
    void SetDst(Ipv4Address a)     { m_dst = a; }
    void SetSeqno(uint32_t s)      { m_seqno = s; }
    void SetTtl(uint8_t t)         { m_ttl = t; }
    void SetTotalEnergy(double e)  { m_totalEnergy = e; }
    void SetHopCount(uint8_t h)    { m_hopCount = h; }
    void SetType(ForagerType t)    { m_type = t; }

    // Getters
    Ipv4Address GetSrc()        const { return m_src; }
    Ipv4Address GetDst()        const { return m_dst; }
    uint32_t    GetSeqno()      const { return m_seqno; }
    uint8_t     GetTtl()        const { return m_ttl; }
    double      GetTotalEnergy()const { return m_totalEnergy; }
    uint8_t     GetHopCount()   const { return m_hopCount; }
    ForagerType GetType()       const { return m_type; }

    // Route accumulation
    void AddHop(Ipv4Address addr);
    void DecrementTtl() { if (m_ttl > 0) m_ttl--; }
    bool TtlExpired()   const { return m_ttl == 0; }

    void                     SetRoute(const std::vector<Ipv4Address>& r) { m_route = r; }
    std::vector<Ipv4Address> GetRoute() const { return m_route; }

    // Computed quality: average energy per hop
    double GetAvgEnergy() const {
        return m_hopCount > 0 ? m_totalEnergy / m_hopCount : 0.0;
    }

private:
    Ipv4Address              m_src;
    Ipv4Address              m_dst;
    uint32_t                 m_seqno       {0};
    uint8_t                  m_ttl         {20};
    double                   m_totalEnergy {0.0};  // sum of energy at each hop
    uint8_t                  m_hopCount    {0};
    ForagerType              m_type        {FORAGER_LIFETIME};
    std::vector<Ipv4Address> m_route;
};

// ============================================================
// Backward Scout Header
// Paper: destination sends back ALL received scouts.
// Carries the complete route and quality back to source.
// ============================================================
class BackwardScoutHeader : public Header {
public:
    BackwardScoutHeader();
    ~BackwardScoutHeader() override;

    static TypeId GetTypeId();
    TypeId   GetInstanceTypeId() const override;
    void     Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void     Serialize(Buffer::Iterator i) const override;
    uint32_t Deserialize(Buffer::Iterator i) override;

    void SetSrc(Ipv4Address a)     { m_src = a; }
    void SetDst(Ipv4Address a)     { m_dst = a; }
    void SetSeqno(uint32_t s)      { m_seqno = s; }
    void SetAvgEnergy(double e)    { m_avgEnergy = e; }
    void SetHopCount(uint8_t h)    { m_hopCount = h; }
    void SetRouteIndex(uint8_t i)  { m_routeIndex = i; }
    void SetType(ForagerType t)    { m_type = t; }

    Ipv4Address GetSrc()        const { return m_src; }
    Ipv4Address GetDst()        const { return m_dst; }
    uint32_t    GetSeqno()      const { return m_seqno; }
    double      GetAvgEnergy()  const { return m_avgEnergy; }
    uint8_t     GetHopCount()   const { return m_hopCount; }
    uint8_t     GetRouteIndex() const { return m_routeIndex; }
    ForagerType GetType()       const { return m_type; }

    void                     SetRoute(const std::vector<Ipv4Address>& r) { m_route = r; }
    std::vector<Ipv4Address> GetRoute() const { return m_route; }

private:
    Ipv4Address              m_src;
    Ipv4Address              m_dst;
    uint32_t                 m_seqno      {0};
    double                   m_avgEnergy  {0.0};
    uint8_t                  m_hopCount   {0};
    uint8_t                  m_routeIndex {0};
    ForagerType              m_type       {FORAGER_LIFETIME};
    std::vector<Ipv4Address> m_route;
};

// ============================================================
// Forager (Data) Header — source-routed data packet
// Paper Section 2.3: forager follows point-to-point mode
// and collects network state info depending on its type.
// ============================================================
class ForagerHeader : public Header {
public:
    ForagerHeader();
    ~ForagerHeader() override;

    static TypeId GetTypeId();
    TypeId   GetInstanceTypeId() const override;
    void     Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void     Serialize(Buffer::Iterator i) const override;
    uint32_t Deserialize(Buffer::Iterator i) override;

    void SetSrc(Ipv4Address a)     { m_src = a; }
    void SetDst(Ipv4Address a)     { m_dst = a; }
    void SetRouteIndex(uint8_t i)  { m_routeIndex = i; }
    void SetType(ForagerType t)    { m_type = t; }
    void SetAccumEnergy(double e)  { m_accumEnergy = e; }
    void SetHopCount(uint8_t h)    { m_hopCount = h; }

    Ipv4Address GetSrc()          const { return m_src; }
    Ipv4Address GetDst()          const { return m_dst; }
    uint8_t     GetRouteIndex()   const { return m_routeIndex; }
    ForagerType GetType()         const { return m_type; }
    double      GetAccumEnergy()  const { return m_accumEnergy; }
    uint8_t     GetHopCount()     const { return m_hopCount; }

    void                     SetRoute(const std::vector<Ipv4Address>& r) { m_route = r; }
    std::vector<Ipv4Address> GetRoute() const { return m_route; }

    void AdvanceHop() { m_routeIndex++; }
    bool AtDestination() const {
        return !m_route.empty() && m_routeIndex >= (uint8_t)m_route.size();
    }

private:
    Ipv4Address              m_src;
    Ipv4Address              m_dst;
    uint8_t                  m_routeIndex  {1};  // index of NEXT hop (0=src)
    ForagerType              m_type        {FORAGER_LIFETIME};
    double                   m_accumEnergy {0.0}; // collected along path
    uint8_t                  m_hopCount    {0};
    std::vector<Ipv4Address> m_route;
};

// ============================================================
// Swarm Header — Section 2.4
// Used for UDP: bundles returning foragers when no ACK path
// exists. Carries forager info in payload.
// ============================================================
class SwarmHeader : public Header {
public:
    SwarmHeader();
    ~SwarmHeader() override;

    static TypeId GetTypeId();
    TypeId   GetInstanceTypeId() const override;
    void     Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void     Serialize(Buffer::Iterator i) const override;
    uint32_t Deserialize(Buffer::Iterator i) override;

    void SetSrc(Ipv4Address a)      { m_src = a; }
    void SetDst(Ipv4Address a)      { m_dst = a; }
    void SetForagerCount(uint8_t n) { m_foragerCount = n; }
    void SetAvgEnergy(double e)     { m_avgEnergy = e; }
    void SetHopCount(uint8_t h)     { m_hopCount = h; }
    void SetRouteIndex(uint8_t i)   { m_routeIndex = i; }

    Ipv4Address GetSrc()          const { return m_src; }
    Ipv4Address GetDst()          const { return m_dst; }
    uint8_t     GetForagerCount() const { return m_foragerCount; }
    double      GetAvgEnergy()    const { return m_avgEnergy; }
    uint8_t     GetHopCount()     const { return m_hopCount; }
    uint8_t     GetRouteIndex()   const { return m_routeIndex; }

    void                     SetRoute(const std::vector<Ipv4Address>& r) { m_route = r; }
    std::vector<Ipv4Address> GetRoute() const { return m_route; }

private:
    Ipv4Address              m_src;
    Ipv4Address              m_dst;
    uint8_t                  m_foragerCount {1};
    double                   m_avgEnergy    {0.0};
    uint8_t                  m_hopCount     {0};
    uint8_t                  m_routeIndex   {0};
    std::vector<Ipv4Address> m_route;
};

// ============================================================
// Dance Floor — stores foragers, makes routing decisions
// Paper Section 3.3: heart of the hive
// ============================================================
class DanceFloor {
public:
    DanceFloor();

    // Add a forager from a returning scout/swarm
    void AddForager(const ForagerEntry& f);

    // Get best forager for destination (stochastic among valid ones)
    // Returns true and decrements dance number if found.
    // Paper: young foragers favoured; stochastic selection distributes load.
    bool GetForager(Ipv4Address dst, ForagerEntry& chosen);

    // Called when forager returns from destination with updated quality
    void UpdateForager(Ipv4Address dst, const std::vector<Ipv4Address>& route,
                       double newQuality);

    // Check if we have ANY valid forager for dst
    bool HasForager(Ipv4Address dst) const;

    // Count foragers for dst (for swarm threshold check)
    uint32_t CountForagers(Ipv4Address dst) const;

    // Count outgoing dance-units via first-hop node via (sum of danceNums)
    uint32_t CountOutgoing(Ipv4Address via) const;

    // Purge expired foragers
    void Purge();

    void SetForagerLifetime(Time t) { m_foragerLifetime = t; }
    void SetInitialDanceNum(uint32_t n) { m_initialDanceNum = n; }

private:
    // dst -> list of foragers
    std::map<Ipv4Address, std::list<ForagerEntry>> m_foragers;
    Time     m_foragerLifetime  {Seconds(30)};
    uint32_t m_initialDanceNum  {5};
    // Persistent RNG — created once in the constructor, not on every GetForager call.
    Ptr<UniformRandomVariable>  m_rng;
};

// ============================================================
// BeeAdHoc Routing Protocol — main class
// ============================================================
class BeeAdHocRoutingProtocol : public Ipv4RoutingProtocol {
public:
    static TypeId GetTypeId();
    BeeAdHocRoutingProtocol();
    ~BeeAdHocRoutingProtocol() override;

    // ---- Ipv4RoutingProtocol interface ----
    Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p,
                               const Ipv4Header& header,
                               Ptr<NetDevice> oif,
                               Socket::SocketErrno& sockerr) override;

    bool RouteInput(Ptr<const Packet> p,
                    const Ipv4Header& header,
                    Ptr<const NetDevice> idev,
                    const UnicastForwardCallback&   ucb,
                    const MulticastForwardCallback& mcb,
                    const LocalDeliverCallback&     lcb,
                    const ErrorCallback&            ecb) override;

    void NotifyInterfaceUp(uint32_t i) override;
    void NotifyInterfaceDown(uint32_t i) override;
    void NotifyAddAddress(uint32_t i, Ipv4InterfaceAddress address) override;
    void NotifyRemoveAddress(uint32_t i, Ipv4InterfaceAddress address) override;
    void SetIpv4(Ptr<Ipv4> ipv4) override;
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,
                           Time::Unit unit = Time::S) const override;

    // Configuration
    void SetInitialTtl(uint8_t t)       { m_initialTtl = t; }
    void SetPackerTimeout(Time t)       { m_packerTimeout = t; }
    void SetForagerLifetime(Time t)     { m_danceFloor.SetForagerLifetime(t); }
    void SetInitialDanceNum(uint32_t n) { m_danceFloor.SetInitialDanceNum(n); }
    void SetEnergyThreshold(double e)   { m_energyThreshold = e; }
    void SetSwarmThreshold(uint32_t n)  { m_swarmThreshold = n; }
    void SetDebugTracing(bool enabled)  { m_debugTracing = enabled; }

private:
    // ---- Startup ----
    void Start();

    // ---- Utilities ----
    uint32_t    GetNodeId() const;
    void        DebugCheckpoint(const std::string& stage,
                                const std::string& detail) const;
    Ipv4Address GetLocalAddress() const;
    bool        IsMyAddress(Ipv4Address addr) const;
    double      GetResidualEnergy() const;
    Ptr<Ipv4Route> BuildRoute(Ipv4Address dst, Ipv4Address nextHop) const;

    // ---- Packing Floor (Section 3.1) ----
    // Called when data arrives from transport layer
    void PackingFloorReceive(Ptr<const Packet> p, const Ipv4Header& hdr,
                             const UnicastForwardCallback& ucb,
                             const ErrorCallback& ecb);
    void CheckPackerQueue();         // periodic: retry packers waiting for forager
    void DrainPackerQueue(Ipv4Address dst); // forager arrived, drain queue

    // ---- Entrance (Section 3.2) ----
    void RecvEntrance(Ptr<Socket> socket);   // all incoming packets land here

    // ---- Scout handling ----
    void LaunchForwardScout(Ipv4Address dst, ForagerType type);
    void ProcessForwardScout(Ptr<Packet> p, ForwardScoutHeader& fsh);

    // Destination: send back ALL received scouts (paper requirement)
    void SendBackwardScout(const ForwardScoutHeader& fsh);
    void ProcessBackwardScout(Ptr<Packet> p, BackwardScoutHeader& bsh);

    // ---- Forager handling ----
    // SendForager sends a lightweight energy-probe packet (ForagerHeader only,
    // no data payload).  Actual data travels via the NS-3 routing stack.
    // The probe collects residual-energy at each hop and triggers a swarm
    // feedback at the destination so the source can update route quality.
    void SendForager(const ForagerEntry& fe);

    // ProcessForager handles the arriving energy probe.  It accumulates
    // energy, forwards the probe at intermediate nodes, and at the
    // destination computes avgEnergy and sends a swarm back to the source.
    // No lcb/ecb needed — data delivery is handled by the routing stack.
    void ProcessForager(Ptr<Packet> p, ForagerHeader& fh);

    // ---- Swarm handling (Section 2.4) ----
    void CheckSwarmBalance();        // periodic: check forager imbalance
    void SendSwarm(Ipv4Address to, uint32_t count, double quality,
                   const std::vector<Ipv4Address>& route);
    void ProcessSwarm(Ptr<Packet> p, SwarmHeader& sh);

    // ---- Scout deduplication ----
    bool IsDuplicateScout(const ScoutId& id);
    void MarkSeen(const ScoutId& id);

    // ---- Periodic maintenance ----
    void PeriodicMaintenance();
    void FloodScout();  // proactive hello broadcast for route pre-discovery
    void SendControlUnicast(Ptr<Packet> ctrl, Ipv4Address nextHop); // bypass RouteOutput

    uint32_t GetNextSeqno() { return ++m_seqno; }

    // ---- Members ----
    Ptr<Ipv4>   m_ipv4;
    Ptr<Socket> m_socket;
    uint16_t    m_port          {9898};

    // Protocol parameters (matching paper's simulation)
    uint8_t  m_initialTtl       {20};    // scout max hops
    Time     m_packerTimeout    {Seconds(15.0)};  // wait before launching scout
    double   m_energyThreshold  {0.5};   // min energy to participate (J)
    uint32_t m_swarmThreshold   {3};     // imbalance before launching swarm
    uint32_t m_seqno            {0};

    uint32_t m_maintenanceTick {0};
    // The three hive components
    DanceFloor m_danceFloor;

    // Packing floor: queued packets waiting for a forager
    std::map<Ipv4Address, std::list<PackerEntry>> m_packerQueue;

    // Entrance: seen scouts table (Section 3.2)
    std::map<ScoutId, Time>  m_seenScouts;

    // Swarm balance tracking: outgoing vs incoming foragers per neighbour
    std::map<Ipv4Address, int32_t> m_foragerBalance; // positive = more out than in

    // Pending scout launches (avoid duplicate discovery)
    std::map<Ipv4Address, Time> m_scoutPending;

    Timer m_maintenanceTimer;
    Ipv4StaticRoutingHelper m_staticRoutingHelper;
    bool m_debugTracing {false};

    TracedCallback<Ptr<const Packet>> m_txTrace;
    TracedCallback<Ptr<const Packet>> m_rxTrace;
};

} // namespace beeadhoc
} // namespace ns3

#endif /* BEE_ADHOC_H */
