#include "pch.h"
#include "CppUnitTest.h"
#include "C:\Users\milan\Downloads\Milan Shoni Project files\COIL_Progect_Group_25/PktDef.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PktDefUnitTests
{
    TEST_CLASS(PktDefHelperAndConstructorTests)
    {
    public:

        TEST_METHOD(Constructor_FromRawData_ParsesDrivePacketCorrectly)
        {
            PktDef original;

            original.SetCmd(PktDef::DRIVE);
            original.SetPktCount(42);
            original.SetAck(true);

            char body[3];
            body[0] = (char)PktDef::FORWARD;
            body[1] = 10;
            body[2] = 90;
            original.SetBodyData(body, 3);

            char* raw = original.GenPacket();

            PktDef parsed(raw);

            Assert::AreEqual(42, parsed.GetPktCount());
            Assert::AreEqual((int)PktDef::DRIVE, (int)parsed.GetCmd());
            Assert::IsTrue(parsed.GetAck());
            Assert::AreEqual(PktDef::HEADERSIZE + 3 + 1, parsed.GetLength());
            Assert::IsNotNull(parsed.GetBodyData());

            char* parsedBody = parsed.GetBodyData();
            Assert::AreEqual((int)PktDef::FORWARD, (int)parsedBody[0]);
            Assert::AreEqual(10, (int)parsedBody[1]);
            Assert::AreEqual(90, (int)parsedBody[2]);
        }

        TEST_METHOD(Constructor_FromRawData_ParsesSleepPacketCorrectly)
        {
            PktDef original;

            original.SetCmd(PktDef::SLEEP);
            original.SetPktCount(7);
            original.SetAck(true);

            char* raw = original.GenPacket();

            PktDef parsed(raw);

            Assert::AreEqual(7, parsed.GetPktCount());
            Assert::AreEqual((int)PktDef::SLEEP, (int)parsed.GetCmd());
            Assert::IsTrue(parsed.GetAck());
            Assert::AreEqual(PktDef::HEADERSIZE + 1, parsed.GetLength());
            Assert::IsNull(parsed.GetBodyData());
        }

        TEST_METHOD(Constructor_FromRawData_ParsesResponsePacketCorrectly)
        {
            PktDef original;

            original.SetCmd(PktDef::RESPONSE);
            original.SetPktCount(99);
            original.SetAck(true);

            char body[4] = { 'O', 'K', '!', '\0' };
            original.SetBodyData(body, 4);

            char* raw = original.GenPacket();

            PktDef parsed(raw);

            Assert::AreEqual(99, parsed.GetPktCount());
            Assert::AreEqual((int)PktDef::RESPONSE, (int)parsed.GetCmd());
            Assert::IsTrue(parsed.GetAck());
            Assert::AreEqual(PktDef::HEADERSIZE + 4 + 1, parsed.GetLength());
            Assert::IsNotNull(parsed.GetBodyData());

            char* parsedBody = parsed.GetBodyData();
            Assert::AreEqual((int)'O', (int)parsedBody[0]);
            Assert::AreEqual((int)'K', (int)parsedBody[1]);
            Assert::AreEqual((int)'!', (int)parsedBody[2]);
            Assert::AreEqual((int)'\0', (int)parsedBody[3]);
        }

        TEST_METHOD(Constructor_NullRawPointer_LeavesObjectSafe)
        {
            PktDef parsed(nullptr);

            Assert::AreEqual(0, parsed.GetPktCount());
            Assert::AreEqual(0, parsed.GetLength());
            Assert::IsFalse(parsed.GetAck());
            Assert::IsNull(parsed.GetBodyData());
        }

        TEST_METHOD(Constructor_InvalidTooSmallLength_LeavesPacketInvalid)
        {
            char raw[5] = {};

            raw[0] = 1;
            raw[1] = 0;
            raw[2] = 0;
            raw[3] = 2;
            raw[4] = 0;

            PktDef parsed(raw);

            Assert::AreEqual(1, parsed.GetPktCount());
            Assert::AreEqual(0, parsed.GetLength());
            Assert::IsNull(parsed.GetBodyData());
        }

        TEST_METHOD(Helper_CountBitsInByte_IsTestedIndirectlyThroughCheckCRC_ValidPacket)
        {
            PktDef pkt;

            pkt.SetCmd(PktDef::DRIVE);
            pkt.SetPktCount(3);

            char body[3];
            body[0] = (char)PktDef::BACKWARD;
            body[1] = 8;
            body[2] = 95;
            pkt.SetBodyData(body, 3);

            char* raw = pkt.GenPacket();

            Assert::IsTrue(pkt.CheckCRC(raw, pkt.GetLength()));
        }

        TEST_METHOD(Helper_CountBitsInByte_IsTestedIndirectlyThroughCheckCRC_CorruptPacket)
        {
            PktDef pkt;

            pkt.SetCmd(PktDef::DRIVE);
            pkt.SetPktCount(10);

            char body[3];
            body[0] = (char)PktDef::FORWARD;
            body[1] = 5;
            body[2] = 80;
            pkt.SetBodyData(body, 3);

            char* raw = pkt.GenPacket();
            Assert::IsTrue(pkt.CheckCRC(raw, pkt.GetLength()));

            raw[4] = (char)PktDef::BACKWARD;

            Assert::IsFalse(pkt.CheckCRC(raw, pkt.GetLength()));
        }

        TEST_METHOD(Helper_BuildFlagsByte_IsTestedIndirectlyThroughGenPacket)
        {
            PktDef pkt;

            pkt.SetCmd(PktDef::SLEEP);
            pkt.SetPktCount(12);
            pkt.SetAck(true);

            char* raw = pkt.GenPacket();

            unsigned char flags = (unsigned char)raw[2];

            Assert::AreEqual((unsigned char)0x30, flags);
        }

        TEST_METHOD(Helper_ParseFlagsByte_IsTestedIndirectlyThroughParsingConstructor)
        {
            PktDef original;

            original.SetCmd(PktDef::SLEEP);
            original.SetPktCount(88);
            original.SetAck(true);

            char* raw = original.GenPacket();

            PktDef parsed(raw);

            Assert::AreEqual((int)PktDef::SLEEP, (int)parsed.GetCmd());
            Assert::IsTrue(parsed.GetAck());
        }

        TEST_METHOD(Helper_BuildFlagsAndParseFlags_WorkTogether_ForResponseAck)
        {
            PktDef original;

            original.SetCmd(PktDef::RESPONSE);
            original.SetPktCount(55);
            original.SetAck(true);

            char* raw = original.GenPacket();
            PktDef parsed(raw);

            Assert::AreEqual((int)PktDef::RESPONSE, (int)parsed.GetCmd());
            Assert::IsTrue(parsed.GetAck());
            Assert::AreEqual(55, parsed.GetPktCount());
        }
    };
}