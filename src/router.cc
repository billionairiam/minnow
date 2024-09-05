#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  // Step 1: Validate prefix length.
  if (prefix_length > 32) {
      throw std::invalid_argument("Prefix length must be between 0 and 32");
  }

  // Step 2: Create a new RouteEntry with the provided information.
  RouteEntry new_entry{route_prefix, prefix_length, next_hop, interface_num};

  // Step 3: Insert the route entry into the routing table.
  // For simplicity, let's append it to a vector of RouteEntry objects.
  // In a real-world scenario, this might involve more sophisticated data structures.
  _routes.push_back(new_entry);
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for (auto &interface_: _interfaces) {
    while (!interface_->datagrams_received().empty()) {
        InternetDatagram& datagram = interface_->datagrams_received().front();
        if (!datagram.header.ttl) {
          interface_->datagrams_received().pop();
          continue;
        }

        bool choose = false;
        RouteEntry choose_route {};
        for (auto& route: _routes) {
          if (match_route(route, datagram.header.dst)) {
            choose = true;
            if (choose_route.prefix_length < route.prefix_length) {
              choose_route = route;
            }
          }
        }

        if (!choose) {
          interface_->datagrams_received().pop();
        } else {
          datagram.header.ttl--;
          datagram.header.compute_checksum();
          if (choose_route.next_hop.has_value()) {
            interface(choose_route.interface_num)->send_datagram(datagram, choose_route.next_hop.value());
          } else {
            interface(choose_route.interface_num)->send_datagram(datagram, Address::from_ipv4_numeric(datagram.header.dst));
          }
          interface_->datagrams_received().pop();
        }
    }
  }
}
