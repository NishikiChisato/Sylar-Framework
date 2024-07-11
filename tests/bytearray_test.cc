#include "../src/include/bytearray.hh"
#include <gtest/gtest.h>
#include <variant>
#include <vector>

TEST(ByteArray, BasicReadWrite1) {
  Sylar::ByteArray arr(8);
  const size_t sz = 64;
  char buf[sz];
  memset(buf, 0xff, sizeof(buf));
  arr.Write(buf, 64);
  arr.Write(buf, 64);
  arr.Write(buf, 32);
  arr.Write(buf, 32);
  char rcv[3 * sz];
  memset(rcv, 0, sizeof(rcv));
  arr.Read(rcv, 3 * sz);
  for (int byte = 0; byte < 3 * sz; byte += 1) {
    EXPECT_EQ(*(uint8_t *)(rcv + byte), 0xff);
  }
}

TEST(ByteArray, BasicReadWrite2) {
  Sylar::ByteArray arr(8);
  uint8_t a1 = 1;
  uint16_t b1 = 2;
  uint32_t c1 = 3;
  uint64_t d1 = 4;
  int8_t a2 = 5;
  int16_t b2 = 6;
  int32_t c2 = 7;
  int64_t d2 = 8;

  std::string str = "hello world";

  arr.Write(&a1, sizeof(a1));
  arr.Write(&b1, sizeof(b1));
  arr.Write(&c1, sizeof(c1));
  arr.Write(&d1, sizeof(d1));
  arr.Write(&a2, sizeof(a2));
  arr.Write(&b2, sizeof(b2));
  arr.Write(&c2, sizeof(c2));
  arr.Write(&d2, sizeof(d2));
  arr.Write(&str[0], str.length());

  char buf[64];
  memset(buf, 0, sizeof(buf));

  arr.Read(buf, sizeof(a1));
  EXPECT_EQ(*(uint8_t *)buf, a1);
  arr.Read(buf, sizeof(b1));
  EXPECT_EQ(*(uint16_t *)buf, b1);
  arr.Read(buf, sizeof(c1));
  EXPECT_EQ(*(uint32_t *)buf, c1);
  arr.Read(buf, sizeof(d1));
  EXPECT_EQ(*(uint64_t *)buf, d1);

  arr.Read(buf, sizeof(a2));
  EXPECT_EQ(*(int8_t *)buf, a2);
  arr.Read(buf, sizeof(b2));
  EXPECT_EQ(*(int16_t *)buf, b2);
  arr.Read(buf, sizeof(c2));
  EXPECT_EQ(*(int32_t *)buf, c2);
  arr.Read(buf, sizeof(d2));
  EXPECT_EQ(*(int64_t *)buf, d2);

  memset(buf, 0, sizeof(buf));
  arr.Read(buf, str.length());
  for (size_t byte = 0; byte < str.length(); byte++) {
    EXPECT_EQ(buf[byte], str[byte]);
  }
}

TEST(ByteArray, IntReadWriteFixed) {
  Sylar::ByteArray::ptr ba(new Sylar::ByteArray(8));

  ba->WriteFuint8(1);
  ba->WriteFint8(2);
  ba->WriteFuint16(3);
  ba->WriteFint16(4);
  ba->WriteFuint32(5);
  ba->WriteFint32(6);
  ba->WriteFuint64(7);
  ba->WriteFint64(8);
  ba->WriteStringWithoutLength(std::string("hello world"));
  ba->WriteFloat(9.0);
  ba->WriteDouble(10.0);

  auto v1 = ba->ReadFuint8();
  auto v2 = ba->ReadFint8();
  auto v3 = ba->ReadFuint16();
  auto v4 = ba->ReadFint16();
  auto v5 = ba->ReadFuint32();
  auto v6 = ba->ReadFint32();
  auto v7 = ba->ReadFuint64();
  auto v8 = ba->ReadFint64();
  auto v9 = ba->ReadStringWithoutLength();
  auto v10 = ba->ReadFloat();
  auto v11 = ba->ReadDouble();

  EXPECT_EQ(v1, 1);
  EXPECT_EQ(v2, 2);
  EXPECT_EQ(v3, 3);
  EXPECT_EQ(v4, 4);
  EXPECT_EQ(v5, 5);
  EXPECT_EQ(v6, 6);
  EXPECT_EQ(v7, 7);
  EXPECT_EQ(v8, 8);
  EXPECT_EQ(v9, std::string("hello world"));
  EXPECT_EQ(v10, 9.0);
  EXPECT_EQ(v11, 10.0);
}

TEST(ByteArray, IntReadWriteVarint) {
  Sylar::ByteArray::ptr ba(new Sylar::ByteArray(8));

  ba->WriteVuint8(1);
  ba->WriteVint8(2);
  ba->WriteVuint16(3);
  ba->WriteVint16(4);
  ba->WriteVuint32(5);
  ba->WriteVint32(6);
  ba->WriteVuint64(7);
  ba->WriteVint64(8);
  ba->WriteStringWithoutLength(std::string("hello world"));
  ba->WriteFloat(9.0);
  ba->WriteDouble(10.0);

  auto v1 = ba->ReadVuint8();
  auto v2 = ba->ReadVint8();
  auto v3 = ba->ReadVuint16();
  auto v4 = ba->ReadVint16();
  auto v5 = ba->ReadVuint32();
  auto v6 = ba->ReadVint32();
  auto v7 = ba->ReadVuint64();
  auto v8 = ba->ReadVint64();
  auto v9 = ba->ReadStringWithoutLength();
  auto v10 = ba->ReadFloat();
  auto v11 = ba->ReadDouble();

  std::cout << "=====Memory dump output=====" << std::endl;
  ba->Dump();

  std::cout << "=====Bit stream output=====" << std::endl;
  std::string s = ba->BitsStream();
  int cnt = 0;
  for (unsigned char c : s) {
    // std::bitset<8> bit(c);
    // std::cout << bit << std::endl;
    std::cout << std::hex << +c << ' ';
    cnt++;
    if (cnt && cnt % 8 == 0) {
      std::cout << std::endl;
    }
  }

  EXPECT_EQ(v1, 1);
  EXPECT_EQ(v2, 2);
  EXPECT_EQ(v3, 3);
  EXPECT_EQ(v4, 4);
  EXPECT_EQ(v5, 5);
  EXPECT_EQ(v6, 6);
  EXPECT_EQ(v7, 7);
  EXPECT_EQ(v8, 8);
  EXPECT_EQ(v9, std::string("hello world"));
  EXPECT_EQ(v10, 9.0);
  EXPECT_EQ(v11, 10.0);
}