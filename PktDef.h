#pragma once

class PktDef
{
public:
    // Fixed header size:
    // PktCount (2 bytes) + Flags (1 byte) + Length (1 byte)
    static const int HEADERSIZE = 4;

    // Direction constants from the protocol
    static const int FORWARD = 1;
    static const int BACKWARD = 2;
    static const int RIGHT = 3;
    static const int LEFT = 4;

    // Command types required by the assignment
    enum CmdType
    {
        DRIVE,
        SLEEP,
        RESPONSE
    };

    // Packet header
    struct Header
    {
        unsigned short PktCount;

        // Flags packed into 1 byte
        unsigned char Drive : 1;
        unsigned char Status : 1;
        unsigned char Sleep : 1;
        unsigned char Ack : 1;
        unsigned char Padding : 4;

        unsigned char Length;
    };

    // Drive packet body for FORWARD / BACKWARD
    struct DriveBody
    {
        unsigned char Direction;
        unsigned char Duration;
        unsigned char Power;
    };

    // Turn packet body for LEFT / RIGHT
    struct TurnBody
    {
        unsigned char Direction;
        unsigned short Duration;
    };

    // Internal packet structure
    struct CmdPacket
    {
        Header Header;
        char* Data;
        char CRC;
    };

private:
    CmdPacket Packet;
    char* RawBuffer;

public:
    // Constructors / destructor
    PktDef();
    PktDef(char* pData);
    ~PktDef();

    // Setters
    void SetCmd(CmdType cmd);
    void SetBodyData(char* pData, int size);
    void SetPktCount(int count);
    void SetAck(bool ack);

    // Getters
    CmdType GetCmd();
    bool GetAck();
    int GetLength();
    char* GetBodyData();
    int GetPktCount();

    // CRC / packet generation
    bool CheckCRC(char* pData, int size);
    void CalcCRC();
    char* GenPacket();
};