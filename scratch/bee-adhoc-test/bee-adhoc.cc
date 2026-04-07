#include "bee-adhoc.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/address-utils.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace ns3 {
namespace beeadhoc {

NS_LOG_COMPONENT_DEFINE("BeeAdHoc");

// ============================================================
// ForwardScoutHeader
// ============================================================
// NS_OBJECT_ENSURE_REGISTERED(ForwardScoutHeader);

ForwardScoutHeader::ForwardScoutHeader() {}
ForwardScoutHeader::~ForwardScoutHeader() {}

TypeId ForwardScoutHeader::GetTypeId() {
    static TypeId tid = TypeId("ns3::beeadhoc::ForwardScoutHeader")
        .SetParent<Header>().SetGroupName("BeeAdHoc")
        .AddConstructor<ForwardScoutHeader>();
    return tid;
}
TypeId ForwardScoutHeader::GetInstanceTypeId() const { return GetTypeId(); }

void ForwardScoutHeader::AddHop(Ipv4Address addr) {
    m_route.push_back(addr);
    m_hopCount++;
}

void ForwardScoutHeader::Print(std::ostream& os) const {
    os << "FS src=" << m_src << " dst=" << m_dst
       << " seq=" << m_seqno << " ttl=" << (int)m_ttl
       << " hops=" << (int)m_hopCount << " avgE=" << GetAvgEnergy();
}

uint32_t ForwardScoutHeader::GetSerializedSize() const {
    // src(4)+dst(4)+seqno(4)+ttl(1)+totalE(8)+hopCount(1)+type(1)+rsize(1)+route(4*n)
    return 24 + (4 * m_route.size());
}

void ForwardScoutHeader::Serialize(Buffer::Iterator i) const {
    ns3::WriteTo(i, m_src);
    ns3::WriteTo(i, m_dst);
    i.WriteHtonU32(m_seqno);
    i.WriteU8(m_ttl);
    uint64_t e; std::memcpy(&e, &m_totalEnergy, 8);
    i.WriteHtonU64(e);
    i.WriteU8(m_hopCount);
    i.WriteU8((uint8_t)m_type);
    i.WriteU8((uint8_t)m_route.size());
    for (const auto& a : m_route) ns3::WriteTo(i, a);
}

uint32_t ForwardScoutHeader::Deserialize(Buffer::Iterator i) {
    ns3::ReadFrom(i, m_src);
    ns3::ReadFrom(i, m_dst);
    m_seqno = i.ReadNtohU32();
    m_ttl   = i.ReadU8();
    uint64_t e = i.ReadNtohU64(); std::memcpy(&m_totalEnergy, &e, 8);
    m_hopCount = i.ReadU8();
    m_type     = (ForagerType)i.ReadU8();
    uint8_t n  = i.ReadU8();
    m_route.clear();
    for (int k = 0; k < n; k++) {
        Ipv4Address a; ns3::ReadFrom(i, a); m_route.push_back(a);
    }
    return GetSerializedSize();
}

// ============================================================
// BackwardScoutHeader
// ============================================================
// NS_OBJECT_ENSURE_REGISTERED(BackwardScoutHeader);

BackwardScoutHeader::BackwardScoutHeader() {}
BackwardScoutHeader::~BackwardScoutHeader() {}

TypeId BackwardScoutHeader::GetTypeId() {
    static TypeId tid = TypeId("ns3::beeadhoc::BackwardScoutHeader")
        .SetParent<Header>().SetGroupName("BeeAdHoc")
        .AddConstructor<BackwardScoutHeader>();
    return tid;
}
TypeId BackwardScoutHeader::GetInstanceTypeId() const { return GetTypeId(); }

void BackwardScoutHeader::Print(std::ostream& os) const {
    os << "BS src=" << m_src << " dst=" << m_dst
       << " avgE=" << m_avgEnergy << " idx=" << (int)m_routeIndex;
}

uint32_t BackwardScoutHeader::GetSerializedSize() const {
    return 4+4+4+8+1+1+1+1+(4*m_route.size());
}

void BackwardScoutHeader::Serialize(Buffer::Iterator i) const {
    ns3::WriteTo(i, m_src);
    ns3::WriteTo(i, m_dst);
    i.WriteHtonU32(m_seqno);
    uint64_t e; std::memcpy(&e, &m_avgEnergy, 8);
    i.WriteHtonU64(e);
    i.WriteU8(m_hopCount);
    i.WriteU8(m_routeIndex);
    i.WriteU8((uint8_t)m_type);
    i.WriteU8((uint8_t)m_route.size());
    for (const auto& a : m_route) ns3::WriteTo(i, a);
}

uint32_t BackwardScoutHeader::Deserialize(Buffer::Iterator i) {
    ns3::ReadFrom(i, m_src);
    ns3::ReadFrom(i, m_dst);
    m_seqno = i.ReadNtohU32();
    uint64_t e = i.ReadNtohU64(); std::memcpy(&m_avgEnergy, &e, 8);
    m_hopCount   = i.ReadU8();
    m_routeIndex = i.ReadU8();
    m_type       = (ForagerType)i.ReadU8();
    uint8_t n    = i.ReadU8();
    m_route.clear();
    for (int k = 0; k < n; k++) {
        Ipv4Address a; ns3::ReadFrom(i, a); m_route.push_back(a);
    }
    return GetSerializedSize();
}

// ============================================================
// ForagerHeader
// ============================================================
// NS_OBJECT_ENSURE_REGISTERED(ForagerHeader);

ForagerHeader::ForagerHeader() {}
ForagerHeader::~ForagerHeader() {}

TypeId ForagerHeader::GetTypeId() {
    static TypeId tid = TypeId("ns3::beeadhoc::ForagerHeader")
        .SetParent<Header>().SetGroupName("BeeAdHoc")
        .AddConstructor<ForagerHeader>();
    return tid;
}
TypeId ForagerHeader::GetInstanceTypeId() const { return GetTypeId(); }

void ForagerHeader::Print(std::ostream& os) const {
    os << "Forager src=" << m_src << " dst=" << m_dst
       << " idx=" << (int)m_routeIndex;
}

uint32_t ForagerHeader::GetSerializedSize() const {
    return 4+4+1+1+8+1+1+(4*m_route.size());
}

void ForagerHeader::Serialize(Buffer::Iterator i) const {
    ns3::WriteTo(i, m_src);
    ns3::WriteTo(i, m_dst);
    i.WriteU8(m_routeIndex);
    i.WriteU8((uint8_t)m_type);
    uint64_t e; std::memcpy(&e, &m_accumEnergy, 8);
    i.WriteHtonU64(e);
    i.WriteU8(m_hopCount);
    i.WriteU8((uint8_t)m_route.size());
    for (const auto& a : m_route) ns3::WriteTo(i, a);
}

uint32_t ForagerHeader::Deserialize(Buffer::Iterator i) {
    ns3::ReadFrom(i, m_src);
    ns3::ReadFrom(i, m_dst);
    m_routeIndex = i.ReadU8();
    m_type       = (ForagerType)i.ReadU8();
    uint64_t e   = i.ReadNtohU64(); std::memcpy(&m_accumEnergy, &e, 8);
    m_hopCount   = i.ReadU8();
    uint8_t n    = i.ReadU8();
    m_route.clear();
    for (int k = 0; k < n; k++) {
        Ipv4Address a; ns3::ReadFrom(i, a); m_route.push_back(a);
    }
    return GetSerializedSize();
}

// ============================================================
// SwarmHeader
// ============================================================
// NS_OBJECT_ENSURE_REGISTERED(SwarmHeader);

SwarmHeader::SwarmHeader() {}
SwarmHeader::~SwarmHeader() {}

TypeId SwarmHeader::GetTypeId() {
    static TypeId tid = TypeId("ns3::beeadhoc::SwarmHeader")
        .SetParent<Header>().SetGroupName("BeeAdHoc")
        .AddConstructor<SwarmHeader>();
    return tid;
}
TypeId SwarmHeader::GetInstanceTypeId() const { return GetTypeId(); }

void SwarmHeader::Print(std::ostream& os) const {
    os << "Swarm src=" << m_src << " dst=" << m_dst
       << " count=" << (int)m_foragerCount << " avgE=" << m_avgEnergy;
}

uint32_t SwarmHeader::GetSerializedSize() const {
    // src(4)+dst(4)+foragerCount(1)+avgEnergy(8)+hopCount(1)+routeIndex(1)+routeSize(1)+route(4*n)
    return 4+4+1+8+1+1+1+(4*m_route.size());
}

void SwarmHeader::Serialize(Buffer::Iterator i) const {
    ns3::WriteTo(i, m_src);
    ns3::WriteTo(i, m_dst);
    i.WriteU8(m_foragerCount);
    uint64_t e; std::memcpy(&e, &m_avgEnergy, 8);
    i.WriteHtonU64(e);
    i.WriteU8(m_hopCount);
    i.WriteU8(m_routeIndex);                      // was missing — multi-hop swarms broken without this
    i.WriteU8((uint8_t)m_route.size());
    for (const auto& a : m_route) ns3::WriteTo(i, a);
}

uint32_t SwarmHeader::Deserialize(Buffer::Iterator i) {
    ns3::ReadFrom(i, m_src);
    ns3::ReadFrom(i, m_dst);
    m_foragerCount = i.ReadU8();
    uint64_t e = i.ReadNtohU64(); std::memcpy(&m_avgEnergy, &e, 8);
    m_hopCount   = i.ReadU8();
    m_routeIndex = i.ReadU8();                    // was missing — restored here
    uint8_t n    = i.ReadU8();
    m_route.clear();
    for (int k = 0; k < n; k++) {
        Ipv4Address a; ns3::ReadFrom(i, a); m_route.push_back(a);
    }
    return GetSerializedSize();
}

// ============================================================
// DanceFloor
// Paper Section 3.3
// ============================================================
DanceFloor::DanceFloor()
{
    // Create the RNG once here rather than on every GetForager() call.
    // CreateObject<> is expensive (TypeId lookup + heap alloc); doing it
    // per-packet would dominate runtime in busy simulations.
    m_rng = CreateObject<UniformRandomVariable>();
}

void DanceFloor::AddForager(const ForagerEntry& f) {
    auto& list = m_foragers[f.dst];

    // Remove expired entries
    list.remove_if([](const ForagerEntry& e){ return e.IsExpired(); });

    // Paper: young foragers (newer routes) are favoured.
    // Insert at front so newest is tried first.
    list.push_front(f);

    // Cap at reasonable number per destination
    while (list.size() > 10) list.pop_back();

    NS_LOG_DEBUG("DanceFloor: Added forager to " << f.dst
                 << " quality=" << f.quality
                 << " danceNum=" << f.danceNum
                 << " total=" << list.size());
}

bool DanceFloor::GetForager(Ipv4Address dst, ForagerEntry& chosen) {
    auto it = m_foragers.find(dst);
    if (it == m_foragers.end()) return false;

    auto& list = it->second;
    // Purge expired/exhausted entries first
    list.remove_if([](const ForagerEntry& e){ return e.IsExpired() || e.danceNum == 0; });
    if (list.empty()) return false;

    // Paper Section 3.3: stochastic weighted selection.
    // Weight = quality * danceNum. Higher quality/busier routes preferred.
    double totalWeight = 0;
    for (const auto& e : list) totalWeight += e.quality * (double)e.danceNum;

    // Use the persistent RNG — not a freshly allocated one per call.
    double pick  = (totalWeight > 0) ? m_rng->GetValue(0, totalWeight) : 0;
    double cumul = 0;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        cumul += iter->quality * (double)iter->danceNum;
        if (totalWeight <= 0 || pick <= cumul) {
            // Decrement BEFORE copying so chosen.danceNum is consistent
            // with what remains in the list (avoids a stale +1 value).
            --iter->danceNum;
            chosen = *iter;
            if (iter->danceNum == 0) list.erase(iter);
            return true;
        }
    }
    // Fallback: floating-point rounding left pick just above cumul at the
    // last entry — pick the front entry.
    --list.front().danceNum;
    chosen = list.front();
    if (list.front().danceNum == 0) list.pop_front();
    return true;
}

bool DanceFloor::HasForager(Ipv4Address dst) const {
    auto it = m_foragers.find(dst);
    if (it == m_foragers.end()) return false;
    for (const auto& e : it->second) {
        if (!e.IsExpired() && e.danceNum > 0) return true;
    }
    return false;
}

uint32_t DanceFloor::CountForagers(Ipv4Address dst) const {
    auto it = m_foragers.find(dst);
    if (it == m_foragers.end()) return 0;
    uint32_t n = 0;
    for (const auto& e : it->second) {
        if (!e.IsExpired()) n += e.danceNum;
    }
    return n;
}

uint32_t DanceFloor::CountOutgoing(Ipv4Address via) const {
    uint32_t n = 0;
    for (const auto& [dst, list] : m_foragers) {
        for (const auto& e : list) {
            // Sum danceNums, not entry count.  The paper's imbalance check
            // compares individual forager instances (each dance = one forager),
            // so counting entries would under-report multi-dance foragers.
            if (!e.IsExpired() && e.route.size() > 1 && e.route[1] == via)
                n += e.danceNum;
        }
    }
    return n;
}

void DanceFloor::UpdateForager(Ipv4Address dst,
                               const std::vector<Ipv4Address>& route,
                               double newQuality)
{
    auto it = m_foragers.find(dst);
    if (it == m_foragers.end()) return;
    for (auto& e : it->second) {
        if (e.route == route) {
            // Paper: returning forager updates quality and recruits new dances.
            // Use the same capped formula as ProcessBackwardScout so danceNum
            // stays in [1, 20].  The old `newQuality * 5` could produce 500
            // dances for a 100 J node — far outside the intended range.
            uint32_t newDance = std::max(1u,
                std::min(20u, (uint32_t)(newQuality / 20.0) + 1));
            e.quality   = newQuality;
            e.danceNum  = std::max(e.danceNum, newDance);
            e.createdAt = Simulator::Now(); // refresh age
            return;
        }
    }
}

void DanceFloor::Purge() {
    for (auto& [dst, list] : m_foragers) {
        list.remove_if([](const ForagerEntry& e){
            return e.IsExpired() || e.danceNum == 0;
        });
    }
}

// ============================================================
// BeeAdHocRoutingProtocol
// ============================================================
// NS_OBJECT_ENSURE_REGISTERED(BeeAdHocRoutingProtocol);

TypeId BeeAdHocRoutingProtocol::GetTypeId() {
    static TypeId tid = TypeId("ns3::beeadhoc::BeeAdHocRoutingProtocol")
        .SetParent<Ipv4RoutingProtocol>()
        .SetGroupName("BeeAdHoc")
        .AddConstructor<BeeAdHocRoutingProtocol>()
        .AddAttribute("InitialTtl", "Scout initial TTL",
            UintegerValue(20),
            MakeUintegerAccessor(&BeeAdHocRoutingProtocol::m_initialTtl),
            MakeUintegerChecker<uint8_t>())
        .AddAttribute("EnergyThreshold", "Min energy (J) to forward scouts",
            DoubleValue(0.5),
            MakeDoubleAccessor(&BeeAdHocRoutingProtocol::m_energyThreshold),
            MakeDoubleChecker<double>())
        .AddAttribute("PackerTimeout", "How long a packer waits before launching scout",
            TimeValue(Seconds(15.0)),
            MakeTimeAccessor(&BeeAdHocRoutingProtocol::m_packerTimeout),
            MakeTimeChecker())
        .AddAttribute("SwarmThreshold",
            "Forager imbalance count before launching a swarm",
            UintegerValue(3),
            MakeUintegerAccessor(&BeeAdHocRoutingProtocol::m_swarmThreshold),
            MakeUintegerChecker<uint32_t>())
        .AddTraceSource("Tx", "Packet sent",
            MakeTraceSourceAccessor(&BeeAdHocRoutingProtocol::m_txTrace),
            "ns3::Packet::TracedCallback")
        .AddTraceSource("Rx", "Packet received",
            MakeTraceSourceAccessor(&BeeAdHocRoutingProtocol::m_rxTrace),
            "ns3::Packet::TracedCallback");
    return tid;
}

BeeAdHocRoutingProtocol::BeeAdHocRoutingProtocol()
    : m_port(9898), m_initialTtl(20),
      // FIX (Bug 5): Original timeout was 2s. Scout round-trip across a
      // 800x800m network with mobile nodes easily exceeds 2s, so ALL
      // buffered packets expired before the backward scout arrived.
      // 15s gives enough headroom for multi-hop discovery to complete.
      m_packerTimeout(Seconds(15.0)),
      m_energyThreshold(0.5), m_swarmThreshold(3), m_seqno(0),
      m_maintenanceTimer(Timer::CANCEL_ON_DESTROY)
{
    m_danceFloor.SetForagerLifetime(Seconds(30));
    m_danceFloor.SetInitialDanceNum(5);
}

BeeAdHocRoutingProtocol::~BeeAdHocRoutingProtocol() {}

void BeeAdHocRoutingProtocol::SetIpv4(Ptr<Ipv4> ipv4) {
    m_ipv4 = ipv4;
    // FIX (Bug 3): Original used 0.5s which was not enough time for the
    // WiFi interface to get its IP address assigned. GetLocalAddress()
    // returned 0.0.0.0 at Start() time, corrupting all scout src fields.
    // 1.0s is sufficient for NS-3 interface setup to complete.
    Simulator::Schedule(Seconds(1.0), &BeeAdHocRoutingProtocol::Start, this);
}

void BeeAdHocRoutingProtocol::Start() {
    NS_LOG_FUNCTION(this << GetLocalAddress());

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(m_ipv4->GetObject<Node>(), tid);
    [[maybe_unused]] int bindRes =
        m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_port));
    m_socket->SetAllowBroadcast(true);
    m_socket->SetRecvCallback(
        MakeCallback(&BeeAdHocRoutingProtocol::RecvEntrance, this));

    m_maintenanceTimer.SetFunction(
        &BeeAdHocRoutingProtocol::PeriodicMaintenance, this);
    m_maintenanceTimer.Schedule(Seconds(1.0));

    // FIX (Bug 1/3): Original fired FloodScout at t=0.55s (0.5s Start delay +
    // 0.05s here).  GetLocalAddress() still returns 0.0.0.0 at that point so
    // every flood scout had src=0.0.0.0 and the backward scouts that came back
    // never matched the source check -> no foragers ever installed.
    // Fix: delay FloodScout to t=2.0s so the WiFi interface has a valid IP.
    // Also guard inside FloodScout itself.
    Simulator::Schedule(Seconds(1.5),
        &BeeAdHocRoutingProtocol::FloodScout, this);
}

// Flood a scout to ALL nodes by using a wildcard destination.
// We send a ForwardScout to 255.255.255.255 as dst — every node that
// receives it will send a BackwardScout back to us, populating the dance floor.
void BeeAdHocRoutingProtocol::FloodScout() {
    if (!m_socket) {
        return;
    }

    // FIX (Bug 3): guard against launching a flood with src=0.0.0.0
    Ipv4Address myAddr = GetLocalAddress();
    if (myAddr == Ipv4Address("0.0.0.0")) {
        Simulator::Schedule(Seconds(1.0),
            &BeeAdHocRoutingProtocol::FloodScout, this);
        return;
    }

    ForwardScoutHeader fsh;
    fsh.SetSrc(myAddr);
    fsh.SetDst(Ipv4Address("255.255.255.255"));
    fsh.SetSeqno(GetNextSeqno());
    fsh.SetTtl(m_initialTtl);
    fsh.SetTotalEnergy(GetResidualEnergy());
    fsh.SetHopCount(0);
    fsh.SetType(FORAGER_LIFETIME);
    fsh.AddHop(myAddr);

    ScoutId sid; sid.src = fsh.GetSrc(); sid.seqno = fsh.GetSeqno();
    MarkSeen(sid);

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(fsh);
    uint8_t t = PKT_FORWARD_SCOUT;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);
    m_socket->SendTo(ctrl, 0,
        InetSocketAddress(Ipv4Address("255.255.255.255"), m_port));
}

// ---- Utilities ----

Ipv4Address BeeAdHocRoutingProtocol::GetLocalAddress() const {
    for (uint32_t i = 1; i < m_ipv4->GetNInterfaces(); i++) {
        for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); j++) {
            auto addr = m_ipv4->GetAddress(i, j).GetLocal();
            if (addr != Ipv4Address::GetLoopback()) return addr;
        }
    }
    // BUG-ADDR: returning 0.0.0.0 means no WiFi interface has an IP yet.
    // Any scout launched now will embed src=0.0.0.0; when the backward scout
    // returns, bsh.GetSrc()==myAddr will NEVER match -> forager never installed.
    return Ipv4Address("0.0.0.0");
}

bool BeeAdHocRoutingProtocol::IsMyAddress(Ipv4Address addr) const {
    for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); i++)
        for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); j++)
            if (m_ipv4->GetAddress(i, j).GetLocal() == addr) return true;
    return false;
}

double BeeAdHocRoutingProtocol::GetResidualEnergy() const {
    // Energy container is aggregated on the Node, not on this protocol object
    Ptr<Node> node = m_ipv4->GetObject<Node>();
    if (!node) return 100.0;
    Ptr<energy::EnergySourceContainer> src =
        node->GetObject<energy::EnergySourceContainer>();
    if (src && src->GetN() > 0) return src->Get(0)->GetRemainingEnergy();
    return 100.0;
}

Ptr<Ipv4Route> BeeAdHocRoutingProtocol::BuildRoute(
    Ipv4Address dst, Ipv4Address nextHop) const
{
    // Find a valid non-loopback output device
    Ptr<NetDevice> dev = nullptr;
    for (uint32_t i = 1; i < m_ipv4->GetNInterfaces(); i++) {
        Ptr<NetDevice> d = m_ipv4->GetNetDevice(i);
        if (d) { dev = d; break; }
    }
    if (!dev) {
        NS_LOG_WARN("BeeAdHoc::BuildRoute: no valid net device found");
        return nullptr;
    }

    Ptr<Ipv4Route> rt = Create<Ipv4Route>();
    rt->SetDestination(dst);
    rt->SetSource(GetLocalAddress());
    rt->SetGateway(nextHop);
    rt->SetOutputDevice(dev);
    return rt;
}

// Send a control packet unicast directly to a neighbor, bypassing RouteOutput.
// Uses Ipv4StaticRouting to temporarily install a /32 host route, sends, then
// removes it — so the UDP socket succeeds even before app routes exist.
void BeeAdHocRoutingProtocol::SendControlUnicast(
    Ptr<Packet> ctrl, Ipv4Address nextHop)
{
    if (!m_socket || !m_ipv4) {
        return;
    }

    // ---- FIX (Bug 4) ----
    // Original code relied on Ipv4StaticRouting being installed so it could
    // temporarily add a /32 host route before calling SendTo().  When BeeAdHoc
    // is the sole routing protocol Ipv4StaticRouting is NOT installed and
    // GetStaticRouting() returns nullptr -> every backward scout and swarm was
    // silently dropped here, which is why TX packets = 0.
    //
    // Fix: send directly on the already-bound socket.  Because the socket was
    // bound with Ipv4Address::GetAny() and SetAllowBroadcast(true) the UDP/IP
    // stack will accept the SendTo() even without a routing entry; the WiFi
    // MAC handles the L2 unicast to nextHop within the same subnet.
    int result = m_socket->SendTo(ctrl, 0, InetSocketAddress(nextHop, m_port));
    (void) result;
}

// ---- RouteOutput — called by NS-3 when app wants to send ----

Ptr<Ipv4Route> BeeAdHocRoutingProtocol::RouteOutput(
    Ptr<Packet> p, const Ipv4Header& header,
    Ptr<NetDevice> oif, Socket::SocketErrno& sockerr)
{
    Ipv4Address dst = header.GetDestination();

    // --- existing broadcast/multicast check ---
    if (dst.IsMulticast() || dst.IsBroadcast()) {
        Ptr<Ipv4Route> rt = BuildRoute(dst, dst);
        if (!rt) { sockerr = Socket::ERROR_NOROUTETOHOST; return nullptr; }
        sockerr = Socket::ERROR_NOTERROR;
        return rt;
    }

    // Check dance floor — do we have a forager for this destination?
    ForagerEntry fe;
    if (m_danceFloor.GetForager(dst, fe) && fe.route.size() >= 2) {
        SendForager(fe);
        sockerr = Socket::ERROR_NOTERROR;
        return BuildRoute(dst, fe.route[1]);
    }

    // *** FIX: same-subnet direct delivery ***
    // When SendControlUnicast sends a backward scout or swarm to a neighbor,
    // it calls socket->SendTo() which hits RouteOutput. No forager exists yet
    // (that's WHY we're discovering a route), so the DanceFloor check above
    // always misses, returning nullptr and killing the backward scout.
    //
    // For any destination on the same /24, we know the WiFi MAC can reach it
    // directly — just hand back a direct route so the IP stack forwards it.
    // This only applies to same-subnet neighbors (backward scouts, swarms,
    // forager probes). Multi-hop data flows still go through the DanceFloor.
    for (uint32_t i = 1; i < m_ipv4->GetNInterfaces(); i++) {
        for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); j++) {
            Ipv4InterfaceAddress ifAddr = m_ipv4->GetAddress(i, j);
            if (ifAddr.IsInSameSubnet(dst)) {
                Ptr<Ipv4Route> rt = BuildRoute(dst, dst); // direct, no gateway
                if (rt) {
                    sockerr = Socket::ERROR_NOTERROR;
                    return rt;
                }
            }
        }
    }

    if (p) {
        PackerEntry pe;
        pe.packet      = p->Copy();
        pe.ipHdr       = header;
        // Ensure source address is set — NS-3 sometimes passes 0.0.0.0
        // at RouteOutput time and fills it in later; we need it now for SendWithHeader.
        if (pe.ipHdr.GetSource().IsAny() ||
            pe.ipHdr.GetSource() == Ipv4Address("0.0.0.0")) {
            pe.ipHdr.SetSource(GetLocalAddress());
        }
        pe.createdAt   = Simulator::Now();
        pe.waitTimeout = m_packerTimeout;
        // ucb is not available in RouteOutput — left as null, DrainPackerQueue
        // will use SendWithHeader with a pre-built route instead.
        m_packerQueue[dst].push_back(pe);
    }

    if (m_scoutPending.find(dst) == m_scoutPending.end() ||
        Simulator::Now() > m_scoutPending[dst])
    {
        m_scoutPending[dst] = Simulator::Now() + Seconds(5.0);
        LaunchForwardScout(dst, FORAGER_LIFETIME);
    }

    sockerr = Socket::ERROR_NOROUTETOHOST;
    return nullptr;
}

// ---- RouteInput — called by NS-3 when packet arrives ----

bool BeeAdHocRoutingProtocol::RouteInput(
    Ptr<const Packet> p, const Ipv4Header& header,
    Ptr<const NetDevice> idev,
    const UnicastForwardCallback& ucb,
    const MulticastForwardCallback& mcb,
    const LocalDeliverCallback& lcb,
    const ErrorCallback& ecb)
{
    Ipv4Address dst = header.GetDestination();

    // Broadcast/multicast: deliver locally so RecvEntrance picks up scout
    // broadcasts on port 9898. NS-3 will call lcb which routes to our UDP socket.
    if (dst.IsBroadcast() || dst.IsMulticast()) {
        int32_t iface = idev ? m_ipv4->GetInterfaceForDevice(idev) : 0;
        lcb(p, header, iface);
        return true;
    }

    // Local unicast delivery — covers backward scouts, swarms, and app data.
    if (IsMyAddress(dst)) {
        int32_t iface = idev ? m_ipv4->GetInterfaceForDevice(idev) : 0;
        lcb(p, header, iface);
        return true;
    }

    ForagerEntry fe;
    if (m_danceFloor.GetForager(dst, fe) && fe.route.size() >= 2) {
        Ptr<Ipv4Route> rt = BuildRoute(dst, fe.route[1]);
        if (rt) {
            ucb(rt, p, header);
            return true;
        }
    }

    PackingFloorReceive(p, header, ucb, ecb);
    return true;
}

// ============================================================
// PACKING FLOOR (Section 3.1)
// Paper: packer created for each data packet. Checks dance
// floor for forager. If found, hands packet to forager.
// If not, waits, then launches scout.
// ============================================================

void BeeAdHocRoutingProtocol::PackingFloorReceive(
    Ptr<const Packet> p, const Ipv4Header& hdr,
    const UnicastForwardCallback& ucb, const ErrorCallback& ecb)
{
    Ipv4Address dst = hdr.GetDestination();
    NS_LOG_DEBUG("BeeAdHoc PackingFloor: packet for " << dst);

    // Check dance floor immediately - if route known, forward via NS-3 ucb
    ForagerEntry fe;
    if (m_danceFloor.GetForager(dst, fe) && fe.route.size() >= 2) {
        Ptr<Ipv4Route> rt = BuildRoute(dst, fe.route[1]);
        if (rt && !ucb.IsNull()) {
            NS_LOG_DEBUG("BeeAdHoc PackingFloor: route known, forwarding via ucb");
            ucb(rt, p, hdr);
            return;
        }
    }

    // No route yet: buffer the packet
    PackerEntry pe;
    pe.packet      = p->Copy();
    pe.ipHdr       = hdr;
    pe.ucb         = ucb;
    pe.ecb         = ecb;
    pe.createdAt   = Simulator::Now();
    pe.waitTimeout = m_packerTimeout;
    m_packerQueue[dst].push_back(pe);

    NS_LOG_DEBUG("BeeAdHoc PackingFloor: queued packer for " << dst
                 << " total=" << m_packerQueue[dst].size());

    // Launch scout if not already pending
    if (m_scoutPending.find(dst) == m_scoutPending.end() ||
        Simulator::Now() > m_scoutPending[dst])
    {
        m_scoutPending[dst] = Simulator::Now() + Seconds(5.0);
        LaunchForwardScout(dst, FORAGER_LIFETIME);
    }
}

void BeeAdHocRoutingProtocol::DrainPackerQueue(Ipv4Address dst) {
    auto it = m_packerQueue.find(dst);
    if (it == m_packerQueue.end()) return;

    NS_LOG_DEBUG("BeeAdHoc: Draining " << it->second.size()
                 << " packers for " << dst);

    // Paper Section 3.3: each data packet consumes one dance count from the
    // forager that carries it.  GetForager() is therefore called once per
    // packet so every forwarded packet decrements the dance number by exactly
    // one, matching the "clone and send" semantics described in the paper.
    for (auto& pe : it->second) {
        ForagerEntry fe;
        if (!m_danceFloor.GetForager(dst, fe) || fe.route.size() < 2) {
            if (!pe.ecb.IsNull())
                pe.ecb(pe.packet, pe.ipHdr, Socket::ERROR_NOROUTETOHOST);
            continue;
        }

        Ptr<Ipv4Route> rt = BuildRoute(dst, fe.route[1]);
        if (!rt || !rt->GetOutputDevice()) {
            continue;
        }

        if (!pe.ucb.IsNull()) {
            pe.ucb(rt, pe.packet, pe.ipHdr);
        } else {
            Ptr<Packet> pkt = pe.packet->Copy();
            m_ipv4->SendWithHeader(pkt, pe.ipHdr, rt);
        }
        SendForager(fe);
    }
    m_packerQueue.erase(it);
    m_scoutPending.erase(dst);
}

void BeeAdHocRoutingProtocol::CheckPackerQueue() {
    Time now = Simulator::Now();
    std::vector<Ipv4Address> toDrain;
    for (auto& [dst, list] : m_packerQueue) {
        list.remove_if([&](const PackerEntry& pe) {
            if ((now - pe.createdAt) > pe.waitTimeout) {
                if (!pe.ecb.IsNull())
                    pe.ecb(pe.packet, pe.ipHdr, Socket::ERROR_NOROUTETOHOST);
                return true;
            }
            return false;
        });
        // Retry: maybe a forager arrived while we were waiting
        if (!list.empty() && m_danceFloor.HasForager(dst)) {
            toDrain.push_back(dst);
        }
    }
    for (auto& dst : toDrain) DrainPackerQueue(dst);
    // Remove empty queues
    for (auto it = m_packerQueue.begin(); it != m_packerQueue.end(); ) {
        if (it->second.empty()) it = m_packerQueue.erase(it);
        else ++it;
    }
}

// ============================================================
// ENTRANCE (Section 3.2)
// Paper: handles all incoming/outgoing packets.
// Deduplicates scouts, routes foragers, forwards to packing floor.
// ============================================================

void BeeAdHocRoutingProtocol::RecvEntrance(Ptr<Socket> socket) {
    Ptr<Packet> p;
    Address from;
    while ((p = socket->RecvFrom(from))) {
        m_rxTrace(p);
        if (p->GetSize() == 0) continue;

        uint8_t typeBuf[1];
        p->CopyData(typeBuf, 1);
        p->RemoveAtStart(1);

        switch ((BeePacketType)typeBuf[0]) {
            case PKT_FORWARD_SCOUT: {
                ForwardScoutHeader fsh;
                p->RemoveHeader(fsh);
                ProcessForwardScout(p, fsh);
                break;
            }
            case PKT_BACKWARD_SCOUT: {
                BackwardScoutHeader bsh;
                p->RemoveHeader(bsh);
                ProcessBackwardScout(p, bsh);
                break;
            }
            case PKT_FORAGER: {
                // Energy-probe packet: ForagerHeader only, no data payload.
                // Data travels via the NS-3 routing stack; this probe collects
                // per-hop residual energy and triggers a swarm at the destination
                // so the source can refresh DanceFloor route quality.
                ForagerHeader fh;
                p->RemoveHeader(fh);
                ProcessForager(p, fh);
                break;
            }
            case PKT_SWARM: {
                SwarmHeader sh;
                p->RemoveHeader(sh);
                ProcessSwarm(p, sh);
                break;
            }
            default:
                NS_LOG_WARN("BeeAdHoc Entrance: unknown type " << (int)typeBuf[0]);
        }
    }
}

// ============================================================
// SCOUTS — Forward and Backward
// ============================================================

void BeeAdHocRoutingProtocol::LaunchForwardScout(
    Ipv4Address dst, ForagerType type)
{
    NS_LOG_FUNCTION(this << dst);

    if (!m_socket) {
        return;
    }

    double myEnergy = GetResidualEnergy();
    if (myEnergy < m_energyThreshold) {
        return;
    }

    // FIX (Bug 3): abort if local address is still 0.0.0.0
    Ipv4Address myAddr = GetLocalAddress();
    if (myAddr == Ipv4Address("0.0.0.0")) {
        return;
    }

    ForwardScoutHeader fsh;
    fsh.SetSrc(myAddr);
    fsh.SetDst(dst);
    fsh.SetSeqno(GetNextSeqno());
    fsh.SetTtl(m_initialTtl);
    fsh.SetTotalEnergy(myEnergy);
    fsh.SetHopCount(0);
    fsh.SetType(type);
    fsh.AddHop(myAddr);

    ScoutId sid; sid.src = fsh.GetSrc(); sid.seqno = fsh.GetSeqno();
    MarkSeen(sid);

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(fsh);
    uint8_t t = PKT_FORWARD_SCOUT;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);

    m_socket->SendTo(ctrl, 0,
        InetSocketAddress(Ipv4Address("255.255.255.255"), m_port));
    m_txTrace(ctrl);
}

void BeeAdHocRoutingProtocol::ProcessForwardScout(
    Ptr<Packet> p, ForwardScoutHeader& fsh)
{
    Ipv4Address myAddr = GetLocalAddress();

    // Ignore if we sent this (normal -- we see our own broadcast)
    if (fsh.GetSrc() == myAddr) return;

    ScoutId sid; sid.src = fsh.GetSrc(); sid.seqno = fsh.GetSeqno();
    if (IsDuplicateScout(sid)) {
        return;
    }
    MarkSeen(sid);

    if (fsh.TtlExpired()) {
        return;
    }

    double myEnergy = GetResidualEnergy();
    if (myEnergy < m_energyThreshold) {
        return;
    }

    fsh.SetTotalEnergy(fsh.GetTotalEnergy() + myEnergy);
    fsh.AddHop(myAddr);
    fsh.DecrementTtl();

    bool isHelloFlood = fsh.GetDst().IsBroadcast();
    if (fsh.GetDst() == myAddr || isHelloFlood) {
        SendBackwardScout(fsh);
        // FIX (Bug 1): For hello floods, do NOT rebroadcast. The original code
        // rebroadcast hello floods, creating an exponential control-packet storm
        // (every node rebroadcasts to all neighbours, up to TTL=20 hops) that
        // consumed all channel airtime and prevented backward scouts from
        // getting through. Normal unicast scouts ARE rebroadcast as usual.
        return;
    }

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(fsh);
    uint8_t t = PKT_FORWARD_SCOUT;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);
    m_socket->SendTo(ctrl, 0,
        InetSocketAddress(Ipv4Address("255.255.255.255"), m_port));
    m_txTrace(ctrl);
}

void BeeAdHocRoutingProtocol::SendBackwardScout(const ForwardScoutHeader& fsh) {
    std::vector<Ipv4Address> route = fsh.GetRoute();
    if (route.empty()) {
        return;
    }

    double avgEnergy = fsh.GetAvgEnergy();

    BackwardScoutHeader bsh;
    bsh.SetSrc(fsh.GetSrc());
    bsh.SetDst(GetLocalAddress());
    bsh.SetSeqno(fsh.GetSeqno());
    bsh.SetAvgEnergy(avgEnergy);
    bsh.SetHopCount(fsh.GetHopCount());
    bsh.SetType(fsh.GetType());
    bsh.SetRoute(route);

    Ipv4Address nextHop;
    if (route.size() >= 2) {
        bsh.SetRouteIndex((uint8_t)(route.size() - 2));
        nextHop = route[route.size() - 2];
    } else {
        bsh.SetRouteIndex(0);
        nextHop = route[0];
    }

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(bsh);
    uint8_t t = PKT_BACKWARD_SCOUT;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);
    SendControlUnicast(ctrl, nextHop);
    m_txTrace(ctrl);
}

void BeeAdHocRoutingProtocol::ProcessBackwardScout(
    Ptr<Packet> p, BackwardScoutHeader& bsh)
{
    Ipv4Address myAddr = GetLocalAddress();

    // Are we the original source?
    if (bsh.GetSrc() == myAddr) {
        uint32_t danceNum = std::max(1u,
            std::min(20u, (uint32_t)(bsh.GetAvgEnergy() / 20.0) + 1));

        ForagerEntry fe;
        fe.dst       = bsh.GetDst();
        fe.route     = bsh.GetRoute();
        fe.type      = bsh.GetType();
        fe.quality   = bsh.GetAvgEnergy();
        fe.danceNum  = danceNum;
        fe.createdAt = Simulator::Now();
        fe.lifetime  = Seconds(30);

        m_danceFloor.AddForager(fe);

        m_scoutPending.erase(bsh.GetDst());

        DrainPackerQueue(bsh.GetDst());
        return;
    }

    // Intermediate relay
    int8_t idx = (int8_t)bsh.GetRouteIndex();
    if (idx <= 0) {
        return;
    }

    const auto& route = bsh.GetRoute();
    if ((size_t)idx >= route.size()) {
        return;
    }

    Ipv4Address nextHop = route[(uint8_t)(idx - 1)];
    bsh.SetRouteIndex((uint8_t)(idx - 1));

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(bsh);
    uint8_t t = PKT_BACKWARD_SCOUT;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);
    SendControlUnicast(ctrl, nextHop);
    m_txTrace(ctrl);
}

// ============================================================
// FORAGERS — data delivery
// Paper Section 2.3: follows point-to-point, collects state.
// Once at destination, waits to piggyback back (TCP) or
// returns via swarm (UDP).
// ============================================================

void BeeAdHocRoutingProtocol::SendForager(const ForagerEntry& fe)
{
    // SendForager sends a lightweight energy-probe (ForagerHeader only).
    // The actual data packet is forwarded separately by the NS-3 routing
    // stack (RouteOutput / DrainPackerQueue).  Conflating data delivery with
    // the control socket made delivery impossible because the control socket
    // cannot hand packets to application-layer UDP sockets.
    if (fe.route.size() < 2) return;
    if (!m_socket) return;

    ForagerHeader fh;
    fh.SetSrc(fe.route.front());
    fh.SetDst(fe.route.back());
    fh.SetRoute(fe.route);
    fh.SetRouteIndex(1);            // first hop = route[1]
    fh.SetType(fe.type);
    fh.SetAccumEnergy(GetResidualEnergy()); // source contributes its energy
    fh.SetHopCount(1);              // source counts as hop 1

    Ipv4Address nextHop = fe.route[1];

    // Empty probe packet — ForagerHeader is the entire payload
    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(fh);
    uint8_t t = PKT_FORAGER;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);

    SendControlUnicast(ctrl, nextHop);
    m_txTrace(ctrl);

    // Track outgoing forager for swarm-balance detection (Section 2.4)
    m_foragerBalance[nextHop]++;

    NS_LOG_DEBUG("BeeAdHoc: Sent energy probe to next=" << nextHop
                 << " dst=" << fe.route.back());
}

void BeeAdHocRoutingProtocol::ProcessForager(Ptr<Packet> p, ForagerHeader& fh)
{
    // This processes the energy-probe packet (ForagerHeader only, no data).
    // Data delivery is handled by the NS-3 routing stack independently.
    Ipv4Address myAddr = GetLocalAddress();
    const auto& route  = fh.GetRoute();
    if (route.empty()) return;

    // Every node (including the destination) accumulates its residual energy.
    // This mirrors the forward-scout pattern so avgEnergy reflects the full path.
    fh.SetAccumEnergy(fh.GetAccumEnergy() + GetResidualEnergy());
    fh.SetHopCount(fh.GetHopCount() + 1);

    Ipv4Address finalDst = route.back();

    if (finalDst == myAddr) {
        // Probe reached its destination.
        NS_LOG_DEBUG("BeeAdHoc: energy probe arrived at destination, avgE="
                     << (fh.GetHopCount() > 0
                         ? fh.GetAccumEnergy() / fh.GetHopCount() : 0));

        double avgEnergy = fh.GetHopCount() > 0
                         ? fh.GetAccumEnergy() / fh.GetHopCount()
                         : 0.0;

        // Send a swarm back so the source can update the DanceFloor with
        // fresh path quality (avgEnergy). Reverse the route so the swarm
        // travels src-ward.
        std::vector<Ipv4Address> revRoute = route;
        std::reverse(revRoute.begin(), revRoute.end());
        if (revRoute.size() >= 2) {
            SendSwarm(revRoute[1], 1, avgEnergy, revRoute);
        }
        return;
    }

    // Intermediate node: fix for self-loop bug.
    // Old code: nextHop = route[idx] where idx == routeIndex-on-arrival.
    //   At node A (= route[1]), idx=1, nextHop=route[1]=A → sends to itself.
    // Fix: call AdvanceHop() FIRST so routeIndex points to the NEXT node,
    //   then read route[idx] as the correct forward hop.
    fh.AdvanceHop();                         // routeIndex: 1→2 at first hop
    uint8_t idx = fh.GetRouteIndex();
    if (idx >= (uint8_t)route.size()) {
        NS_LOG_WARN("BeeAdHoc: Forager probe route index out of bounds");
        return;
    }

    Ipv4Address nextHop = route[idx];        // now correctly route[2]=B, etc.

    Ptr<Packet> pkt = p->Copy();
    pkt->AddHeader(fh);
    uint8_t t = PKT_FORAGER;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);

    SendControlUnicast(ctrl, nextHop);
    m_txTrace(ctrl);

    NS_LOG_DEBUG("BeeAdHoc: Forwarding energy probe to " << nextHop);
}

// ============================================================
// SWARMS (Section 2.4)
// Paper: when difference between incoming/outgoing foragers
// at node j from node i exceeds threshold, j launches swarm to i.
// Swarm bundles multiple returning foragers.
// ============================================================

void BeeAdHocRoutingProtocol::CheckSwarmBalance() {
    for (auto& [neighbour, balance] : m_foragerBalance) {
        if (balance > (int32_t)m_swarmThreshold) {
            NS_LOG_DEBUG("BeeAdHoc: Swarm balance " << balance
                         << " to " << neighbour << ", launching swarm");
            // Find a route back to that neighbour via dance floor
            // For simplicity: direct unicast to neighbour with 1 forager
            std::vector<Ipv4Address> route = {GetLocalAddress(), neighbour};
            SendSwarm(neighbour, (uint32_t)balance, GetResidualEnergy(), route);
            balance = 0;
        }
    }
}

void BeeAdHocRoutingProtocol::SendSwarm(
    Ipv4Address to, uint32_t count, double quality,
    const std::vector<Ipv4Address>& route)
{
    if (!m_socket) return;
    NS_LOG_DEBUG("BeeAdHoc: Sending swarm of " << count << " to " << to);

    SwarmHeader sh;
    sh.SetSrc(GetLocalAddress());
    sh.SetDst(route.back());
    sh.SetForagerCount((uint8_t)std::min(count, (uint32_t)255));
    sh.SetAvgEnergy(quality);
    sh.SetHopCount((uint8_t)route.size());
    sh.SetRoute(route);
    sh.SetRouteIndex(1);

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(sh);
    uint8_t t = PKT_SWARM;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);

    SendControlUnicast(ctrl, to);
    m_txTrace(ctrl);
}

void BeeAdHocRoutingProtocol::ProcessSwarm(
    Ptr<Packet> p, SwarmHeader& sh)
{
    Ipv4Address myAddr = GetLocalAddress();

    if (sh.GetDst() == myAddr) {
        // Swarm arrived: extract foragers and add to dance floor
        NS_LOG_DEBUG("BeeAdHoc: Swarm arrived with "
                     << (int)sh.GetForagerCount() << " foragers"
                     << " quality=" << sh.GetAvgEnergy());

        // Paper Section 2.4: foragers extracted from swarm payload
        // and stored as if they arrived normally
        ForagerEntry fe;
        fe.dst      = sh.GetSrc();  // foragers came from the original source
        // Reversed route: swarm carried route dst->...->src
        std::vector<Ipv4Address> rev = sh.GetRoute();
        std::reverse(rev.begin(), rev.end());
        fe.route    = rev;
        fe.type     = FORAGER_LIFETIME;
        fe.quality  = sh.GetAvgEnergy();
        fe.danceNum = sh.GetForagerCount();
        fe.createdAt= Simulator::Now();
        fe.lifetime = Seconds(30);

        m_danceFloor.AddForager(fe);

        // Update forager balance.
        // The swarm route is [data_dst, ..., data_src].  The last hop before
        // the source (= route[size-2]) is the direct neighbour we originally
        // sent probes through.  route[1] is wrong for routes longer than 2
        // hops (it points to the second node from data_dst, not to A).
        const auto& sr = sh.GetRoute();
        if (sr.size() >= 2) {
            m_foragerBalance[sr[sr.size() - 2]]--;
        }
        return;
    }

    // Forward swarm along route
    uint8_t idx = sh.GetRouteIndex();
    const auto& route = sh.GetRoute();
    if (idx >= route.size()) return;

    Ipv4Address nextHop = route[idx];
    sh.SetRouteIndex(idx + 1);

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(sh);
    uint8_t t = PKT_SWARM;
    Ptr<Packet> ctrl = Create<Packet>(&t, 1);
    ctrl->AddAtEnd(pkt);
    SendControlUnicast(ctrl, nextHop);
    m_txTrace(ctrl);
}

// ============================================================
// PERIODIC MAINTENANCE
// ============================================================

void BeeAdHocRoutingProtocol::PeriodicMaintenance() {
    m_danceFloor.Purge();
    CheckPackerQueue();
    CheckSwarmBalance();

    m_maintenanceTick++;

    // Re-flood hello scout every 10s
    if (m_maintenanceTick % 10 == 0) {
        FloodScout();
    }

    m_maintenanceTimer.Schedule(Seconds(1.0));
}

// ---- Scout deduplication ----

bool BeeAdHocRoutingProtocol::IsDuplicateScout(const ScoutId& id) {
    auto it = m_seenScouts.find(id);
    if (it == m_seenScouts.end()) return false;
    return (Simulator::Now() - it->second) < Seconds(10.0);
}

void BeeAdHocRoutingProtocol::MarkSeen(const ScoutId& id) {
    m_seenScouts[id] = Simulator::Now();
    Time now = Simulator::Now();
    for (auto it = m_seenScouts.begin(); it != m_seenScouts.end(); ) {
        if ((now - it->second) > Seconds(20.0))
            it = m_seenScouts.erase(it);
        else ++it;
    }
}

// ---- Interface stubs ----
void BeeAdHocRoutingProtocol::NotifyInterfaceUp(uint32_t i)   {}
void BeeAdHocRoutingProtocol::NotifyInterfaceDown(uint32_t i) {}
void BeeAdHocRoutingProtocol::NotifyAddAddress(uint32_t i, Ipv4InterfaceAddress a) {}
void BeeAdHocRoutingProtocol::NotifyRemoveAddress(uint32_t i, Ipv4InterfaceAddress a) {}

void BeeAdHocRoutingProtocol::PrintRoutingTable(
    Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    *stream->GetStream()
        << "BeeAdHoc @ " << GetLocalAddress()
        << " t=" << Simulator::Now().As(unit) << "\n";
}

} // namespace beeadhoc
} // namespace ns3
