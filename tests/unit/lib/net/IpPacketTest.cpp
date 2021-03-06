/*
 *  Copyright 2019 Carnegie Technologies
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <gtest/gtest.h>

#include "basic/IpAddress.hpp"
#include "net/IpHeaders.hpp"
#include "net/IpPacket.hpp"
#include "net/UdpPacket.hpp"
#include "net/TcpPacket.hpp"
#include "net/IcmpPacket.hpp"
#include "net/IpChecksum.hpp"

using namespace Pravala;

/// @brief IpPacket tests
class IpPacketTest: public ::testing::Test
{
    protected:
        /// @brief Test if the given data can be parsed as an IP packet, then verifies some parameters.
        /// @param [in] data The raw packet data to parse.
        /// @param [in] dataLen The length of the raw data.
        /// @param [in] ipVer The expected IP version in the packet.
        /// @param [in] srcAddr The expected source address in the packet.
        /// @param [in] dstAddr The expected destination address in the packet.
        /// @param [in] checkSum The expected payload checksum in the packet.
        /// @tparam PacketType The packet subclass of IpPacket to decode the data as.
        ///                    This class must define a static const ProtoNumber property with the
        ///                     IpPacket::Proto::Number that the class represents.
        ///                    And it must define a Header struct with the binary layout of the packet
        ///                     header that the class represents.
        template<typename PacketType> static void testIpPacket (
            const unsigned char * data,
            size_t dataLen,
            uint8_t ipVer,
            const char * srcAddr,
            const char * dstAddr,
            uint16_t checkSum )
        {
            MemHandle m ( dataLen );
            size_t offset = 0;
            EXPECT_TRUE ( m.setData ( offset, data, dataLen ) );

            IpPacket p ( m );
            IpAddress sAddr, dAddr;

            // Validate the packet constructed from raw data has correct info
            EXPECT_TRUE ( p.isValid() );
            EXPECT_EQ ( ipVer, p.getIpVersion() );
            EXPECT_TRUE ( p.is ( PacketType::ProtoNumber ) );
            ASSERT_NE ( ( const typename PacketType::Header * ) 0, p.getProtoHeader<PacketType>() );

            EXPECT_TRUE ( p.getAddr ( sAddr, dAddr ) );
            EXPECT_STREQ ( srcAddr, sAddr.toString ( false ).c_str() );
            EXPECT_STREQ ( dstAddr, dAddr.toString ( false ).c_str() );
            EXPECT_EQ ( checkSum, p.getProtoHeader<PacketType>()->getChecksum() );

            // Validate checksum calculation (using a new copy of the packet to avoid changing the original)
            IpPacket pCopy ( p );
            pCopy.getWritableProtoHeader<PacketType>()->checksum = 0;

            if ( p.is ( IpPacket::Proto::ICMP ) )
            {
                // ICMP does checksums differently
                MemVector payload;
                ASSERT_TRUE ( pCopy.getProtoPayload<PacketType> ( payload ) );

                IpChecksum cSum;
                cSum.addMemory ( pCopy.getProtoHeader<PacketType>(), sizeof ( typename PacketType::Header ) );
                cSum.addMemory ( payload );

                EXPECT_EQ ( p.getProtoHeader<PacketType>()->getChecksum(),
                            htons ( cSum.getChecksum() ) );
            }
            else
            {
                EXPECT_EQ ( p.getProtoHeader<PacketType>()->getChecksum(),
                            htons ( pCopy.calcPseudoHeaderPayloadChecksum() ) );
            }

            m.clear();

            // Validate the data exported from packet matches original raw data
            EXPECT_TRUE ( p.getPacketData().storeContinuous ( m ) );
            EXPECT_EQ ( dataLen, m.size() );
            EXPECT_EQ ( 0, memcmp ( m.get(), data, dataLen ) );
        }
};

TEST_F ( IpPacketTest, IpPacketCreateInvalid )
{
    // Test that we cannot create a packet with invalid data (correct min size, but all zeroes):

    MemHandle m ( sizeof ( struct ip ) );
    m.setZero();

    IpPacket p ( m );

    EXPECT_FALSE ( p.isValid() );
}

TEST_F ( IpPacketTest, IpPacketParseV4Udp )
{
    // Test a sample IPv4 UDP packet exported from a packet capture

    static const unsigned char data[] =
    {
        // IPv4 header
        0x45, 0x00, 0x00, 0x48, 0xc7, 0x6a, 0x40, 0x00,
        0x40, 0x11, 0x4e, 0x64, 0xc0, 0xa8, 0x51, 0x86,
        0xc0, 0xa8, 0x51, 0xff,
        // UDP header
        0xe1, 0x15, 0xe1, 0x15, 0x00, 0x34, 0xd0, 0x56,
        // Payload
        0x53, 0x70, 0x6f, 0x74, 0x55, 0x64, 0x70, 0x30,
        0x6b, 0x2b, 0xdb, 0x6e, 0x5c, 0x80, 0xc3, 0xae,
        0x00, 0x01, 0x00, 0x04, 0x48, 0x95, 0xc2, 0x03,
        0x5e, 0xad, 0x3b, 0xf6, 0x48, 0x08, 0xec, 0x72,
        0x46, 0xb9, 0x6f, 0x76, 0x5b, 0x22, 0x11, 0x84,
        0x15, 0x1f, 0x47, 0x38
    };

    testIpPacket<UdpPacket> (
        data, sizeof ( data ),
        4, "192.168.81.134", "192.168.81.255", 0xd056 );
}

TEST_F ( IpPacketTest, IpPacketParseV6Udp )
{
    // Test a sample IPv6 UDP packet exported from a packet capture

    static const unsigned char data[] =
    {
        // IPv6 header
        0x60, 0x0c, 0x15, 0x0f, 0x00, 0x4a, 0x11, 0xff,
        0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x1c, 0x11, 0x88, 0x78, 0x52, 0x6c, 0x4b, 0x11,
        0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb,
        // UDP header
        0x14, 0xe9, 0x14, 0xe9, 0x00, 0x4a, 0x99, 0x1a,
        // Payload
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x08, 0x5f, 0x68, 0x6f,
        0x6d, 0x65, 0x6b, 0x69, 0x74, 0x04, 0x5f, 0x74,
        0x63, 0x70, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
        0x00, 0x00, 0x0c, 0x80, 0x01, 0x00, 0x00, 0x29,
        0x05, 0xa0, 0x00, 0x00, 0x11, 0x94, 0x00, 0x12,
        0x00, 0x04, 0x00, 0x0e, 0x00, 0xd0, 0x22, 0xdb,
        0x70, 0xe2, 0xbc, 0xeb, 0x00, 0xdb, 0x70, 0xe2,
        0xbc, 0xeb
    };

    testIpPacket<UdpPacket> (
        data, sizeof ( data ),
        6, "fe80::1c11:8878:526c:4b11", "ff02::fb", 0x991a );
}

TEST_F ( IpPacketTest, IpPacketParseV4Tcp )
{
    // Test a sample IPv4 TCP packet exported from a packet capture

    static const unsigned char data[] =
    {
        // IPv4 header
        0x45, 0x00, 0x00, 0x61, 0x4d, 0xfb, 0x40, 0x00,
        0x6f, 0x06, 0x6a, 0xb4, 0x34, 0x6d, 0x0c, 0x4a,
        0xc0, 0xa8, 0x51, 0x88,
        // TCP header
        0x01, 0xbb, 0xdb, 0xd3, 0xbd, 0xd8, 0xb5, 0xdc,
        0xfc, 0xc7, 0x3e, 0x81, 0x80, 0x18, 0x04, 0x04,
        0x59, 0x1f, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a,
        0x01, 0x64, 0x34, 0x16, 0x53, 0x88, 0x9e, 0x59,
        // Payload
        0x17, 0x03, 0x03, 0x00, 0x28, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x61, 0xf0, 0xc3, 0x07,
        0xb2, 0xc1, 0x9b, 0xd4, 0x65, 0x1d, 0x23, 0x89,
        0x04, 0x8a, 0x0f, 0x38, 0x67, 0x71, 0x9c, 0x94,
        0x3e, 0xa0, 0xa1, 0x8e, 0xb9, 0x3d, 0x05, 0xfc,
        0x8d, 0x94, 0x07, 0x96, 0x88
    };

    testIpPacket<TcpPacket> (
        data, sizeof ( data ),
        4, "52.109.12.74", "192.168.81.136", 0x591f );
}

TEST_F ( IpPacketTest, IpPacketParseV6Tcp )
{
    // Test a sample IPv6 TCP packet exported from a packet capture

    static const unsigned char data[] =
    {
        // IPv6 header
        0x60, 0x01, 0xe8, 0x91, 0x00, 0x3e, 0x06, 0x38,
        0x26, 0x20, 0x00, 0x00, 0x08, 0x61, 0xed, 0x1a,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x26, 0x07, 0xfe, 0xa8, 0x1c, 0xdf, 0xea, 0xd0,
        0x50, 0x86, 0xf9, 0x2f, 0xb4, 0xce, 0xbc, 0x4b,
        // TCP header
        0x01, 0xbb, 0xd6, 0x5a, 0x9d, 0x9f, 0x85, 0x82,
        0x1f, 0xf3, 0x61, 0xe0, 0x80, 0x18, 0x00, 0x3c,
        0xe3, 0x5b, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a,
        0x86, 0x52, 0x2c, 0x50, 0x4e, 0x68, 0x70, 0xb0,
        // Payload
        0x17, 0x03, 0x03, 0x00, 0x19, 0x5e, 0x37, 0x24,
        0xb4, 0x54, 0xc0, 0x48, 0x67, 0x38, 0xbd, 0xab,
        0x9b, 0x22, 0x60, 0x17, 0x51, 0x51, 0x33, 0x86,
        0x74, 0xa3, 0x8a, 0xca, 0x1d, 0xe5
    };

    testIpPacket<TcpPacket> (
        data, sizeof ( data ),
        6, "2620:0:861:ed1a::1", "2607:fea8:1cdf:ead0:5086:f92f:b4ce:bc4b", 0xe35b );
}

TEST_F ( IpPacketTest, IpPacketParseV4UdpFragmented )
{
    // Test a sample fragmented IPv4 UDP packet exported from a packet capture

    {
        // Validate that packet fragment 1 constructed from raw data has correct info

        static const unsigned char fragment1[] =
        {
            // IPv4 header
            0x45, 0x00, 0x00, 0x38, 0x00, 0xf2, 0x20, 0x00,
            0x40, 0x11, 0xaf, 0x37, 0x0a, 0x01, 0x01, 0x01,
            0x81, 0x6f, 0x1e, 0x1b,
            // Data
            0x7c, 0xab, 0x4e, 0xe5, 0x00, 0x24, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };

        MemHandle m1 ( sizeof ( fragment1 ) );
        size_t offset = 0;
        EXPECT_TRUE ( m1.setData ( offset, fragment1, sizeof ( fragment1 ) ) );

        IpPacket p1 ( m1 );
        IpAddress srcAddr, dstAddr;

        EXPECT_TRUE ( p1.isValid() );
        EXPECT_EQ ( 4, p1.getIpVersion() );
        EXPECT_TRUE ( p1.is ( IpPacket::Proto::UDP ) );
        ASSERT_NE ( ( const UdpPacket::Header * ) 0, p1.getProtoHeader<UdpPacket>() );

        EXPECT_TRUE ( p1.getAddr ( srcAddr, dstAddr ) );
        EXPECT_STREQ ( "10.1.1.1", srcAddr.toString ( false ).c_str() );
        EXPECT_STREQ ( "129.111.30.27", dstAddr.toString ( false ).c_str() );
    }

    {
        // Validate that packet fragment 2 constructed from raw data has correct info

        static const unsigned char fragment2[] =
        {
            // IPv4 header
            0x45, 0x00, 0x00, 0x18, 0x00, 0xf2, 0x00, 0x03,
            0x40, 0x11, 0xcf, 0x54, 0x0a, 0x01, 0x01, 0x01,
            0x81, 0x6f, 0x1e, 0x1b,
            // Data
            0x7c, 0xab, 0x4e, 0xe5, 0x00, 0x24, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };

        MemHandle m2 ( sizeof ( fragment2 ) );
        size_t offset = 0;
        EXPECT_TRUE ( m2.setData ( offset, fragment2, sizeof ( fragment2 ) ) );

        IpPacket p2 ( m2 );
        IpAddress srcAddr, dstAddr;

        EXPECT_TRUE ( p2.isValid() );
        EXPECT_EQ ( 4, p2.getIpVersion() );

        EXPECT_TRUE ( p2.getAddr ( srcAddr, dstAddr ) );
        EXPECT_STREQ ( "10.1.1.1", srcAddr.toString ( false ).c_str() );
        EXPECT_STREQ ( "129.111.30.27", dstAddr.toString ( false ).c_str() );

        EXPECT_TRUE ( p2.getProtoType() & IpPacket::ProtoBitNextIpFragment );
    }
}

TEST_F ( IpPacketTest, IpPacketParseV4Icmp )
{
    // Test a sample IPv4 ICMP packet exported from a packet capture

    static const unsigned char data[] =
    {
        // IPv4 header
        0x45, 0x00, 0x00, 0x54, 0xc7, 0xe2, 0x00, 0x00,
        0x3a, 0x01, 0x96, 0x86, 0x08, 0x08, 0x08, 0x08,
        0xc0, 0xa8, 0x51, 0x88,
        // ICMP data
        0x00, 0x00, 0x3f, 0xdd, 0xe6, 0x7b, 0x00, 0x00,
        0x5b, 0x47, 0x75, 0x06, 0x00, 0x02, 0x1e, 0x54,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
    };

    testIpPacket<IcmpPacket> (
        data, sizeof ( data ),
        4, "8.8.8.8", "192.168.81.136", 0x3fdd );
}

TEST_F ( IpPacketTest, IpPacketParseV6Icmp )
{
    // Test a sample IPv6 ICMP packet exported from a packet capture

    static const unsigned char data[] =
    {
        // IPv6 header
        0x60, 0x00, 0x00, 0x00, 0x00, 0x88, 0x3a, 0xff,
        0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
        0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        // ICMP data
        0x86, 0x00, 0x02, 0x26, 0x40, 0xc0, 0x00, 0x3c,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x04, 0x40, 0xe0, 0x00, 0x01, 0x51, 0x80,
        0x00, 0x00, 0x38, 0x40, 0x00, 0x00, 0x00, 0x00,
        0x26, 0x07, 0xfe, 0xa8, 0x1c, 0xdf, 0xea, 0xd0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x18, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x19, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
        0x26, 0x07, 0xfe, 0xa8, 0x1c, 0xdf, 0xea, 0xd0,
        0x20, 0x30, 0xa6, 0xff, 0xfe, 0x14, 0x08, 0x6f,
        0x1f, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14,
        0x0b, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64, 0x6f,
        0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
        0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0xdc,
        0x01, 0x01, 0x22, 0x30, 0xa6, 0x14, 0x08, 0x6f
    };

    MemHandle m ( sizeof ( data ) );
    size_t offset = 0;
    EXPECT_TRUE ( m.setData ( offset, data, sizeof ( data ) ) );

    IpPacket p ( m );
    IpAddress sAddr, dAddr;

    // Validate the packet constructed from raw data has correct info
    EXPECT_TRUE ( p.isValid() );
    EXPECT_EQ ( 6, p.getIpVersion() );
    EXPECT_TRUE ( p.is ( IpPacket::Proto::IPv6ICMP ) );

    EXPECT_TRUE ( p.getAddr ( sAddr, dAddr ) );
    EXPECT_STREQ ( "fe80::1:1", sAddr.toString ( false ).c_str() );
    EXPECT_STREQ ( "ff02::1", dAddr.toString ( false ).c_str() );

    m.clear();

    // Validate the data exported from packet matches original raw data
    EXPECT_TRUE ( p.getPacketData().storeContinuous ( m ) );
    EXPECT_EQ ( sizeof ( data ), m.size() );
    EXPECT_EQ ( 0, memcmp ( m.get(), data, sizeof ( data ) ) );
}
