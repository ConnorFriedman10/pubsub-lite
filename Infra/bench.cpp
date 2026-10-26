// bench.cpp - latency/throughput benchmark for pubsub-lite
//
// Measures the time from publish() to the subscriber callback firing.
// Run broker.exe first, then:
//
//   bench.exe <num_subscribers> <num_messages>
//   e.g.  bench.exe 1 10000
//         bench.exe 10 10000
//         bench.exe 50 5000

#include "../Node/node.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

static int64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch())
        .count();
}

static double pct(std::vector<int64_t>& v, double p) {
    size_t i = static_cast<size_t>(p * (v.size() - 1));
    return v[i] / 1000.0;  // ns -> microseconds
}

int main(int argc, char** argv) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    const int numSubs = argc > 1 ? std::atoi(argv[1]) : 1;
    const int numMsgs = argc > 2 ? std::atoi(argv[2]) : 10000;
    const int warmup  = 100;
    const int perSub  = warmup + numMsgs;

    // One latency vector per subscriber; each is written only by its own
    // spin() thread, so no locking is needed.
    std::vector<std::vector<int64_t>> lat(numSubs);
    std::atomic<int> received{0};
    std::vector<std::unique_ptr<Node>> subs;

    for (int i = 0; i < numSubs; ++i) {
        lat[i].reserve(perSub);
        subs.push_back(std::make_unique<Node>("bench_sub_" + std::to_string(i)));
        auto* v = &lat[i];
        subs.back()->subscribe("bench", [v, &received](const std::string& msg) {
            v->push_back(now_ns() - std::stoll(msg));  // payload = send timestamp
            received.fetch_add(1);
        });
    }
    for (auto& s : subs) {
        Node* n = s.get();
        std::thread([n] { n->spin(); }).detach();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // let subscriptions register

    Node pub("bench_pub");
    for (int i = 0; i < warmup; ++i) pub.publish("bench", std::to_string(now_ns()));

    const auto start = Clock::now();
    for (int i = 0; i < numMsgs; ++i) pub.publish("bench", std::to_string(now_ns()));

    const int expected = perSub * numSubs;
    const auto deadline = Clock::now() + std::chrono::seconds(30);
    while (received.load() < expected && Clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const double secs = std::chrono::duration<double>(Clock::now() - start).count();

    std::vector<int64_t> all;
    for (auto& v : lat)
        if (static_cast<int>(v.size()) > warmup) all.insert(all.end(), v.begin() + warmup, v.end());
    std::sort(all.begin(), all.end());

    std::cout << "subscribers:       " << numSubs << "\n"
              << "messages sent:     " << numMsgs << "\n"
              << "deliveries:        " << all.size() << " / "
              << static_cast<long long>(numMsgs) * numSubs << "\n"
              << "throughput:        " << numMsgs / secs << " msgs/sec published\n";
    if (!all.empty()) {
        std::cout << "latency median:    " << pct(all, 0.50) << " us\n"
                  << "latency p99:       " << pct(all, 0.99) << " us\n"
                  << "latency max:       " << all.back() / 1000.0 << " us\n";
    }
    std::_Exit(0);  // spin() threads block forever, so skip normal shutdown
}
