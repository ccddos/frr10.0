# OSPFv3 BFD Integration Guide

This document provides a comprehensive guide to the Bidirectional Forwarding Detection (BFD) integration with OSPFv3 in FRR.

## Overview

The OSPFv3 daemon (ospf6d) in FRR has built-in support for BFD, which provides fast failure detection for network paths. BFD operates independently of the routing protocol and can detect failures much faster than the routing protocol's own mechanisms.

## Architecture

### Core Components

1. **BFD Session Management** (`ospf6_bfd.c`)
   - Session creation and destruction
   - Parameter configuration
   - State change handling

2. **Interface Configuration** (`ospf6_interface.h`)
   - Per-interface BFD configuration
   - BFD parameters storage

3. **Neighbor Integration** (`ospf6_neighbor.h`)
   - Per-neighbor BFD session
   - State synchronization

### Key Data Structures

#### Interface BFD Configuration
```c
struct {
    bool enabled;                    // BFD enabled on interface
    uint8_t detection_multiplier;    // Failure detection multiplier
    uint32_t min_rx;                 // Minimum receive interval
    uint32_t min_tx;                 // Minimum transmit interval  
    char *profile;                   // BFD profile name
} bfd_config;
```

#### Neighbor BFD Session
```c
struct bfd_session_params *bfd_session;  // BFD session parameters
```

## Configuration Commands

### Enable BFD on Interface

#### Basic BFD Enable
```
interface eth0
 ipv6 ospf6 bfd
```

#### BFD with Custom Parameters (Legacy)
```
interface eth0
 ipv6 ospf6 bfd <detection-multiplier> <min-rx> <min-tx>
```
- `detection-multiplier`: 2-255 (number of missed packets before declaring failure)
- `min-rx`: 50-60000 ms (minimum receive interval)
- `min-tx`: 50-60000 ms (minimum transmit interval)

#### BFD with Profile
```
interface eth0
 ipv6 ospf6 bfd profile <profile-name>
```

### Disable BFD

#### Remove BFD Profile
```
interface eth0
 no ipv6 ospf6 bfd profile [profile-name]
```

#### Disable BFD Completely
```
interface eth0
 no ipv6 ospf6 bfd
```

## BFD Operation Flow

### Session Lifecycle

1. **Interface Configuration**: BFD is enabled on OSPFv3 interface
2. **Neighbor Discovery**: OSPFv3 neighbor relationship established
3. **Session Creation**: BFD session created when neighbor reaches TWOWAY state
4. **Session Installation**: BFD session installed with zebra/bfdd
5. **Monitoring**: BFD monitors bidirectional connectivity
6. **Failure Detection**: BFD detects path failure
7. **Notification**: OSPFv3 neighbor marked as down via callback

### State Integration

```
OSPFv3 Neighbor State     BFD Action
-------------------      -----------
< TWOWAY                 No BFD session
>= TWOWAY               Install BFD session
Failure detected        Trigger inactivity timer
```

## Configuration Examples

### Basic Configuration

```
# Enable BFD daemon
bfd
 peer 192.168.1.2

# Configure OSPFv3 with BFD
router ospf6
 router-id 192.168.1.1

interface eth0
 ipv6 ospf6 area 0.0.0.0
 ipv6 ospf6 bfd
```

### Advanced Configuration with Profile

```
# BFD daemon with profile
bfd
 profile fast
  detect-multiplier 3
  receive-interval 200
  transmit-interval 200

# OSPFv3 using BFD profile
interface eth0
 ipv6 ospf6 area 0.0.0.0
 ipv6 ospf6 bfd profile fast
```

### Multi-interface Setup

```
router ospf6
 router-id 192.168.1.1

interface eth0
 ipv6 ospf6 area 0.0.0.0
 ipv6 ospf6 bfd
 
interface eth1
 ipv6 ospf6 area 0.0.0.1
 ipv6 ospf6 bfd profile backbone
```

## Verification Commands

### Show BFD Sessions
```
show bfd peers
show bfd peer <address>
```

### Show OSPFv3 Neighbors
```
show ipv6 ospf6 neighbor
show ipv6 ospf6 interface
```

### Show Interface Configuration
```
show running-config interface <name>
```

## Implementation Details

### Key Functions

#### Session Management
- `ospf6_bfd_info_nbr_create()`: Create BFD session for neighbor
- `ospf6_bfd_trigger_event()`: Handle neighbor state changes
- `ospf6_bfd_callback()`: Process BFD session status changes

#### Configuration
- `ospf6_bfd_write_config()`: Write BFD config to running configuration
- Command handlers for CLI configuration

### Integration Points

1. **Neighbor State Machine**: BFD sessions installed/removed based on neighbor states
2. **Address Updates**: BFD session addresses updated when neighbor link-local address changes
3. **Interface Configuration**: BFD parameters inherited from interface configuration
4. **Failure Handling**: BFD failures trigger OSPFv3 inactivity timer

## Troubleshooting

### Common Issues

1. **BFD Session Not Established**
   - Verify bfdd is running
   - Check interface configuration
   - Confirm neighbor is in TWOWAY or higher state

2. **Frequent BFD Flaps**
   - Adjust detection multiplier
   - Increase receive/transmit intervals
   - Check network latency/jitter

3. **Configuration Not Applied**
   - Verify interface exists in OSPFv3
   - Check for configuration conflicts
   - Restart ospf6d if needed

### Debug Commands

```
debug bfd
debug ipv6 ospf6 neighbor
debug ipv6 ospf6 interface
```

## Performance Considerations

### Recommended Settings

- **LAN Networks**: 3x multiplier, 300ms intervals
- **WAN Networks**: 5x multiplier, 1000ms intervals  
- **Critical Links**: Use BFD profiles for consistency

### Scaling

- BFD sessions are created per neighbor relationship
- Consider CPU and memory impact with large neighbor counts
- Use profiles to standardize configurations

## Best Practices

1. **Use BFD Profiles**: Standardize BFD parameters across interfaces
2. **Tune for Network Type**: Adjust timers based on network characteristics
3. **Monitor Performance**: Watch for CPU impact with many sessions
4. **Test Failure Scenarios**: Verify BFD detects failures correctly
5. **Document Configuration**: Maintain clear network documentation

## Security Considerations

- BFD packets are not authenticated by default
- Use authentication when available
- Secure control plane access to BFD configuration
- Monitor for potential DoS attacks via BFD flooding

## Future Enhancements

Areas for potential improvement:
- Authentication support in OSPFv3-BFD integration
- IPv6 multicast BFD support
- Enhanced debugging and statistics
- Integration with network monitoring systems