#include "PktDef.h"

#include <cstring>

namespace
{
    // Count the number of bits set to 1 in a single byte
    unsigned char CountBitsInByte(unsigned char value)
    {
        unsigned char count = 0;

        while (value != 0)
        {
            count += (value & 0x01);
            value >>= 1;
        }

        return count;
    }

    // Build the flags byte as:
    // bit7 = Drive, bit6 = Status, bit5 = Sleep, bit4 = Ack, bit3..0 = Padding
    unsigned char BuildFlagsByte(const PktDef::Header& hdr)
    {
        unsigned char flags = 0;

        flags |= (static_cast<unsigned char>(hdr.Drive & 0x01) << 7);
        flags |= (static_cast<unsigned char>(hdr.Status & 0x01) << 6);
        flags |= (static_cast<unsigned char>(hdr.Sleep & 0x01) << 5);
        flags |= (static_cast<unsigned char>(hdr.Ack & 0x01) << 4);
        flags |= (static_cast<unsigned char>(hdr.Padding & 0x0F));

        return flags;
    }

    // Decode the flags byte into the Header bit fields
    void ParseFlagsByte(unsigned char flags, PktDef::Header& hdr)
    {
        hdr.Drive = (flags >> 7) & 0x01;
        hdr.Status = (flags >> 6) & 0x01;
        hdr.Sleep = (flags >> 5) & 0x01;
        hdr.Ack = (flags >> 4) & 0x01;
        hdr.Padding = flags & 0x0F;
    }
}

PktDef::PktDef()
{
    std::memset(&Packet.Header, 0, sizeof(Packet.Header));
    Packet.Data = nullptr;
    Packet.CRC = 0;
    RawBuffer = nullptr;
}

PktDef::PktDef(char* pData)
{
    std::memset(&Packet.Header, 0, sizeof(Packet.Header));
    Packet.Data = nullptr;
    Packet.CRC = 0;
    RawBuffer = nullptr;

    if (pData == nullptr)
    {
        return;
    }

    // Header format:
    // [0..1] = PktCount (2 bytes)
    // [2]    = flags
    // [3]    = total packet length
    unsigned short pktCount = 0;
    std::memcpy(&pktCount, pData, sizeof(unsigned short));
    Packet.Header.PktCount = pktCount;

    unsigned char flags = static_cast<unsigned char>(pData[2]);
    ParseFlagsByte(flags, Packet.Header);

    Packet.Header.Length = static_cast<unsigned char>(pData[3]);

    // Defensive check: minimum legal packet is header(4) + crc(1)
    if (Packet.Header.Length < (HEADERSIZE + 1))
    {
        Packet.Header.Length = 0;
        return;
    }

    // Save a copy of the raw packet
    RawBuffer = new char[Packet.Header.Length];
    std::memcpy(RawBuffer, pData, Packet.Header.Length);

    // Body size = total length - header - crc
    int bodySize = static_cast<int>(Packet.Header.Length) - HEADERSIZE - 1;

    if (bodySize > 0)
    {
        Packet.Data = new char[bodySize];
        std::memcpy(Packet.Data, pData + HEADERSIZE, bodySize);
    }

    Packet.CRC = pData[Packet.Header.Length - 1];
}

PktDef::~PktDef()
{
    if (Packet.Data != nullptr)
    {
        delete[] Packet.Data;
        Packet.Data = nullptr;
    }

    if (RawBuffer != nullptr)
    {
        delete[] RawBuffer;
        RawBuffer = nullptr;
    }
}

void PktDef::SetCmd(CmdType cmd)
{
    // Drive, Status, Sleep should not all be set together.
    // Keep Ack unchanged because ACK is separate meaning.
    Packet.Header.Drive = 0;
    Packet.Header.Status = 0;
    Packet.Header.Sleep = 0;

    switch (cmd)
    {
    case DRIVE:
        Packet.Header.Drive = 1;
        break;

    case SLEEP:
        Packet.Header.Sleep = 1;
        break;

    case RESPONSE:
        Packet.Header.Status = 1;
        break;

    default:
        break;
    }
}

void PktDef::SetBodyData(char* pData, int size)
{
    if (Packet.Data != nullptr)
    {
        delete[] Packet.Data;
        Packet.Data = nullptr;
    }

    if (pData != nullptr && size > 0)
    {
        Packet.Data = new char[size];
        std::memcpy(Packet.Data, pData, size);
        Packet.Header.Length = static_cast<unsigned char>(HEADERSIZE + size + 1);
    }
    else
    {
        Packet.Header.Length = static_cast<unsigned char>(HEADERSIZE + 1);
    }
}

void PktDef::SetPktCount(int count)
{
    Packet.Header.PktCount = static_cast<unsigned short>(count);
}

void PktDef::SetAck(bool ack)
{
    Packet.Header.Ack = ack ? 1 : 0;
}

PktDef::CmdType PktDef::GetCmd()
{
    if (Packet.Header.Status == 1)
    {
        return RESPONSE;
    }

    if (Packet.Header.Sleep == 1)
    {
        return SLEEP;
    }

    return DRIVE;
}

bool PktDef::GetAck()
{
    return (Packet.Header.Ack != 0);
}

int PktDef::GetLength()
{
    return static_cast<int>(Packet.Header.Length);
}

char* PktDef::GetBodyData()
{
    return Packet.Data;
}

int PktDef::GetPktCount()
{
    return static_cast<int>(Packet.Header.PktCount);
}

bool PktDef::CheckCRC(char* pData, int size)
{
    if (pData == nullptr || size < (HEADERSIZE + 1))
    {
        return false;
    }

    // CRC is last byte. Count bits in all prior bytes only.
    unsigned int count = 0;

    for (int i = 0; i < size - 1; ++i)
    {
        count += CountBitsInByte(static_cast<unsigned char>(pData[i]));
    }

    unsigned char calculatedCRC = static_cast<unsigned char>(count & 0xFF);
    unsigned char packetCRC = static_cast<unsigned char>(pData[size - 1]);

    return (calculatedCRC == packetCRC);
}

void PktDef::CalcCRC()
{
    unsigned int count = 0;

    // PktCount (2 bytes)
    unsigned char pktCountBytes[2] = { 0 };
    std::memcpy(pktCountBytes, &Packet.Header.PktCount, sizeof(unsigned short));
    count += CountBitsInByte(pktCountBytes[0]);
    count += CountBitsInByte(pktCountBytes[1]);

    // Flags (1 byte)
    unsigned char flags = BuildFlagsByte(Packet.Header);
    count += CountBitsInByte(flags);

    // Length (1 byte)
    count += CountBitsInByte(static_cast<unsigned char>(Packet.Header.Length));

    // Body (if any)
    int bodySize = static_cast<int>(Packet.Header.Length) - HEADERSIZE - 1;
    if (Packet.Data != nullptr && bodySize > 0)
    {
        for (int i = 0; i < bodySize; ++i)
        {
            count += CountBitsInByte(static_cast<unsigned char>(Packet.Data[i]));
        }
    }

    Packet.CRC = static_cast<char>(count & 0xFF);
}

char* PktDef::GenPacket()
{
    if (RawBuffer != nullptr)
    {
        delete[] RawBuffer;
        RawBuffer = nullptr;
    }

    // If length was never set, at least produce a header + crc packet
    if (Packet.Header.Length < (HEADERSIZE + 1))
    {
        Packet.Header.Length = static_cast<unsigned char>(HEADERSIZE + 1);
    }

    // Calculate CRC from current object state first
    CalcCRC();

    RawBuffer = new char[Packet.Header.Length];
    std::memset(RawBuffer, 0, Packet.Header.Length);

    // [0..1] PktCount
    std::memcpy(RawBuffer, &Packet.Header.PktCount, sizeof(unsigned short));

    // [2] flags
    RawBuffer[2] = static_cast<char>(BuildFlagsByte(Packet.Header));

    // [3] total length
    RawBuffer[3] = static_cast<char>(Packet.Header.Length);

    // [4..] body
    int bodySize = static_cast<int>(Packet.Header.Length) - HEADERSIZE - 1;
    if (Packet.Data != nullptr && bodySize > 0)
    {
        std::memcpy(RawBuffer + HEADERSIZE, Packet.Data, bodySize);
    }

    // last byte = CRC
    RawBuffer[Packet.Header.Length - 1] = Packet.CRC;

    return RawBuffer;
}