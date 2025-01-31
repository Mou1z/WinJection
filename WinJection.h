#ifndef WINJECTION_H
#define WINJECTION_H

#include <unordered_map>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <WinSock2.h>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "Stream.h"
#include <WinDivert.h>

#define MAXBUF 65535

class PacketManager {
public:
    PacketManager();
    ~PacketManager();

    void start(const char* filter, WINDIVERT_LAYER layer, int16_t priority, uint64_t flags);
    void send(char * packet, UINT packetLen);
    void inject(int src_port, int dst_port, char * payload, uint32_t payloadLen);

    virtual void onPacketProcess(HANDLE, char*, uint32_t, WINDIVERT_ADDRESS*);

private:
    HANDLE _handle;

    char _last_packet[MAXBUF];
    uint32_t _last_packet_len;
    uint32_t _last_payload_len;
    WINDIVERT_ADDRESS * _last_addr;

    bool _injecting;
    std::queue<char*> _inj_queue;

    std::unordered_map<uint16_t, Stream> streams;

    void _preprocess(char* packet, uint32_t packetLen, uint64_t outbound);
    void _injection();

    std::thread _injectionThread;
};

#endif