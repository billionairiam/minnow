#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

const size_t ARP_ENTRY_TIMEOUT = 30000;

const size_t ARP_REQUEST_TIMEOUT = 5000;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address ), arp_table_ {}
  , arp_requests_ {}, ms_since_last_tick_ {}
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  auto arp_entry = arp_table_.find(next_hop.ipv4_numeric());
  if (arp_entry != arp_table_.end() and arp_entry->second.first + ARP_ENTRY_TIMEOUT > ms_since_last_tick_) {

    EthernetFrame frame {
      .header = {
        .dst = arp_entry->second.second,
        .src = ethernet_address_,
        .type = EthernetHeader::TYPE_IPv4,
      },
      .payload = serialize(dgram),
    };

    transmit(frame);
  } else {
    pending_dgram_[next_hop.ipv4_numeric()] = dgram;

    auto arp_request_time = arp_requests_.find(next_hop.ipv4_numeric());

    bool should_send_arp_request = (arp_request_time == arp_requests_.end()) ||
                                       (arp_request_time->second + ARP_REQUEST_TIMEOUT <= ms_since_last_tick_);
    if (should_send_arp_request) {
      ARPMessage arp_request {
        .opcode = ARPMessage::OPCODE_REQUEST,
        .sender_ethernet_address = ethernet_address_, // Our MAC address
        .sender_ip_address = ip_address_.ipv4_numeric(), // Our IP address
        .target_ethernet_address = {}, // Unknown target MAC address
        .target_ip_address = next_hop.ipv4_numeric(), // Target IP address
      };

      // Encapsulate ARP request in an Ethernet frame
      EthernetFrame arp_frame {
        .header = {
          .dst = ETHERNET_BROADCAST,
          .src = ethernet_address_,
          .type = EthernetHeader::TYPE_ARP
        },
        .payload = serialize(arp_request),
      };

      arp_requests_[next_hop.ipv4_numeric()] = ms_since_last_tick_;

      transmit(arp_frame);
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if (frame.header.type == EthernetHeader::TYPE_IPv4 and 
      frame.header.dst == ethernet_address_) {
    InternetDatagram dgram;

    if (parse( dgram, frame.payload )) {
      datagrams_received_.push(dgram);
    }
  }

  if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp_msg;
    if (!parse(arp_msg, frame.payload)) 
      return;

    arp_table_[arp_msg.sender_ip_address] = {ms_since_last_tick_, arp_msg.sender_ethernet_address};
    if (arp_msg.target_ip_address == ip_address_.ipv4_numeric()) {
      if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST) {
        ARPMessage arp_reply {
          .opcode = ARPMessage::OPCODE_REPLY,
          .sender_ethernet_address = ethernet_address_, // Our Ethernet address
          .sender_ip_address = ip_address_.ipv4_numeric(), // Our IP address
          .target_ethernet_address = arp_msg.sender_ethernet_address, // Requester's Ethernet address
          .target_ip_address = arp_msg.sender_ip_address // Requester's IP address
        };

        // Encapsulate ARP reply in an Ethernet frame
        EthernetFrame reply_frame {
            .header = {
              .dst = arp_msg.sender_ethernet_address,
              .src = ethernet_address_,
              .type = frame.header.TYPE_ARP,
            },
            .payload = serialize(arp_reply)
        };

        transmit(reply_frame);
      }

      if (arp_msg.opcode == ARPMessage::OPCODE_REPLY) {
        if (pending_dgram_.find(arp_msg.sender_ip_address) != pending_dgram_.end()) {
          send_datagram(pending_dgram_[arp_msg.sender_ip_address],
                        Address::from_ipv4_numeric(arp_msg.sender_ip_address));

          pending_dgram_.erase(pending_dgram_.find(arp_msg.sender_ip_address));
        }
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  ms_since_last_tick_ += ms_since_last_tick;
  for (auto it = arp_table_.begin(); it != arp_table_.end(); ) {
    if (ms_since_last_tick_ - it->second.first >= ARP_ENTRY_TIMEOUT) {  // Condition: erase if value is >= 30
        it = arp_table_.erase(it);  // Erase and move the iterator forward
    } else {
        ++it;  // Move the iterator forward if not erasing
    }
  }
}
