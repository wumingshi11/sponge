#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

bool NetworkInterface::_vaild_ip(uint32_t ip) {
  if (_ip2eths.find(ip) != _ip2eths.end()) {
    if (_ip2eths[ip].remain <= ETH_TIMEOUT) return true; 
  }
  return false;
}

void NetworkInterface::_arp(uint32_t ip) {
  if (_vaild_ip(ip)) return; 
  if (_ip2eths.find(ip) == _ip2eths.end() || _ip2eths[ip].last_send >= AGAIN_ARP) {
    EthernetHeader header{ETHERNET_BROADCAST, _ethernet_address, EthernetHeader:: TYPE_ARP};
    ARPMessage arpmessage;
    arpmessage.opcode = ARPMessage::OPCODE_REQUEST;
    arpmessage.target_ip_address = ip;
    //arpmessage.target_ethernet_address = ETHERNET_BROADCAST;
    arpmessage.sender_ethernet_address = _ethernet_address;
    arpmessage.sender_ip_address = _ip_address.ipv4_numeric();
    EthernetFrame reply_frame;
    reply_frame.header() = header;
    reply_frame.payload() = arpmessage.serialize();
    _frames_out.push(reply_frame);
    if (_ip2eths.find(ip) == _ip2eths.end()) {
      _ip2eths.emplace(ip, ip2eth{0, ETH_TIMEOUT + 1, EthernetAddress{}});
    }
  }
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    EthernetHeader header{EthernetAddress{}, _ethernet_address, EthernetHeader::TYPE_IPv4};
    EthernetFrame frame;
    frame.header() = header;
    frame.payload() = dgram.serialize();
    if (_vaild_ip(next_hop_ip)) {      
      frame.header().dst = _ip2eths[next_hop_ip].eth_addr;
      _frames_out.push(frame);
    } else {
      if (_wait_send_EthenetFrame.find(next_hop_ip) == _wait_send_EthenetFrame.cend()) {
        _wait_send_EthenetFrame.emplace(next_hop_ip, queue<EthernetFrame>{});
      }
      _wait_send_EthenetFrame[next_hop_ip].push(frame);
      _arp(next_hop_ip);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST)
        return optional<InternetDatagram>{};
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        auto result = dgram.parse(frame.payload());
        if (result == ParseResult::NoError) return dgram;
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arpmessage;
        auto result = arpmessage.parse(frame.payload());
        if (result != ParseResult::NoError) return optional<InternetDatagram>{};
        if (_ip2eths.find(arpmessage.sender_ip_address) == _ip2eths.end()) { 
          _ip2eths.emplace(arpmessage.sender_ip_address, ip2eth{0, 0, arpmessage.sender_ethernet_address});
        } else {
          _ip2eths[arpmessage.sender_ip_address] = ip2eth{0, 0, arpmessage.sender_ethernet_address};
        }

        if (arpmessage.opcode == ARPMessage::OPCODE_REQUEST && 
            arpmessage.target_ip_address == _ip_address.ipv4_numeric()) {
            EthernetHeader header{frame.header().src, _ethernet_address, EthernetHeader:: TYPE_ARP};
            ARPMessage arpmessage_reply;
            arpmessage_reply.opcode = ARPMessage::OPCODE_REPLY;
            arpmessage_reply.target_ip_address = arpmessage.sender_ip_address;
            arpmessage_reply.target_ethernet_address = arpmessage.sender_ethernet_address;
            arpmessage_reply.sender_ethernet_address = _ethernet_address;
            arpmessage_reply.sender_ip_address = _ip_address.ipv4_numeric();
            EthernetFrame reply_frame;
            reply_frame.header() = header;
            reply_frame.payload() = arpmessage_reply.serialize();
            _frames_out.push(reply_frame);
        }

        if (arpmessage.target_ip_address == _ip_address.ipv4_numeric()) {
            if (_wait_send_EthenetFrame.find(arpmessage.sender_ip_address) != 
                _wait_send_EthenetFrame.end()) {
              auto target = _ip2eths[arpmessage.sender_ip_address].eth_addr;
              auto &frames =  _wait_send_EthenetFrame.at(arpmessage.sender_ip_address);
              while (!frames.empty()) {
                auto& frame_t = frames.front();
                frame_t.header().dst = target;
                _frames_out.push(frame_t);
                frames.pop();
              }
              _wait_send_EthenetFrame.erase(arpmessage.sender_ip_address);
            }
        }

    } else {
      cerr << "error ethernethead type" << endl;
    }
    return optional<InternetDatagram>{};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
  for(auto& ip2eth_e : _ip2eths) {
    ip2eth_e.second.last_send += ms_since_last_tick;
    ip2eth_e.second.remain += ms_since_last_tick;
  }
}
