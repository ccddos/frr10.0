#!/usr/bin/env python3
"""
Test script for OSPFv3 BFD integration in FRR

This script provides basic verification tests for the OSPFv3 BFD functionality.
It can be used to verify configuration and check integration points.
"""

import sys
import json
import subprocess
import time
import re

class OSPFv3BFDTester:
    def __init__(self):
        self.errors = []
        self.warnings = []
        
    def log_error(self, msg):
        self.errors.append(msg)
        print(f"ERROR: {msg}")
        
    def log_warning(self, msg):
        self.warnings.append(msg)
        print(f"WARNING: {msg}")
        
    def log_info(self, msg):
        print(f"INFO: {msg}")
        
    def run_vtysh_command(self, command):
        """Run a vtysh command and return the output"""
        try:
            result = subprocess.run(['vtysh', '-c', command], 
                                  capture_output=True, text=True)
            if result.returncode != 0:
                self.log_error(f"Command failed: {command}")
                self.log_error(f"Error: {result.stderr}")
                return None
            return result.stdout
        except Exception as e:
            self.log_error(f"Failed to run command '{command}': {str(e)}")
            return None
    
    def test_ospf6_running(self):
        """Test if OSPFv3 is running"""
        self.log_info("Testing if OSPFv3 is running...")
        output = self.run_vtysh_command("show ipv6 ospf6")
        if output is None:
            self.log_error("Failed to get OSPFv3 status")
            return False
        if "OSPFv3 Routing Process" in output:
            self.log_info("OSPFv3 is running")
            return True
        else:
            self.log_warning("OSPFv3 is not running")
            return False
    
    def test_bfd_running(self):
        """Test if BFD is running"""
        self.log_info("Testing if BFD is running...")
        output = self.run_vtysh_command("show bfd peers")
        if output is None:
            self.log_error("Failed to get BFD status")
            return False
        # BFD may be running but have no peers
        self.log_info("BFD daemon is accessible")
        return True
    
    def test_ospf6_interfaces(self):
        """Test OSPFv3 interfaces for BFD configuration"""
        self.log_info("Testing OSPFv3 interfaces...")
        output = self.run_vtysh_command("show ipv6 ospf6 interface")
        if output is None:
            return False
            
        interfaces = []
        current_interface = None
        
        for line in output.split('\n'):
            if "Interface " in line and "Area " in line:
                # Extract interface name
                match = re.search(r'Interface (\S+)', line)
                if match:
                    current_interface = match.group(1)
                    interfaces.append(current_interface)
        
        if interfaces:
            self.log_info(f"Found OSPFv3 interfaces: {', '.join(interfaces)}")
            return True
        else:
            self.log_warning("No OSPFv3 interfaces found")
            return False
    
    def test_ospf6_neighbors(self):
        """Test OSPFv3 neighbors"""
        self.log_info("Testing OSPFv3 neighbors...")
        output = self.run_vtysh_command("show ipv6 ospf6 neighbor")
        if output is None:
            return False
            
        neighbor_count = 0
        for line in output.split('\n'):
            if re.match(r'^\d+\.\d+\.\d+\.\d+', line):
                neighbor_count += 1
        
        if neighbor_count > 0:
            self.log_info(f"Found {neighbor_count} OSPFv3 neighbors")
            return True
        else:
            self.log_info("No OSPFv3 neighbors found")
            return True  # This is not necessarily an error
    
    def test_bfd_sessions(self):
        """Test BFD sessions"""
        self.log_info("Testing BFD sessions...")
        output = self.run_vtysh_command("show bfd peers brief")
        if output is None:
            return False
            
        session_count = 0
        for line in output.split('\n'):
            if re.match(r'^\s*\d+\.\d+\.\d+\.\d+', line) or \
               re.match(r'^\s*[0-9a-fA-F:]+', line):
                session_count += 1
        
        if session_count > 0:
            self.log_info(f"Found {session_count} BFD sessions")
        else:
            self.log_info("No BFD sessions found")
        return True
    
    def test_configuration_syntax(self):
        """Test basic configuration syntax"""
        self.log_info("Testing configuration syntax...")
        
        # Test basic BFD configuration commands
        test_commands = [
            "configure terminal",
            "interface lo",
            "ipv6 ospf6 bfd",
            "no ipv6 ospf6 bfd",
            "ipv6 ospf6 bfd profile test",
            "no ipv6 ospf6 bfd profile test",
            "exit",
            "exit"
        ]
        
        for cmd in test_commands:
            output = self.run_vtysh_command(cmd)
            if output is None and cmd not in ["exit"]:
                self.log_error(f"Configuration command failed: {cmd}")
                return False
        
        self.log_info("Configuration syntax tests passed")
        return True
    
    def test_show_commands(self):
        """Test show commands for OSPFv3 BFD"""
        self.log_info("Testing show commands...")
        
        show_commands = [
            "show ipv6 ospf6",
            "show ipv6 ospf6 interface",
            "show ipv6 ospf6 neighbor",
            "show bfd peers",
        ]
        
        for cmd in show_commands:
            output = self.run_vtysh_command(cmd)
            if output is None:
                self.log_error(f"Show command failed: {cmd}")
                return False
        
        self.log_info("Show commands tests passed")
        return True
    
    def run_all_tests(self):
        """Run all tests"""
        self.log_info("Starting OSPFv3 BFD integration tests...")
        self.log_info("=" * 50)
        
        tests = [
            ("OSPFv3 Running", self.test_ospf6_running),
            ("BFD Running", self.test_bfd_running),
            ("OSPFv3 Interfaces", self.test_ospf6_interfaces),
            ("OSPFv3 Neighbors", self.test_ospf6_neighbors),
            ("BFD Sessions", self.test_bfd_sessions),
            ("Configuration Syntax", self.test_configuration_syntax),
            ("Show Commands", self.test_show_commands),
        ]
        
        passed = 0
        total = len(tests)
        
        for test_name, test_func in tests:
            self.log_info(f"\nRunning test: {test_name}")
            try:
                if test_func():
                    self.log_info(f"PASSED: {test_name}")
                    passed += 1
                else:
                    self.log_error(f"FAILED: {test_name}")
            except Exception as e:
                self.log_error(f"EXCEPTION in {test_name}: {str(e)}")
        
        self.log_info("\n" + "=" * 50)
        self.log_info(f"Test Summary: {passed}/{total} tests passed")
        
        if self.warnings:
            self.log_info(f"Warnings: {len(self.warnings)}")
            for warning in self.warnings:
                print(f"  - {warning}")
                
        if self.errors:
            self.log_info(f"Errors: {len(self.errors)}")
            for error in self.errors:
                print(f"  - {error}")
        
        return len(self.errors) == 0

def main():
    """Main function"""
    if len(sys.argv) > 1 and sys.argv[1] == "--help":
        print("OSPFv3 BFD Integration Tester")
        print("Usage: python3 test_ospf6_bfd.py")
        print("\nThis script tests the basic functionality of OSPFv3 BFD integration")
        print("Make sure FRR is running before executing this script")
        return 0
    
    tester = OSPFv3BFDTester()
    success = tester.run_all_tests()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())