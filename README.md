# pubsub-lite

A minimal publish-subscribe messaging system in C++, inspired by ROS's node/topic architecture. Built from scratch using TCP sockets on Windows (Winsock2) with no external dependencies.

FULL ROS SUCKS AND TAKES WAY TOO LONG TO IMPLEMENT!!!!!! THIS IMPLEMENTATION IS A LIGHTWEIGHT ALTERNATIVE THAT YOU CAN FIT INTO ANY PROJECT REGARDLESS OF DEPENDENCIES!!!!

---

## How It Works

Three components work together:

**Broker** — a standalone process that runs permanently. It accepts connections from any number of nodes, maintains a registry of which nodes are subscribed to which topics, and forwards published messages to all matching subscribers. Nodes never talk directly to each other.

**Publisher node** — connects to the broker, sends a message tagged with a topic name, and disconnects. Fire-and-forget.

**Subscriber node** — connects to the broker, registers interest in a topic, then keeps the connection open. When a publisher sends a message on that topic, the broker pushes it to all subscribers. The subscriber fires a user-provided callback for each message received.

---

## Message Protocol

Every message sent between a node and the broker follows this wire format:

```
[1 byte: message type] [1 byte: topic length] [N bytes: topic] [1 byte: payload length] [M bytes: payload]
```

Message types are defined in `Infra/protocol.h`:
- `MSG_SUBSCRIBE (0x01)` — register as a subscriber for a topic
- `MSG_PUBLISH   (0x02)` — send a message to a topic
- `MSG_DATA      (0x03)` — reserved for future broker-to-node data framing

---

## Project Structure

```
pubsub-lite/
├── Infra/
│   ├── broker.cpp      — the broker process
│   ├── protocol.h      — shared message type constants
│   ├── server.cpp      — example subscriber using the Node API
│   └── client.cpp      — example publisher using the Node API
├── Node/
│   ├── node.h          — Node class declaration
│   └── node.cpp        — Node class implementation
├── testing-files/
│   └── playfile.cpp    — scratch file
└── CMakeLists.txt
```

---

## Node API

```cpp
Node(std::string name)
```
Constructs a node with the given name.

```cpp
void publish(std::string topic, std::string message)
```
Connects to the broker, publishes `message` to `topic`, and disconnects. Safe to call on a node that also subscribes — uses its own socket and does not interfere with the subscriber connection.

```cpp
void subscribe(std::string topic, std::function<void(const std::string&)> callback)
```
Connects to the broker, registers as a subscriber for `topic`, and stores `callback` for later. Keeps the connection open. Must be called before `spin()`.

```cpp
void spin()
```
Blocks the calling thread, listening for messages from the broker. Fires `callback` each time one arrives. If a node needs to both publish and subscribe simultaneously, run `spin()` on a `std::thread`.

---

## Running

Build order (CMake):
```
cmake -S . -B build
cmake --build build
```

Launch order:
1. `broker.exe` — must be running first
2. Subscriber nodes (they register with the broker)
3. Publisher nodes (they send messages the broker routes)

---

## Platform

Windows only — uses Winsock2 (`ws2_32.lib`). Requires C++17.
