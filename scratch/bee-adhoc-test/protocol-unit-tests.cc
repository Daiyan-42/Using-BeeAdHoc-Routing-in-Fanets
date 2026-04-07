/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * protocol-unit-tests.cc
 *
 * Focused tests for BeeAdHocRoutingProtocol methods.
 *
 * Note:
 *   GetNodeId() and DebugCheckpoint() are declared in bee-adhoc.h but do not
 *   currently have implementations in bee-adhoc.cc, so they are not callable
 *   from this test target.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/energy-module.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#include "bee-adhoc.h"
#undef private
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

using namespace ns3;
using namespace ns3::beeadhoc;

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, msg)                                                         \
    do                                                                           \
    {                                                                            \
        if (cond)                                                                \
        {                                                                        \
            std::cout << "  [PASS] " << msg << "\n";                             \
            ++g_passed;                                                          \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            std::cout << "  [FAIL] " << msg << "\n";                             \
            ++g_failed;                                                          \
        }                                                                        \
    } while (0)

#define CHECK_EQ(a, b, msg)                                                      \
    do                                                                           \
    {                                                                            \
        auto _a = (a);                                                           \
        auto _b = (b);                                                           \
        if (_a == _b)                                                            \
        {                                                                        \
            std::cout << "  [PASS] " << msg << "\n";                             \
            ++g_passed;                                                          \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            std::cout << "  [FAIL] " << msg << " (got=" << _a << " want=" << _b \
                      << ")\n";                                                  \
            ++g_failed;                                                          \
        }                                                                        \
    } while (0)

#define CHECK_NEAR(a, b, tol, msg)                                               \
    do                                                                           \
    {                                                                            \
        double _a = (double)(a);                                                 \
        double _b = (double)(b);                                                 \
        double _d = std::abs(_a - _b);                                           \
        if (_d <= (tol))                                                         \
        {                                                                        \
            std::cout << "  [PASS] " << msg << "\n";                             \
            ++g_passed;                                                          \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            std::cout << "  [FAIL] " << msg << " (got=" << _a << " want=" << _b \
                      << " diff=" << _d << ")\n";                                \
            ++g_failed;                                                          \
        }                                                                        \
    } while (0)

static void
Begin(const std::string& name)
{
    std::cout << "\n-- " << name << " --\n";
}

/**
 * SocketRecorder is a minimal packet sink used by tests that need to observe
 * what the protocol sent over UDP. It records how many packets arrived, the
 * total byte count, and the first byte of the payload, which this protocol
 * uses as its packet-type discriminator.
 *
 * This lets the tests verify behavior indirectly but realistically:
 * - a packet arrived at all
 * - the control plane chose the correct control-packet type
 * - the send path used the actual socket/network machinery instead of only
 *   mutating internal state
 */
struct SocketRecorder
{
    uint32_t packets{0};
    uint32_t bytes{0};
    uint8_t lastType{0};

    void Reset()
    {
        packets = 0;
        bytes = 0;
        lastType = 0;
    }

    void Recv(Ptr<Socket> socket)
    {
        Address from;
        while (Ptr<Packet> p = socket->RecvFrom(from))
        {
            packets++;
            bytes += p->GetSize();
            if (p->GetSize() > 0)
            {
                uint8_t typeBuf[1];
                p->CopyData(typeBuf, 1);
                lastType = typeBuf[0];
            }
        }
    }
};

/**
 * CallbackRecorder captures the three routing callbacks that ns-3 gives to
 * RouteInput/PackingFloorReceive:
 * - Ucb: unicast forward callback
 * - Lcb: local deliver callback
 * - Ecb: error callback
 *
 * The tests use it to prove which routing branch executed. For example, a
 * local packet should increment lcbCalls, a routable transit packet should
 * increment ucbCalls and capture a concrete route, and an expired buffered
 * packet should hit ecbCalls with a no-route style error.
 */
struct CallbackRecorder
{
    uint32_t ucbCalls{0};
    uint32_t lcbCalls{0};
    uint32_t ecbCalls{0};
    uint32_t lastIif{0};
    Socket::SocketErrno lastErr{Socket::ERROR_NOTERROR};
    Ptr<Ipv4Route> lastRoute;

    void Reset()
    {
        ucbCalls = 0;
        lcbCalls = 0;
        ecbCalls = 0;
        lastIif = 0;
        lastErr = Socket::ERROR_NOTERROR;
        lastRoute = nullptr;
    }

    void Ucb(Ptr<Ipv4Route> rt, Ptr<const Packet>, const Ipv4Header&)
    {
        ucbCalls++;
        lastRoute = rt;
    }

    void Lcb(Ptr<const Packet>, const Ipv4Header&, uint32_t iif)
    {
        lcbCalls++;
        lastIif = iif;
    }

    void Ecb(Ptr<const Packet>, const Ipv4Header&, Socket::SocketErrno err)
    {
        ecbCalls++;
        lastErr = err;
    }
};

/**
 * Fixture builds a tiny two-node network with real ns-3 objects:
 * - two nodes
 * - one shared SimpleChannel
 * - one SimpleNetDevice per node
 * - IPv4 installed with addresses on the same subnet
 * - one BeeAdHocRoutingProtocol instance bound to one of the nodes
 *
 * This is intentionally small but realistic. It gives the protocol a real
 * Ipv4 object, real interfaces, and a reachable peer so tests can exercise
 * socket send/receive behavior and route construction without needing a large
 * simulation topology.
 */
struct Fixture
{
    NodeContainer nodes;
    NetDeviceContainer devices;
    Ipv4InterfaceContainer ifaces;
    Ptr<BeeAdHocRoutingProtocol> proto;
    uint32_t protoIndex;

    explicit Fixture(uint32_t index = 0, bool start = false)
        : protoIndex(index)
    {
        nodes.Create(2);

        Ptr<SimpleChannel> channel = CreateObject<SimpleChannel>();

        Ptr<SimpleNetDevice> dev0 = CreateObject<SimpleNetDevice>();
        dev0->SetAddress(Mac48Address::Allocate());
        dev0->SetChannel(channel);
        nodes.Get(0)->AddDevice(dev0);

        Ptr<SimpleNetDevice> dev1 = CreateObject<SimpleNetDevice>();
        dev1->SetAddress(Mac48Address::Allocate());
        dev1->SetChannel(channel);
        nodes.Get(1)->AddDevice(dev1);

        devices.Add(dev0);
        devices.Add(dev1);

        InternetStackHelper internet;
        internet.Install(nodes);

        Ipv4AddressHelper address;
        address.SetBase("10.1.1.0", "255.255.255.0");
        ifaces = address.Assign(devices);

        proto = CreateObject<BeeAdHocRoutingProtocol>();
        proto->m_ipv4 = nodes.Get(protoIndex)->GetObject<Ipv4>();

        if (start)
        {
            proto->Start();
        }
    }

    Ptr<Node> ProtoNode() const
    {
        return nodes.Get(protoIndex);
    }

    Ptr<Node> PeerNode() const
    {
        return nodes.Get(1 - protoIndex);
    }

    Ptr<NetDevice> ProtoDevice() const
    {
        return devices.Get(protoIndex);
    }

    Ipv4Address ProtoAddress() const
    {
        return ifaces.GetAddress(protoIndex);
    }

    Ipv4Address PeerAddress() const
    {
        return ifaces.GetAddress(1 - protoIndex);
    }
};

/**
 * BindRecorder creates a UDP socket on a node and connects its receive
 * callback to a SocketRecorder. Tests use this as a passive observer on the
 * peer node so they can assert that a control packet was actually emitted and
 * inspect its packet type after the simulator runs briefly.
 */
static Ptr<Socket>
BindRecorder(Ptr<Node> node, uint16_t port, SocketRecorder& recorder)
{
    Ptr<Socket> socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
    socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), port));
    socket->SetAllowBroadcast(true);
    socket->SetRecvCallback(MakeCallback(&SocketRecorder::Recv, &recorder));
    return socket;
}

/**
 * SendControlPacket injects a protocol control packet into RecvEntrance using
 * the real UDP path. The helper serializes the requested BeeAdHoc header,
 * prefixes the 1-byte packet type expected by the protocol, and sends the
 * result to the destination/port under test.
 *
 * This is used for dispatch tests where we want to verify the top-level
 * receive path, not merely call an internal handler directly.
 */
template <typename HeaderType>
static void
SendControlPacket(Ptr<Node> sender, Ipv4Address dst, uint16_t port, uint8_t type, const HeaderType& header)
{
    Ptr<Socket> socket = Socket::CreateSocket(sender, UdpSocketFactory::GetTypeId());
    socket->SetAllowBroadcast(true);

    Ptr<Packet> payload = Create<Packet>();
    payload->AddHeader(header);
    Ptr<Packet> packet = Create<Packet>(&type, 1);
    packet->AddAtEnd(payload);

    socket->SendTo(packet, 0, InetSocketAddress(dst, port));
}

/**
 * RunBrief advances the simulator just long enough for immediately scheduled
 * asynchronous work to happen: socket delivery, callbacks, and short deferred
 * protocol actions. Many protocol methods schedule work rather than completing
 * synchronously, so tests call this after triggering an action and before
 * checking the externally visible result.
 */
static void
RunBrief(Time duration = MilliSeconds(50))
{
    Simulator::Stop(duration);
    Simulator::Run();
}

/**
 * MakeForager creates a consistent three-hop ForagerEntry:
 * source -> next hop -> destination.
 *
 * Most tests do not care about full scout discovery; they only need a valid
 * forager entry so the protocol can exercise its "route already known" paths.
 * This helper keeps those tests compact while still providing all fields that
 * DanceFloor and the routing logic expect.
 */
static ForagerEntry
MakeForager(Ipv4Address src, Ipv4Address via, Ipv4Address dst, double quality, uint32_t danceNum)
{
    ForagerEntry fe;
    fe.dst = dst;
    fe.route = {src, via, dst};
    fe.type = FORAGER_LIFETIME;
    fe.quality = quality;
    fe.danceNum = danceNum;
    fe.createdAt = Simulator::Now();
    fe.lifetime = Seconds(30);
    return fe;
}

/**
 * Verifies constructor and TypeId defaults without any network setup.
 *
 * What it checks:
 * - the registered ns-3 TypeId name is correct
 * - constructor/member defaults match the intended protocol configuration
 * - no socket exists before Start()
 * - the initial sequence counter starts at zero
 *
 * Why it matters:
 * these defaults drive scout TTL, packer timeout, swarm behavior, and routing
 * startup. If they drift, many later behaviors change in subtle ways.
 */
static void
TestProtocolDefaultsAndTypeId()
{
    Begin("Protocol defaults and TypeId");

    TypeId tid = BeeAdHocRoutingProtocol::GetTypeId();
    Ptr<BeeAdHocRoutingProtocol> proto = CreateObject<BeeAdHocRoutingProtocol>();

    CHECK_EQ(tid.GetName(), std::string("ns3::beeadhoc::BeeAdHocRoutingProtocol"), "TypeId name matches");
    CHECK_EQ((int)proto->m_initialTtl, 20, "default initial TTL is 20");
    CHECK_NEAR(proto->m_packerTimeout.GetSeconds(), 15.0, 1e-9, "default packer timeout is 15s");
    CHECK_NEAR(proto->m_energyThreshold, 0.5, 1e-9, "default energy threshold is 0.5");
    CHECK_EQ(proto->m_swarmThreshold, 3u, "default swarm threshold is 3");
    CHECK(proto->m_socket == nullptr, "socket is null before Start");
    CHECK_EQ(proto->m_seqno, 0u, "initial sequence number is 0");
}

/**
 * Verifies that SetIpv4 performs two jobs:
 * - stores the Ipv4 pointer immediately
 * - schedules Start() for later rather than starting synchronously
 *
 * The test creates a real fixture, calls SetIpv4, runs the simulator past the
 * scheduled start time, and then proves that startup side effects occurred:
 * socket creation, maintenance timer scheduling, and a valid local address.
 */
static void
TestSetIpv4SchedulesStart()
{
    Begin("SetIpv4 schedules Start");

    Fixture f(0, false);
    f.proto->SetIpv4(f.nodes.Get(0)->GetObject<Ipv4>());

    Simulator::Stop(Seconds(1.1));
    Simulator::Run();

    CHECK(f.proto->m_ipv4 == f.nodes.Get(0)->GetObject<Ipv4>(), "SetIpv4 stores the Ipv4 pointer");
    CHECK(f.proto->m_socket != nullptr, "Start created the socket after the scheduled delay");
    CHECK(f.proto->m_maintenanceTimer.IsRunning(), "maintenance timer is running after Start");
    CHECK_EQ(f.proto->GetLocalAddress(), f.ProtoAddress(), "GetLocalAddress returns the node address after Start");

    Simulator::Destroy();
}

/**
 * Exercises helper-style protocol methods that do not require multi-step
 * message exchange.
 *
 * What it covers:
 * - GetLocalAddress returns the node's assigned IPv4 address
 * - IsMyAddress distinguishes local vs non-local addresses
 * - GetResidualEnergy uses the default fallback and then the attached energy
 *   source once one is installed
 * - BuildRoute constructs a concrete ns-3 route when a usable device exists
 * - BuildRoute returns null when no non-loopback device exists
 * - PrintRoutingTable emits readable content containing the local address
 *
 * This test is mostly about proving the protocol's utility functions behave
 * sensibly before more complex message-flow tests depend on them.
 */
static void
TestHelperMethodsAndPrint()
{
    Begin("Helper methods and PrintRoutingTable");

    Fixture f(0, true);

    CHECK_EQ(f.proto->GetLocalAddress(), f.ProtoAddress(), "GetLocalAddress returns the protocol node address");
    CHECK(f.proto->IsMyAddress(f.ProtoAddress()), "IsMyAddress is true for local address");
    CHECK(!f.proto->IsMyAddress(f.PeerAddress()), "IsMyAddress is false for peer address");
    CHECK_NEAR(f.proto->GetResidualEnergy(), 100.0, 1e-9, "GetResidualEnergy falls back to 100 J without energy model");

    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(55.0));
    energyHelper.Install(f.ProtoNode());
    CHECK_NEAR(f.proto->GetResidualEnergy(), 55.0, 1e-6, "GetResidualEnergy reads the attached energy source");

    Ptr<Ipv4Route> rt = f.proto->BuildRoute(f.PeerAddress(), f.PeerAddress());
    CHECK(rt != nullptr, "BuildRoute returns a route when a data device exists");
    CHECK_EQ(rt->GetDestination(), f.PeerAddress(), "BuildRoute sets destination");
    CHECK_EQ(rt->GetGateway(), f.PeerAddress(), "BuildRoute sets gateway");
    CHECK_EQ(rt->GetSource(), f.ProtoAddress(), "BuildRoute sets source");

    Ptr<BeeAdHocRoutingProtocol> emptyProto = CreateObject<BeeAdHocRoutingProtocol>();
    emptyProto->m_ipv4 = CreateObject<Ipv4L3Protocol>();
    CHECK(emptyProto->BuildRoute(Ipv4Address("10.0.0.1"), Ipv4Address("10.0.0.2")) == nullptr,
          "BuildRoute returns null when no non-loopback device exists");

    std::ostringstream oss;
    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&oss);
    f.proto->PrintRoutingTable(stream, Time::S);
    std::ostringstream addr;
    addr << f.ProtoAddress();
    CHECK(oss.str().find(addr.str()) != std::string::npos,
          "PrintRoutingTable includes the local address");

    Simulator::Destroy();
}

/**
 * Validates the duplicate-scout bookkeeping and the no-op notify methods.
 *
 * The duplicate logic is checked by observing the full lifecycle of one scout
 * ID:
 * - unseen scout is not duplicate
 * - once marked seen it is duplicate
 * - after the 10-second retention window it stops being duplicate
 *
 * The notify callbacks are invoked simply to confirm they are callable and do
 * not crash, since they currently act as protocol stubs.
 */
static void
TestSeenScoutHelpersAndNotifyStubs()
{
    Begin("Seen-scout helpers and notify stubs");

    Ptr<BeeAdHocRoutingProtocol> proto = CreateObject<BeeAdHocRoutingProtocol>();
    ScoutId id;
    id.src = Ipv4Address("10.0.0.2");
    id.seqno = 7;

    CHECK(!proto->IsDuplicateScout(id), "unknown scout is not a duplicate");
    proto->MarkSeen(id);
    CHECK(proto->IsDuplicateScout(id), "freshly seen scout is detected as duplicate");

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();
    CHECK(!proto->IsDuplicateScout(id), "duplicate window expires after 10 seconds");

    proto->NotifyInterfaceUp(1);
    proto->NotifyInterfaceDown(1);
    proto->NotifyAddAddress(1, Ipv4InterfaceAddress(Ipv4Address("10.0.0.1"), Ipv4Mask("/24")));
    proto->NotifyRemoveAddress(1, Ipv4InterfaceAddress(Ipv4Address("10.0.0.1"), Ipv4Mask("/24")));
    CHECK(true, "notify stubs are callable");

    Simulator::Destroy();
}

/**
 * Tests two direct control-packet transmit paths:
 * - SendControlUnicast sends one packet directly to a known next hop
 * - FloodScout emits a broadcast forward-scout packet
 *
 * A recorder socket on the peer node gives evidence of actual packet delivery,
 * and the recorded leading type byte proves the protocol sent the expected
 * control-packet kind rather than some other message.
 */
static void
TestSendControlUnicastAndFloodScout()
{
    Begin("SendControlUnicast and FloodScout");

    Fixture f(0, true);
    SocketRecorder recorder;
    Ptr<Socket> sink = BindRecorder(f.PeerNode(), f.proto->m_port, recorder);
    (void)sink;

    recorder.Reset();
    f.proto->SendControlUnicast(Create<Packet>(8), f.PeerAddress());
    RunBrief();
    CHECK_EQ(recorder.packets, 1u, "SendControlUnicast delivers a packet to the peer");

    recorder.Reset();
    f.proto->FloodScout();
    RunBrief();
    CHECK(recorder.packets >= 1, "FloodScout emits at least one broadcast control packet");
    CHECK_EQ((int)recorder.lastType, (int)PKT_FORWARD_SCOUT, "FloodScout sends a forward scout");

    Simulator::Destroy();
}

/**
 * Covers RouteOutput in its main branches:
 * - broadcast traffic gets an immediate route
 * - unicast with a forager is routed using the stored next hop
 * - unicast without a forager is buffered and scout discovery is triggered
 *
 * This test also checks the side effects that define correctness here:
 * - dance count is consumed on a route hit
 * - a forager probe is sent after a hit
 * - a missed destination is queued on the packing floor
 * - scout discovery is marked pending and a forward scout is launched
 */
static void
TestRouteOutputPaths()
{
    Begin("RouteOutput normal and miss cases");

    Fixture f(0, true);
    SocketRecorder recorder;
    Ptr<Socket> sink = BindRecorder(f.PeerNode(), f.proto->m_port, recorder);
    (void)sink;

    Ipv4Header bcast;
    bcast.SetSource(f.ProtoAddress());
    bcast.SetDestination(Ipv4Address("255.255.255.255"));
    Socket::SocketErrno err = Socket::ERROR_NOTERROR;
    Ptr<Ipv4Route> bcastRoute = f.proto->RouteOutput(Create<Packet>(4), bcast, nullptr, err);
    CHECK(bcastRoute != nullptr, "RouteOutput returns a route for broadcast traffic");
    CHECK_EQ(err, Socket::ERROR_NOTERROR, "RouteOutput broadcast path sets no error");

    f.proto->m_danceFloor.AddForager(MakeForager(f.ProtoAddress(), f.PeerAddress(), f.PeerAddress(), 80.0, 2));
    Ipv4Header unicast;
    unicast.SetSource(f.ProtoAddress());
    unicast.SetDestination(f.PeerAddress());

    recorder.Reset();
    Ptr<Ipv4Route> hitRoute = f.proto->RouteOutput(Create<Packet>(20), unicast, nullptr, err);
    CHECK(hitRoute != nullptr, "RouteOutput returns a route when a forager exists");
    CHECK_EQ(hitRoute->GetGateway(), f.PeerAddress(), "RouteOutput uses route[1] as next hop");
    CHECK_EQ(f.proto->m_danceFloor.CountForagers(f.PeerAddress()), 1u, "RouteOutput consumes one dance count");
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_FORAGER, "RouteOutput hit sends a forager probe");

    Ipv4Address missDst("10.9.9.9");
    Ipv4Header missHdr;
    missHdr.SetSource(f.ProtoAddress());
    missHdr.SetDestination(missDst);

    recorder.Reset();
    Ptr<Ipv4Route> missRoute = f.proto->RouteOutput(Create<Packet>(12), missHdr, nullptr, err);
    CHECK(missRoute == nullptr, "RouteOutput miss returns null");
    CHECK_EQ(err, Socket::ERROR_NOROUTETOHOST, "RouteOutput miss reports no route");
    CHECK_EQ((int)f.proto->m_packerQueue[missDst].size(), 1, "RouteOutput miss buffers the packet");
    CHECK(f.proto->m_scoutPending.count(missDst) == 1, "RouteOutput miss marks scout discovery as pending");
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_FORWARD_SCOUT, "RouteOutput miss launches a forward scout");

    Simulator::Destroy();
}

/**
 * Tests RouteInput together with PackingFloorReceive, because the transit-miss
 * path of RouteInput delegates to the packing floor.
 *
 * Cases covered:
 * - local unicast: must deliver via lcb
 * - broadcast: must deliver via lcb
 * - transit hit: must forward via ucb and provide a concrete route
 * - transit miss: must buffer rather than drop
 * - packing floor with forager: forwards immediately
 * - packing floor without forager: queues packet and starts scout discovery
 *
 * Using CallbackRecorder makes each branch visible through the callback type
 * the protocol selects.
 */
static void
TestRouteInputAndPackingFloor()
{
    Begin("RouteInput and PackingFloorReceive");

    Fixture f(0, true);
    CallbackRecorder cb;

    Ipv4Header localHdr;
    localHdr.SetSource(f.PeerAddress());
    localHdr.SetDestination(f.ProtoAddress());
    bool handled = f.proto->RouteInput(Create<Packet>(5),
                                       localHdr,
                                       f.ProtoDevice(),
                                       MakeCallback(&CallbackRecorder::Ucb, &cb),
                                       Ipv4RoutingProtocol::MulticastForwardCallback(),
                                       MakeCallback(&CallbackRecorder::Lcb, &cb),
                                       MakeCallback(&CallbackRecorder::Ecb, &cb));
    CHECK(handled, "RouteInput handles local unicast");
    CHECK_EQ(cb.lcbCalls, 1u, "RouteInput local path delivers via lcb");

    cb.Reset();
    Ipv4Header bcastHdr;
    bcastHdr.SetSource(f.PeerAddress());
    bcastHdr.SetDestination(Ipv4Address("255.255.255.255"));
    handled = f.proto->RouteInput(Create<Packet>(5),
                                  bcastHdr,
                                  f.ProtoDevice(),
                                  MakeCallback(&CallbackRecorder::Ucb, &cb),
                                  Ipv4RoutingProtocol::MulticastForwardCallback(),
                                  MakeCallback(&CallbackRecorder::Lcb, &cb),
                                  MakeCallback(&CallbackRecorder::Ecb, &cb));
    CHECK(handled, "RouteInput handles broadcast");
    CHECK_EQ(cb.lcbCalls, 1u, "RouteInput broadcast path delivers via lcb");

    cb.Reset();
    Ipv4Address remoteDst("10.1.1.88");
    f.proto->m_danceFloor.AddForager(MakeForager(f.ProtoAddress(), f.PeerAddress(), remoteDst, 75.0, 2));
    Ipv4Header transitHdr;
    transitHdr.SetSource(Ipv4Address("10.1.1.77"));
    transitHdr.SetDestination(remoteDst);
    handled = f.proto->RouteInput(Create<Packet>(9),
                                  transitHdr,
                                  f.ProtoDevice(),
                                  MakeCallback(&CallbackRecorder::Ucb, &cb),
                                  Ipv4RoutingProtocol::MulticastForwardCallback(),
                                  MakeCallback(&CallbackRecorder::Lcb, &cb),
                                  MakeCallback(&CallbackRecorder::Ecb, &cb));
    CHECK(handled, "RouteInput handles transit traffic");
    CHECK_EQ(cb.ucbCalls, 1u, "RouteInput transit hit forwards via ucb");
    CHECK(cb.lastRoute != nullptr, "RouteInput transit hit builds a route");
    CHECK_EQ(cb.lastRoute->GetGateway(), f.PeerAddress(), "RouteInput transit hit uses the first hop");

    cb.Reset();
    Ipv4Address missDst("10.1.1.120");
    Ipv4Header transitMissHdr;
    transitMissHdr.SetSource(Ipv4Address("10.1.1.77"));
    transitMissHdr.SetDestination(missDst);
    handled = f.proto->RouteInput(Create<Packet>(11),
                                  transitMissHdr,
                                  f.ProtoDevice(),
                                  MakeCallback(&CallbackRecorder::Ucb, &cb),
                                  Ipv4RoutingProtocol::MulticastForwardCallback(),
                                  MakeCallback(&CallbackRecorder::Lcb, &cb),
                                  MakeCallback(&CallbackRecorder::Ecb, &cb));
    CHECK(handled, "RouteInput miss still returns true after buffering");
    CHECK_EQ((int)f.proto->m_packerQueue[missDst].size(), 1, "RouteInput miss buffers via PackingFloorReceive");

    cb.Reset();
    Ipv4Address directDst("10.1.1.121");
    f.proto->m_danceFloor.AddForager(MakeForager(f.ProtoAddress(), f.PeerAddress(), directDst, 90.0, 1));
    Ipv4Header packHdr;
    packHdr.SetSource(f.ProtoAddress());
    packHdr.SetDestination(directDst);
    f.proto->PackingFloorReceive(Create<Packet>(7),
                                 packHdr,
                                 MakeCallback(&CallbackRecorder::Ucb, &cb),
                                 MakeCallback(&CallbackRecorder::Ecb, &cb));
    CHECK_EQ(cb.ucbCalls, 1u, "PackingFloorReceive forwards immediately when a forager exists");

    cb.Reset();
    Ipv4Address queuedDst("10.1.1.122");
    packHdr.SetDestination(queuedDst);
    f.proto->PackingFloorReceive(Create<Packet>(7),
                                 packHdr,
                                 MakeCallback(&CallbackRecorder::Ucb, &cb),
                                 MakeCallback(&CallbackRecorder::Ecb, &cb));
    CHECK_EQ((int)f.proto->m_packerQueue[queuedDst].size(), 1, "PackingFloorReceive queues packets when no route exists");
    CHECK(f.proto->m_scoutPending.count(queuedDst) == 1, "PackingFloorReceive schedules a scout when queueing");

    Simulator::Destroy();
}

/**
 * Verifies the two queue-maintenance operations for buffered data packets.
 *
 * DrainPackerQueue is checked by enqueueing two packets, adding exactly enough
 * dance count to send both, and confirming:
 * - both packets are forwarded
 * - the queue entry is removed afterward
 * - the destination is removed from the pending-scout set
 *
 * CheckPackerQueue is then checked in two independent scenarios:
 * - expired buffered packet triggers ecb and queue cleanup
 * - still-valid buffered packet drains once a forager becomes available
 */
static void
TestDrainAndCheckPackerQueue()
{
    Begin("DrainPackerQueue and CheckPackerQueue");

    Fixture f(0, true);
    CallbackRecorder cb;
    Ipv4Address dst("10.1.1.130");
    Ipv4Header hdr;
    hdr.SetSource(f.ProtoAddress());
    hdr.SetDestination(dst);

    f.proto->PackingFloorReceive(Create<Packet>(10),
                                 hdr,
                                 MakeCallback(&CallbackRecorder::Ucb, &cb),
                                 MakeCallback(&CallbackRecorder::Ecb, &cb));
    f.proto->PackingFloorReceive(Create<Packet>(10),
                                 hdr,
                                 MakeCallback(&CallbackRecorder::Ucb, &cb),
                                 MakeCallback(&CallbackRecorder::Ecb, &cb));
    f.proto->m_danceFloor.AddForager(MakeForager(f.ProtoAddress(), f.PeerAddress(), dst, 85.0, 2));
    f.proto->DrainPackerQueue(dst);
    CHECK_EQ(cb.ucbCalls, 2u, "DrainPackerQueue forwards every queued packet while dances remain");
    CHECK(f.proto->m_packerQueue.count(dst) == 0, "DrainPackerQueue erases the queue after draining");
    CHECK(f.proto->m_scoutPending.count(dst) == 0, "DrainPackerQueue clears pending scout state");

    cb.Reset();
    Ipv4Address expiredDst("10.1.1.131");
    PackerEntry expired;
    expired.packet = Create<Packet>(5);
    expired.ipHdr = hdr;
    expired.ipHdr.SetDestination(expiredDst);
    expired.createdAt = Seconds(-5);
    expired.waitTimeout = Seconds(1);
    expired.ecb = MakeCallback(&CallbackRecorder::Ecb, &cb);
    f.proto->m_packerQueue[expiredDst].push_back(expired);
    f.proto->CheckPackerQueue();
    CHECK_EQ(cb.ecbCalls, 1u, "CheckPackerQueue drops expired packers through ecb");
    CHECK(f.proto->m_packerQueue.count(expiredDst) == 0, "CheckPackerQueue removes empty expired queues");

    cb.Reset();
    Ipv4Address drainDst("10.1.1.132");
    PackerEntry pending;
    pending.packet = Create<Packet>(5);
    pending.ipHdr = hdr;
    pending.ipHdr.SetDestination(drainDst);
    pending.createdAt = Simulator::Now();
    pending.waitTimeout = Seconds(10);
    pending.ucb = MakeCallback(&CallbackRecorder::Ucb, &cb);
    f.proto->m_packerQueue[drainDst].push_back(pending);
    f.proto->m_danceFloor.AddForager(MakeForager(f.ProtoAddress(), f.PeerAddress(), drainDst, 70.0, 1));
    f.proto->CheckPackerQueue();
    CHECK_EQ(cb.ucbCalls, 1u, "CheckPackerQueue drains queues when a forager is available");
    CHECK(f.proto->m_packerQueue.count(drainDst) == 0, "CheckPackerQueue removes drained queues");

    Simulator::Destroy();
}

/**
 * Exercises forward-scout creation and handling under normal and edge cases.
 *
 * LaunchForwardScout:
 * - should refuse to send when residual energy is below threshold
 * - should not mark a scout as seen when it was never launched
 * - should broadcast a valid forward scout once energy is sufficient
 *
 * ProcessForwardScout:
 * - duplicate scout returns immediately
 * - TTL=0 returns immediately
 * - low-energy node returns immediately
 * - when the current node is the destination, it appends itself, decrements
 *   TTL, and emits a backward scout
 * - when relaying, it appends itself, decrements TTL, and rebroadcasts
 *
 * The test checks both header mutation and transmitted packet type so it can
 * prove not only that a branch ran, but also that it updated protocol state
 * correctly.
 */
static void
TestLaunchAndProcessForwardScout()
{
    Begin("LaunchForwardScout and ProcessForwardScout");

    Fixture f(0, true);
    SocketRecorder recorder;
    Ptr<Socket> sink = BindRecorder(f.PeerNode(), f.proto->m_port, recorder);
    (void)sink;

    f.proto->m_energyThreshold = 150.0;
    f.proto->LaunchForwardScout(f.PeerAddress(), FORAGER_LIFETIME);
    RunBrief();
    CHECK_EQ(recorder.packets, 0u, "LaunchForwardScout drops when residual energy is below threshold");
    CHECK(f.proto->m_seenScouts.empty(), "LaunchForwardScout does not mark scouts seen on energy-gated drop");

    f.proto->m_energyThreshold = 0.5;
    recorder.Reset();
    f.proto->LaunchForwardScout(f.PeerAddress(), FORAGER_LIFETIME);
    RunBrief();
    CHECK(recorder.packets >= 1, "LaunchForwardScout broadcasts a forward scout");
    CHECK_EQ((int)recorder.lastType, (int)PKT_FORWARD_SCOUT, "LaunchForwardScout sends a forward scout packet");
    CHECK(!f.proto->m_seenScouts.empty(), "LaunchForwardScout marks the new scout as seen");

    ForwardScoutHeader dup;
    dup.SetSrc(f.PeerAddress());
    dup.SetDst(Ipv4Address("10.1.1.200"));
    dup.SetSeqno(77);
    dup.SetTtl(5);
    ScoutId sid;
    sid.src = dup.GetSrc();
    sid.seqno = dup.GetSeqno();
    f.proto->MarkSeen(sid);
    f.proto->ProcessForwardScout(Create<Packet>(), dup);
    CHECK_EQ((int)dup.GetHopCount(), 0, "ProcessForwardScout duplicate path returns before mutating header");

    ForwardScoutHeader ttl0;
    ttl0.SetSrc(f.PeerAddress());
    ttl0.SetDst(Ipv4Address("10.1.1.201"));
    ttl0.SetSeqno(78);
    ttl0.SetTtl(0);
    f.proto->ProcessForwardScout(Create<Packet>(), ttl0);
    CHECK_EQ((int)ttl0.GetHopCount(), 0, "ProcessForwardScout TTL-expired path returns before mutating header");

    f.proto->m_energyThreshold = 150.0;
    ForwardScoutHeader gated;
    gated.SetSrc(f.PeerAddress());
    gated.SetDst(Ipv4Address("10.1.1.202"));
    gated.SetSeqno(79);
    gated.SetTtl(5);
    f.proto->ProcessForwardScout(Create<Packet>(), gated);
    CHECK_EQ((int)gated.GetHopCount(), 0, "ProcessForwardScout energy gate returns before mutating header");
    f.proto->m_energyThreshold = 0.5;

    recorder.Reset();
    ForwardScoutHeader toDst;
    toDst.SetSrc(f.PeerAddress());
    toDst.SetDst(f.ProtoAddress());
    toDst.SetSeqno(80);
    toDst.SetTtl(5);
    toDst.SetTotalEnergy(10.0);
    toDst.AddHop(f.PeerAddress());
    f.proto->ProcessForwardScout(Create<Packet>(), toDst);
    RunBrief();
    CHECK_EQ((int)toDst.GetHopCount(), 2, "ProcessForwardScout adds the destination hop");
    CHECK_EQ((int)toDst.GetTtl(), 4, "ProcessForwardScout decrements TTL after acceptance");
    CHECK_EQ((int)recorder.lastType, (int)PKT_BACKWARD_SCOUT, "ProcessForwardScout at destination sends a backward scout");

    recorder.Reset();
    ForwardScoutHeader relay;
    relay.SetSrc(f.PeerAddress());
    relay.SetDst(Ipv4Address("10.1.1.250"));
    relay.SetSeqno(81);
    relay.SetTtl(5);
    relay.SetTotalEnergy(10.0);
    relay.AddHop(f.PeerAddress());
    f.proto->ProcessForwardScout(Create<Packet>(), relay);
    RunBrief();
    CHECK_EQ((int)relay.GetHopCount(), 2, "ProcessForwardScout relay path adds the local hop");
    CHECK_EQ((int)relay.GetTtl(), 4, "ProcessForwardScout relay path decrements TTL");
    CHECK_EQ((int)recorder.lastType, (int)PKT_FORWARD_SCOUT, "ProcessForwardScout relay path rebroadcasts the scout");

    Simulator::Destroy();
}

/**
 * Covers backward-scout generation and processing.
 *
 * SendBackwardScout is verified by checking that a backward-scout packet is
 * emitted on the wire.
 *
 * ProcessBackwardScout is verified in three cases:
 * - when the scout reaches its source, a forager is installed in DanceFloor
 *   and its dance number follows the capped formula used by the protocol
 * - when an intermediate node receives it, the scout is forwarded again
 * - malformed intermediate packets with routeIndex <= 0 are dropped
 */
static void
TestBackwardScoutHandlers()
{
    Begin("SendBackwardScout and ProcessBackwardScout");

    Fixture f(0, true);
    SocketRecorder recorder;
    Ptr<Socket> sink = BindRecorder(f.PeerNode(), f.proto->m_port, recorder);
    (void)sink;

    ForwardScoutHeader fsh;
    fsh.SetSrc(f.PeerAddress());
    fsh.SetDst(f.ProtoAddress());
    fsh.SetSeqno(90);
    fsh.SetTotalEnergy(140.0);
    fsh.SetHopCount(2);
    fsh.SetType(FORAGER_LIFETIME);
    fsh.SetRoute({f.PeerAddress(), f.ProtoAddress()});

    recorder.Reset();
    f.proto->SendBackwardScout(fsh);
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_BACKWARD_SCOUT, "SendBackwardScout unicasts a backward scout");

    BackwardScoutHeader atSource;
    atSource.SetSrc(f.ProtoAddress());
    atSource.SetDst(f.PeerAddress());
    atSource.SetSeqno(91);
    atSource.SetAvgEnergy(80.0);
    atSource.SetHopCount(2);
    atSource.SetRouteIndex(0);
    atSource.SetType(FORAGER_LIFETIME);
    atSource.SetRoute({f.ProtoAddress(), f.PeerAddress()});
    f.proto->ProcessBackwardScout(Create<Packet>(), atSource);
    CHECK(f.proto->m_danceFloor.HasForager(f.PeerAddress()), "ProcessBackwardScout installs a forager when the source receives it");
    CHECK_EQ(f.proto->m_danceFloor.CountForagers(f.PeerAddress()), 5u, "installed dance number follows the capped formula");

    recorder.Reset();
    BackwardScoutHeader relay;
    relay.SetSrc(f.PeerAddress());
    relay.SetDst(Ipv4Address("10.1.1.251"));
    relay.SetSeqno(92);
    relay.SetAvgEnergy(70.0);
    relay.SetHopCount(3);
    relay.SetRouteIndex(1);
    relay.SetType(FORAGER_LIFETIME);
    relay.SetRoute({f.PeerAddress(), f.ProtoAddress(), Ipv4Address("10.1.1.251")});
    f.proto->ProcessBackwardScout(Create<Packet>(), relay);
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_BACKWARD_SCOUT, "ProcessBackwardScout relay path forwards the scout");

    recorder.Reset();
    BackwardScoutHeader malformed = relay;
    malformed.SetRouteIndex(0);
    f.proto->ProcessBackwardScout(Create<Packet>(), malformed);
    RunBrief();
    CHECK_EQ(recorder.packets, 0u, "ProcessBackwardScout drops idx<=0 packets at non-source nodes");

    Simulator::Destroy();
}

/**
 * Tests the forager probe workflow.
 *
 * SendForager should:
 * - send a forager control packet
 * - increment the outgoing forager balance for the destination
 *
 * ProcessForager should:
 * - at an intermediate node, advance routeIndex, increment hop count, and
 *   forward the probe
 * - at the destination, stop forwarding and emit a swarm response instead
 */
static void
TestForagerHandlers()
{
    Begin("SendForager and ProcessForager");

    Fixture f(0, true);
    SocketRecorder recorder;
    Ptr<Socket> sink = BindRecorder(f.PeerNode(), f.proto->m_port, recorder);
    (void)sink;

    recorder.Reset();
    ForagerEntry fe = MakeForager(f.ProtoAddress(), f.PeerAddress(), f.PeerAddress(), 80.0, 1);
    f.proto->SendForager(fe);
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_FORAGER, "SendForager sends a forager control packet");
    CHECK_EQ(f.proto->m_foragerBalance[f.PeerAddress()], 1, "SendForager increments the outgoing balance");

    recorder.Reset();
    ForagerHeader transit;
    transit.SetSrc(Ipv4Address("10.1.1.10"));
    transit.SetDst(Ipv4Address("10.1.1.20"));
    transit.SetRoute({Ipv4Address("10.1.1.10"), f.ProtoAddress(), f.PeerAddress()});
    transit.SetRouteIndex(1);
    transit.SetAccumEnergy(10.0);
    transit.SetHopCount(1);
    transit.SetType(FORAGER_LIFETIME);
    f.proto->ProcessForager(Create<Packet>(), transit);
    RunBrief();
    CHECK_EQ((int)transit.GetRouteIndex(), 2, "ProcessForager transit path advances the route index");
    CHECK_EQ((int)transit.GetHopCount(), 2, "ProcessForager transit path increments hop count");
    CHECK_EQ((int)recorder.lastType, (int)PKT_FORAGER, "ProcessForager transit path forwards the probe");

    recorder.Reset();
    ForagerHeader atDst;
    atDst.SetSrc(f.PeerAddress());
    atDst.SetDst(f.ProtoAddress());
    atDst.SetRoute({f.PeerAddress(), f.ProtoAddress()});
    atDst.SetRouteIndex(1);
    atDst.SetAccumEnergy(5.0);
    atDst.SetHopCount(1);
    atDst.SetType(FORAGER_LIFETIME);
    f.proto->ProcessForager(Create<Packet>(), atDst);
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_SWARM, "ProcessForager destination path sends a swarm back");

    Simulator::Destroy();
}

/**
 * Covers swarm packet behavior and the balance-based swarm trigger.
 *
 * The test verifies:
 * - SendSwarm emits a swarm control packet
 * - ProcessSwarm at the source installs/refreshes a forager using the swarm's
 *   foragerCount as the new dance number
 * - source-side swarm receipt decrements the stored balance
 * - transit swarm packets are forwarded
 * - CheckSwarmBalance emits a swarm once the threshold is exceeded and resets
 *   the tracked balance afterward
 */
static void
TestSwarmHandlersAndBalance()
{
    Begin("SendSwarm, ProcessSwarm, and CheckSwarmBalance");

    Fixture f(0, true);
    SocketRecorder recorder;
    Ptr<Socket> sink = BindRecorder(f.PeerNode(), f.proto->m_port, recorder);
    (void)sink;

    recorder.Reset();
    f.proto->SendSwarm(f.PeerAddress(), 2, 77.0, {f.ProtoAddress(), f.PeerAddress()});
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_SWARM, "SendSwarm sends a swarm control packet");

    f.proto->m_foragerBalance[f.PeerAddress()] = 2;
    SwarmHeader atSource;
    atSource.SetSrc(f.PeerAddress());
    atSource.SetDst(f.ProtoAddress());
    atSource.SetForagerCount(3);
    atSource.SetAvgEnergy(70.0);
    atSource.SetHopCount(2);
    atSource.SetRouteIndex(1);
    atSource.SetRoute({f.PeerAddress(), f.ProtoAddress()});
    f.proto->ProcessSwarm(Create<Packet>(), atSource);
    CHECK(f.proto->m_danceFloor.HasForager(f.PeerAddress()), "ProcessSwarm at source installs a forager");
    CHECK_EQ(f.proto->m_danceFloor.CountForagers(f.PeerAddress()), 3u, "ProcessSwarm copies foragerCount into danceNum");
    CHECK_EQ(f.proto->m_foragerBalance[f.PeerAddress()], 1, "ProcessSwarm decrements the inbound balance on source arrival");

    recorder.Reset();
    SwarmHeader relay;
    relay.SetSrc(Ipv4Address("10.1.1.40"));
    relay.SetDst(Ipv4Address("10.1.1.41"));
    relay.SetForagerCount(1);
    relay.SetAvgEnergy(60.0);
    relay.SetHopCount(3);
    relay.SetRouteIndex(2);
    relay.SetRoute({Ipv4Address("10.1.1.40"), f.ProtoAddress(), f.PeerAddress()});
    f.proto->ProcessSwarm(Create<Packet>(), relay);
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_SWARM, "ProcessSwarm transit path forwards the swarm");

    recorder.Reset();
    f.proto->m_foragerBalance[f.PeerAddress()] = 4;
    f.proto->CheckSwarmBalance();
    RunBrief();
    CHECK_EQ((int)recorder.lastType, (int)PKT_SWARM, "CheckSwarmBalance launches a swarm when the threshold is exceeded");
    CHECK_EQ(f.proto->m_foragerBalance[f.PeerAddress()], 0, "CheckSwarmBalance resets the balance after launching a swarm");

    Simulator::Destroy();
}

/**
 * Tests the top-level receive dispatcher RecvEntrance.
 *
 * Instead of calling ProcessBackwardScout directly, this test serializes a
 * backward-scout packet, injects it through a real UDP socket, runs the
 * simulator, and then checks the externally visible result: a forager was
 * installed. That proves packet decoding and dispatch selected the correct
 * handler.
 */
static void
TestRecvEntranceDispatch()
{
    Begin("RecvEntrance dispatch");

    Fixture f(0, true);

    BackwardScoutHeader bsh;
    bsh.SetSrc(f.ProtoAddress());
    bsh.SetDst(f.PeerAddress());
    bsh.SetSeqno(101);
    bsh.SetAvgEnergy(60.0);
    bsh.SetHopCount(2);
    bsh.SetRouteIndex(0);
    bsh.SetType(FORAGER_LIFETIME);
    bsh.SetRoute({f.ProtoAddress(), f.PeerAddress()});

    SendControlPacket(f.PeerNode(), f.ProtoAddress(), f.proto->m_port, PKT_BACKWARD_SCOUT, bsh);
    RunBrief();
    CHECK(f.proto->m_danceFloor.HasForager(f.PeerAddress()), "RecvEntrance dispatches backward scouts to ProcessBackwardScout");

    Simulator::Destroy();
}

/**
 * Verifies the recurring maintenance routine by seeding one example of each
 * kind of work it is responsible for:
 * - an expired forager in DanceFloor
 * - an expired buffered packet in the packing floor
 * - a forager balance above the swarm threshold
 *
 * After calling PeriodicMaintenance and letting the simulator process any
 * follow-up work, the test confirms that:
 * - expired foragers were purged
 * - expired buffered packets hit the error callback
 * - swarm balancing sent a swarm packet
 * - the maintenance tick counter advanced
 * - the timer was rescheduled for the next cycle
 */
static void
TestPeriodicMaintenance()
{
    Begin("PeriodicMaintenance");

    Fixture f(0, true);
    SocketRecorder recorder;
    Ptr<Socket> sink = BindRecorder(f.PeerNode(), f.proto->m_port, recorder);
    (void)sink;
    CallbackRecorder cb;

    ForagerEntry expired = MakeForager(f.ProtoAddress(), f.PeerAddress(), Ipv4Address("10.1.1.210"), 40.0, 1);
    expired.createdAt = Seconds(-5);
    expired.lifetime = Seconds(1);
    f.proto->m_danceFloor.AddForager(expired);

    PackerEntry pe;
    pe.packet = Create<Packet>(4);
    pe.ipHdr.SetSource(f.ProtoAddress());
    pe.ipHdr.SetDestination(Ipv4Address("10.1.1.211"));
    pe.createdAt = Seconds(-5);
    pe.waitTimeout = Seconds(1);
    pe.ecb = MakeCallback(&CallbackRecorder::Ecb, &cb);
    f.proto->m_packerQueue[Ipv4Address("10.1.1.211")].push_back(pe);

    f.proto->m_foragerBalance[f.PeerAddress()] = 4;

    recorder.Reset();
    f.proto->m_maintenanceTimer.Cancel();
    f.proto->PeriodicMaintenance();
    RunBrief();

    CHECK(!f.proto->m_danceFloor.HasForager(Ipv4Address("10.1.1.210")), "PeriodicMaintenance purges expired foragers");
    CHECK_EQ(cb.ecbCalls, 1u, "PeriodicMaintenance expires timed-out packers");
    CHECK_EQ((int)recorder.lastType, (int)PKT_SWARM, "PeriodicMaintenance checks swarm balance and sends a swarm");
    CHECK_EQ(f.proto->m_maintenanceTick, 1u, "PeriodicMaintenance increments the tick counter");
    CHECK(f.proto->m_maintenanceTimer.IsRunning(), "PeriodicMaintenance reschedules itself");

    Simulator::Destroy();
}

int
main(int argc, char* argv[])
{
    // Run the protocol tests in a fixed order from basic invariants to full
    // message-flow behavior. The final process exit code is non-zero if any
    // assertion failed, so the executable can be used directly in CI or via
    // `./ns3 run protocol-unit-tests`.
    std::cout << "=== BeeAdHocRoutingProtocol Unit Tests ===\n";

    TestProtocolDefaultsAndTypeId();
    TestSetIpv4SchedulesStart();
    TestHelperMethodsAndPrint();
    TestSeenScoutHelpersAndNotifyStubs();
    TestSendControlUnicastAndFloodScout();
    TestRouteOutputPaths();
    TestRouteInputAndPackingFloor();
    TestDrainAndCheckPackerQueue();
    TestLaunchAndProcessForwardScout();
    TestBackwardScoutHandlers();
    TestForagerHandlers();
    TestSwarmHandlersAndBalance();
    TestRecvEntranceDispatch();
    TestPeriodicMaintenance();

    std::cout << "\nTOTAL: " << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
