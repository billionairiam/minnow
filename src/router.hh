#pragma once

#include <memory>
#include <optional>

#include "exception.hh"
#include "network_interface.hh"

// RouteEntry represents a single route entry in the routing table.
struct RouteEntry {
    uint32_t route_prefix;              // Network prefix (in host byte order).
    uint8_t prefix_length;              // Prefix length (0-32).
    std::optional<Address> next_hop;    // Optional next-hop address.
    size_t interface_num;               // Interface number.
};

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};

  // A simple vector to store routes. Could be a more complex structure for efficiency.
  std::vector<RouteEntry> _routes {};

  bool match_route(const RouteEntry& route, uint32_t destination_ip) {
      uint32_t mask = (route.prefix_length == 0) ? 0 : ~((1 << (32 - route.prefix_length)) - 1);
      return (destination_ip & mask) == (route.route_prefix & mask);
  }
};
