# ISIS Grid Topology Configuration Examples

This directory contains FRR ISIS configuration examples for multi-area grid topology deployments, optimized for fast convergence.

## Overview

These configurations demonstrate how to set up ISIS in a grid topology where:
- Large grids are divided into smaller sub-grids (areas)
- Each sub-grid operates as an ISIS area
- L1L2 boundary routers connect different areas
- IETF SPF optimization is enabled for fast convergence

## Topology Design

```
Grid-A (49.0001) ←→ Grid-B (49.0002)
     ↕                    ↕
Grid-C (49.0003) ←→ Grid-D (49.0004)
```

Each 3x3 sub-grid contains:
- 4 L1L2 boundary routers (edges: R2, R4, R6, R8)
- 5 L1 internal routers (including center: R1, R3, R5, R7, R9)

## Configuration Files

### 1. grid-boundary-router.conf
Configuration template for L1L2 boundary routers that:
- Connect to other grids via L2 adjacencies
- Provide inter-area routing capabilities
- Inject default routes into L1 area via ATT bit
- Perform route summarization

**Key Features:**
- Fast convergence with IETF SPF optimization
- Sub-second Hello intervals
- Optimized LSP generation timers
- ECMP support
- Optional BFD integration

### 2. grid-internal-router.conf
Configuration template for L1 internal routers that:
- Operate purely within the local area
- Rely on default routes for inter-area traffic
- Provide intra-area connectivity

**Key Features:**
- Simplified L1-only operation
- Fast convergence optimization
- BFD support for rapid failure detection
- Optimized for minimal resource usage

## Performance Characteristics

With these optimizations, expected convergence times are:
- Failure detection: 1-3 seconds (Hello timeout)
- SPF calculation: 50-200ms (IETF SPF)
- Route installation: 100-500ms
- **Total convergence: 2-4 seconds** (without BFD)
- **Total convergence: 1-2 seconds** (with BFD)

## Key Configuration Parameters

### Fast Convergence Settings
```
spf-delay-ietf init-delay 50 short-delay 50 long-delay 200 holddown 2000 time-to-learn 500
lsp-gen-interval 1
isis hello-interval 1
isis hello-multiplier 3
```

### Route Optimization
```
summary-address 10.X.0.0/16 level-2
maximum-paths 4
isis network point-to-point
```

## Usage Instructions

1. **Boundary Router Setup:**
   - Use `grid-boundary-router.conf` as template
   - Adjust NET addresses for each area
   - Configure inter-grid interfaces
   - Set appropriate metrics

2. **Internal Router Setup:**
   - Use `grid-internal-router.conf` as template
   - Configure only intra-area interfaces
   - Ensure L1-only operation

3. **Network-wide Considerations:**
   - Plan NET addressing scheme
   - Configure route summarization
   - Set up monitoring and debugging

## Monitoring Commands

```bash
# Check adjacencies
show isis neighbor

# Verify convergence
show isis spf-delay-ietf

# Monitor routes
show isis route
show ip route isis

# Debug (use carefully in production)
debug isis spf-events
debug isis route-events
```

## Production Deployment Notes

- Test configurations in lab environment first
- Adjust timers based on network scale and link quality
- Monitor CPU and memory usage during deployment
- Consider gradual rollout for large networks
- Implement proper change management procedures

## References

- RFC 1195: Use of OSI IS-IS for Routing in TCP/IP and Dual Environments
- RFC 5305: IS-IS Extensions for Traffic Engineering
- RFC 8405: Shortest Path First (SPF) Back-Off Delay Algorithm for Link-State IGPs
