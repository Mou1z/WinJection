#include "WinJection.h"
#include <windows.h>
#include <WinDivert.h>
#include <cstring>
#include <thread>
#include <chrono>

PacketManager::PacketManager() : 
    _handle(INVALID_HANDLE_VALUE),
    _injecting(false),
    streams()
{}

PacketManager::~PacketManager() {
    if (_injectionThread.joinable()) {
        _injectionThread.join();
    }
}

void PacketManager::_preprocess(char* packet, uint32_t packetLen, uint64_t outbound) {
    WINDIVERT_IPHDR* ipv4;
    WINDIVERT_TCPHDR* tcp;

    _last_payload_len = packetLen;

    bool parsed = WinDivertHelperParsePacket(
        packet, packetLen, &ipv4, nullptr, nullptr, nullptr, nullptr, &tcp, nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (!parsed) {
        printf("Failed to parse packet\n");
        return;
    }

    uint16_t id = ntohs(ipv4->Id);
    uint32_t seq = ntohl(tcp->SeqNum);
    uint32_t ack = ntohl(tcp->AckNum);
    uint16_t dst_port = ntohs(tcp->DstPort);
    uint16_t src_port = ntohs(tcp->SrcPort);

    if (outbound == 1) {
        if (streams.count(src_port)) {
            Stream* stream = &streams[src_port];
            ipv4->Id = htons((id + stream->identDisplacement[0]) % 65536);
            tcp->SeqNum = htonl((seq + stream->syncDisplacement[0]) % 4294967295);
            tcp->AckNum = htonl((ack - stream->syncDisplacement[1]) % 4294967295);
        } else {
            streams.emplace(src_port, Stream(src_port, dst_port));
        }
    } else {
        if (streams.count(dst_port)) {
            Stream* stream = &streams[dst_port];
            ipv4->Id = htons((id + stream->identDisplacement[1]) % 65536);
            tcp->SeqNum = htonl((seq + stream->syncDisplacement[1]) % 4294967295);
            tcp->AckNum = htonl((ack - stream->syncDisplacement[0]) % 4294967295);
        } else {
            streams.emplace(dst_port, Stream(dst_port, src_port));
        }
    }
}

void PacketManager::start(const char* filter, WINDIVERT_LAYER layer, int16_t priority, uint64_t flags) {
    _handle = WinDivertOpen(filter, layer, priority, flags);

    if (_handle == INVALID_HANDLE_VALUE) {
        int error = GetLastError();
        switch (error) {
        case 5:
            printf("Execution failed. Please run the program with administrator privileges.\n");
            break;
        default:
            printf("Execution failed. Error Code: %d\n", error);
            break;
        }
        exit(1);
    }

    _injectionThread = std::thread([this]() { 
        _injection(); 
    });

    while (true) {
        if (!WinDivertRecv(_handle, _last_packet, sizeof(_last_packet), &_last_packet_len, _last_addr)) {
            printf("ERROR: Failed to read the packet. Error Code: %d\n", GetLastError());
            continue;
        }

        _preprocess(_last_packet, _last_packet_len, _last_addr->Outbound);
        onPacketProcess(_handle, _last_packet, _last_packet_len, _last_addr);
    }
}

void PacketManager::send(char * packet, UINT packetLen) {
    if(_handle == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to send packet. (Error: No Connection)\n");
        exit(1);
    }

    WinDivertHelperCalcChecksums(packet, packetLen, _last_addr, 0);
    if (!WinDivertSend(_handle, packet, packetLen, NULL, _last_addr)) {
        printf("ERROR: Failed to send the packet. (Error Code: %d)\nPacket Data: ", GetLastError());
        for(UINT i = 0; i < packetLen; i++)
            printf("%d ", packet[i]);
        printf("\n");
    }
}

void PacketManager::_injection() {
    while(true) {
        if(!_inj_queue.empty()) {
            _injecting = true;
        }
        _injecting = false;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void PacketManager::inject(int src_port, int dst_port, char * payload, uint32_t payloadLen) {
    if(_handle == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to inject packet. (Error: No Connection)\n");
        exit(1);
    }

    // Currently working on this
    // Todo:
    // 1. Generate packets based on the payload
    // 2. Append the packets to _inj_queue
    // 3. Add checks on line 87 to make sure the packet is not sent while _injecting is true
}