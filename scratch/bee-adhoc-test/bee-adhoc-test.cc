/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * bee-adhoc-test.cc
 * Tests ForwardScoutHeader, BackwardScoutHeader, ForagerHeader, SwarmHeader,
 * and basic DanceFloor behavior.
 */

#include "bee-adhoc.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"

#include <iostream>
#include <cmath>

using namespace ns3;
using namespace ns3::beeadhoc;

// ---- simple pass/fail macro ----
#define CHECK(cond, label) \
    do { \
        if (cond) std::cout << "[PASS] " << label << "\n"; \
        else      std::cout << "[FAIL] " << label << "\n"; \
    } while (0)

// ============================================================
// ForwardScoutHeader
// ============================================================
void TestForwardScoutHeader()
{
    std::cout << "\n-- ForwardScoutHeader --\n";

    ForwardScoutHeader orig;
    orig.SetSrc(Ipv4Address("10.0.0.1"));
    orig.SetDst(Ipv4Address("10.0.0.4"));
    orig.SetSeqno(42);
    orig.SetTtl(20);
    orig.SetTotalEnergy(3.5);
    orig.SetType(FORAGER_LIFETIME);
    orig.AddHop(Ipv4Address("10.0.0.1"));
    orig.AddHop(Ipv4Address("10.0.0.2"));
    orig.AddHop(Ipv4Address("10.0.0.4"));

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(orig);
    ForwardScoutHeader copy;
    pkt->RemoveHeader(copy);

    CHECK(copy.GetSrc()      == orig.GetSrc(),      "src round-trip");
    CHECK(copy.GetDst()      == orig.GetDst(),       "dst round-trip");
    CHECK(copy.GetSeqno()    == orig.GetSeqno(),     "seqno round-trip");
    CHECK(copy.GetTtl()      == orig.GetTtl(),       "ttl round-trip");
    CHECK(copy.GetHopCount() == orig.GetHopCount(),  "hopCount round-trip");
    CHECK(copy.GetType()     == orig.GetType(),      "type round-trip");

    auto ro = orig.GetRoute(), rc = copy.GetRoute();
    CHECK(ro.size() == rc.size(), "route size round-trip");
    bool routeOk = true;
    for (size_t i = 0; i < ro.size() && i < rc.size(); i++)
        if (ro[i] != rc[i]) { routeOk = false; break; }
    CHECK(routeOk, "route content round-trip");

    CHECK(std::abs(copy.GetAvgEnergy() - orig.GetAvgEnergy()) < 1e-9,
          "avgEnergy round-trip");

    // TTL logic
    ForwardScoutHeader t;
    t.SetTtl(2);
    CHECK(!t.TtlExpired(), "TTL=2 not expired");
    t.DecrementTtl();
    t.DecrementTtl();
    CHECK(t.TtlExpired(), "TTL=0 expired");
    t.DecrementTtl();  // should not underflow
    CHECK(t.GetTtl() == 0, "no underflow below 0");
}

// ============================================================
// BackwardScoutHeader
// ============================================================
void TestBackwardScoutHeader()
{
    std::cout << "\n-- BackwardScoutHeader --\n";

    BackwardScoutHeader orig;
    orig.SetSrc(Ipv4Address("10.0.0.1"));
    orig.SetDst(Ipv4Address("10.0.0.4"));
    orig.SetSeqno(7);
    orig.SetAvgEnergy(2.2);
    orig.SetHopCount(3);
    orig.SetRouteIndex(2);
    orig.SetType(FORAGER_DELAY);
    orig.SetRoute({
        Ipv4Address("10.0.0.4"),
        Ipv4Address("10.0.0.2"),
        Ipv4Address("10.0.0.1")
    });

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(orig);
    BackwardScoutHeader copy;
    pkt->RemoveHeader(copy);

    CHECK(copy.GetSrc()        == orig.GetSrc(),        "src round-trip");
    CHECK(copy.GetDst()        == orig.GetDst(),        "dst round-trip");
    CHECK(copy.GetSeqno()      == orig.GetSeqno(),      "seqno round-trip");
    CHECK(copy.GetHopCount()   == orig.GetHopCount(),   "hopCount round-trip");
    CHECK(copy.GetRouteIndex() == orig.GetRouteIndex(), "routeIndex round-trip");
    CHECK(copy.GetType()       == orig.GetType(),       "type round-trip");
    CHECK(std::abs(copy.GetAvgEnergy() - orig.GetAvgEnergy()) < 1e-9,
          "avgEnergy round-trip");

    auto ro = orig.GetRoute(), rc = copy.GetRoute();
    CHECK(ro.size() == rc.size(), "route size round-trip");
    bool ok = true;
    for (size_t i = 0; i < ro.size() && i < rc.size(); i++)
        if (ro[i] != rc[i]) { ok = false; break; }
    CHECK(ok, "route content round-trip");

    // routeIndex walkback logic
    std::vector<Ipv4Address> route = {
        Ipv4Address("10.0.0.1"), Ipv4Address("10.0.0.2"),
        Ipv4Address("10.0.0.3"), Ipv4Address("10.0.0.4")
    };
    BackwardScoutHeader bs;
    bs.SetRoute(route);
    bs.SetRouteIndex((uint8_t)(route.size() - 2)); // = 2
    CHECK((int)bs.GetRouteIndex() == 2, "initial routeIndex = size-2");
    int8_t idx = (int8_t)bs.GetRouteIndex();
    bs.SetRouteIndex((uint8_t)(idx - 1));
    CHECK((int)bs.GetRouteIndex() == 1, "routeIndex decrements correctly");
}

// ============================================================
// ForagerHeader
// ============================================================
void TestForagerHeader()
{
    std::cout << "\n-- ForagerHeader --\n";

    ForagerHeader orig;
    orig.SetSrc(Ipv4Address("10.0.0.1"));
    orig.SetDst(Ipv4Address("10.0.0.4"));
    orig.SetRouteIndex(1);
    orig.SetType(FORAGER_LIFETIME);
    orig.SetAccumEnergy(4.8);
    orig.SetHopCount(2);
    orig.SetRoute({
        Ipv4Address("10.0.0.1"),
        Ipv4Address("10.0.0.2"),
        Ipv4Address("10.0.0.4")
    });

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(orig);
    ForagerHeader copy;
    pkt->RemoveHeader(copy);

    CHECK(copy.GetSrc()        == orig.GetSrc(),        "src round-trip");
    CHECK(copy.GetDst()        == orig.GetDst(),        "dst round-trip");
    CHECK(copy.GetRouteIndex() == orig.GetRouteIndex(), "routeIndex round-trip");
    CHECK(copy.GetType()       == orig.GetType(),       "type round-trip");
    CHECK(copy.GetHopCount()   == orig.GetHopCount(),   "hopCount round-trip");
    CHECK(std::abs(copy.GetAccumEnergy() - orig.GetAccumEnergy()) < 1e-9,
          "accumEnergy round-trip");

    auto ro = orig.GetRoute(), rc = copy.GetRoute();
    CHECK(ro.size() == rc.size(), "route size round-trip");
    bool ok = true;
    for (size_t i = 0; i < ro.size() && i < rc.size(); i++)
        if (ro[i] != rc[i]) { ok = false; break; }
    CHECK(ok, "route content round-trip");

    // AdvanceHop / AtDestination
    ForagerHeader fh;
    fh.SetRoute({
        Ipv4Address("10.0.0.1"),
        Ipv4Address("10.0.0.2"),
        Ipv4Address("10.0.0.4")
    });
    fh.SetRouteIndex(1);
    CHECK(!fh.AtDestination(), "not at dest (idx=1, size=3)");
    fh.AdvanceHop();
    CHECK(!fh.AtDestination(), "not at dest (idx=2, size=3)");
    fh.AdvanceHop();
    CHECK(fh.AtDestination(),  "at dest (idx=3 >= size=3)");
}

// ============================================================
// SwarmHeader
// ============================================================
void TestSwarmHeader()
{
    std::cout << "\n-- SwarmHeader --\n";

    SwarmHeader orig;
    orig.SetSrc(Ipv4Address("10.0.0.4"));
    orig.SetDst(Ipv4Address("10.0.0.1"));
    orig.SetForagerCount(5);
    orig.SetAvgEnergy(1.9);
    orig.SetHopCount(3);
    orig.SetRouteIndex(2);
    orig.SetRoute({
        Ipv4Address("10.0.0.4"),
        Ipv4Address("10.0.0.2"),
        Ipv4Address("10.0.0.1")
    });

    Ptr<Packet> pkt = Create<Packet>();
    pkt->AddHeader(orig);
    SwarmHeader copy;
    pkt->RemoveHeader(copy);

    CHECK(copy.GetSrc()          == orig.GetSrc(),          "src round-trip");
    CHECK(copy.GetDst()          == orig.GetDst(),          "dst round-trip");
    CHECK(copy.GetForagerCount() == orig.GetForagerCount(), "foragerCount round-trip");
    CHECK(copy.GetHopCount()     == orig.GetHopCount(),     "hopCount round-trip");
    CHECK(copy.GetRouteIndex()   == orig.GetRouteIndex(),   "routeIndex round-trip");
    CHECK(std::abs(copy.GetAvgEnergy() - orig.GetAvgEnergy()) < 1e-9,
          "avgEnergy round-trip");

    auto ro = orig.GetRoute(), rc = copy.GetRoute();
    CHECK(ro.size() == rc.size(), "route size round-trip");
    bool ok = true;
    for (size_t i = 0; i < ro.size() && i < rc.size(); i++)
        if (ro[i] != rc[i]) { ok = false; break; }
    CHECK(ok, "route content round-trip");
}

// ============================================================
// DanceFloor
// ============================================================
void TestDanceFloor()
{
    std::cout << "\n-- DanceFloor --\n";

    DanceFloor df;
    Ipv4Address dst("10.0.0.9");
    Ipv4Address via("10.0.0.2");

    CHECK(!df.HasForager(dst), "no forager before add");
    CHECK(df.CountForagers(dst) == 0, "CountForagers=0 before add");

    ForagerEntry fe;
    fe.dst = dst;
    fe.route = {
        Ipv4Address("10.0.0.1"),
        via,
        dst
    };
    fe.type = FORAGER_LIFETIME;
    fe.quality = 80.0;
    fe.danceNum = 3;
    fe.createdAt = Simulator::Now();
    fe.lifetime = Seconds(30.0);
    df.AddForager(fe);

    CHECK(df.HasForager(dst), "HasForager true after add");
    CHECK(df.CountForagers(dst) == 3, "CountForagers matches danceNum after add");
    CHECK(df.CountOutgoing(via) == 3, "CountOutgoing sums dances via first hop");

    ForagerEntry chosen;
    CHECK(df.GetForager(dst, chosen), "GetForager succeeds after add");
    CHECK(chosen.dst == dst, "chosen forager keeps destination");
    CHECK(chosen.route.size() == 3, "chosen forager keeps route");
    CHECK(chosen.route[1] == via, "chosen next hop is preserved");
    CHECK(chosen.danceNum == 2, "chosen danceNum reflects decrement-before-copy");
    CHECK(df.CountForagers(dst) == 2, "CountForagers decremented after GetForager");

    df.UpdateForager(dst, fe.route, 100.0);
    CHECK(df.CountForagers(dst) == 6, "UpdateForager refreshes dance count using cap formula");

    ForagerEntry refreshed;
    CHECK(df.GetForager(dst, refreshed), "GetForager succeeds after update");
    CHECK(std::abs(refreshed.quality - 100.0) < 1e-9, "UpdateForager refreshes quality");
    CHECK(refreshed.danceNum == 5, "retrieved danceNum decremented from refreshed count");
}

void TestDanceFloorPurge()
{
    std::cout << "\n-- DanceFloor purge/expiry --\n";

    DanceFloor df;
    Ipv4Address dst("10.0.0.10");
    Ipv4Address via("10.0.0.3");

    ForagerEntry expired;
    expired.dst = dst;
    expired.route = {
        Ipv4Address("10.0.0.1"),
        via,
        dst
    };
    expired.type = FORAGER_LIFETIME;
    expired.quality = 45.0;
    expired.danceNum = 2;
    expired.createdAt = Seconds(0.0);
    expired.lifetime = Seconds(1.0);
    df.AddForager(expired);

    CHECK(!df.HasForager(dst), "expired entry is ignored by HasForager");
    CHECK(df.CountForagers(dst) == 0, "expired entry does not contribute to CountForagers");
    CHECK(df.CountOutgoing(via) == 0, "expired entry does not contribute to CountOutgoing");

    df.Purge();
    ForagerEntry out;
    CHECK(!df.GetForager(dst, out), "Purge removes expired entry from retrieval");
}

// ============================================================
// ForagerEntry expiry
// ============================================================
void TestForagerEntryExpiry()
{
    std::cout << "\n-- ForagerEntry expiry --\n";

    ForagerEntry e;
    e.createdAt = Simulator::Now();  // t=0
    e.lifetime  = Seconds(5.0);
    CHECK(!e.IsExpired(), "not expired at t=0");

    Simulator::Schedule(Seconds(6.0), []() {
        ForagerEntry e2;
        e2.createdAt = Seconds(0.0);
        e2.lifetime  = Seconds(5.0);
        CHECK(e2.IsExpired(), "expired at t=6 (lifetime=5)");
    });
}

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[])
{
    std::cout << "=== BeeAdHoc Header Tests ===\n";

    TestForwardScoutHeader();
    TestBackwardScoutHeader();
    TestForagerHeader();
    TestSwarmHeader();
    TestDanceFloor();

    // Expiry test needs the simulator clock
    Simulator::Schedule(Seconds(0.0), &TestForagerEntryExpiry);
    Simulator::Schedule(Seconds(2.0), &TestDanceFloorPurge);
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "\n=== Done ===\n";
    return 0;
}
