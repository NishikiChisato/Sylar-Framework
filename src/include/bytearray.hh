#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__

#include "./endian.hh"
#include <bitset>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <type_traits>
#include <vector>

namespace Sylar {

class ByteArray {
public:
  typedef std::shared_ptr<ByteArray> ptr;

  struct Node {
    Node(size_t size = 0);
    ~Node();
    std::unique_ptr<uint8_t, void (*)(void *)> base_ptr_;
    size_t rest_size_;
    std::shared_ptr<Node> next_;
  };

  ByteArray(size_t baseSize = 0);

  ~ByteArray();

  size_t GetCapacity() { return capacity_; }

  size_t GetPosition() { return position_; }

  /**
   * @brief write to link list, this method will change the position of
   * position_
   *
   * @param buf buffer to be written
   * @param size the size of data
   */
  void Write(const void *buf, size_t size);

  /**
   * @brief read from link list, start at 0 offset
   *
   * @param buf buffer to be filled with read data
   * @param size the size of read data
   * @exception if size > capacity_, throw exception
   */
  void Read(void *buf, size_t size);

  /**
   * @brief dump all content of this byte array. we can use this method to write
   * all content of byte array to file or network
   */
  std::string BitsStream();

  template <typename T> void WriteAny(const T &val, bool isFixed = false) {
    if (std::is_same<T, uint8_t>::value) {
      if (isFixed) {
        WriteFuint8(val);
      } else {
        WriteVuint8(val);
      }
    } else if (std::is_same<T, int8_t>::value) {
      if (isFixed) {
        WriteFint8(val);
      } else {
        WriteVint8(val);
      }
    } else if (std::is_same<T, uint16_t>::value) {
      if (isFixed) {
        WriteFuint16(val);
      } else {
        WriteVuint16(val);
      }
    } else if (std::is_same<T, int16_t>::value) {
      if (isFixed) {
        WriteFint16(val);
      } else {
        WriteVint16(val);
      }
    } else if (std::is_same<T, uint32_t>::value) {
      if (isFixed) {
        WriteFuint32(val);
      } else {
        WriteVuint32(val);
      }
    } else if (std::is_same<T, int32_t>::value) {
      if (isFixed) {
        WriteFint32(val);
      } else {
        WriteVint32(val);
      }
    } else if (std::is_same<T, uint64_t>::value) {
      if (isFixed) {
        WriteFuint64(val);
      } else {
        WriteVuint64(val);
      }
    } else if (std::is_same<T, int64_t>::value) {
      if (isFixed) {
        WriteFint64(val);
      } else {
        WriteVint64(val);
      }
    } else if (std::is_same<T, std::string>::value) {
      WriteStringWithoutLength(val);
    }
  }

  void Dump();

  /**
   * @brief write fixed, 8-bits, int type to byte array
   */
  void WriteFint8(int8_t val);

  /**
   * @brief write fixed, 8-bits, uint type to byte array
   */
  void WriteFuint8(uint8_t val);

  /**
   * @brief write fixed, 16-bits, int type to byte array
   */
  void WriteFint16(int16_t val);

  /**
   * @brief write fixed, 16-bits, uint type to byte array
   */
  void WriteFuint16(uint16_t val);

  /**
   * @brief write fixed, 32-bits, int type to byte array
   */
  void WriteFint32(int32_t val);

  /**
   * @brief write fixed, 32-bits, uint type to byte array
   */
  void WriteFuint32(uint32_t val);

  /**
   * @brief write fixed, 64-bits, int type to byte array
   */
  void WriteFint64(int64_t val);

  /**
   * @brief write fixed, 64-bits, uint type to byte array
   */
  void WriteFuint64(uint64_t val);

  /**
   * @brief write varint, 8-bits, int type to byte array
   */
  void WriteVint8(int8_t val);

  /**
   * @brief write varint, 8-bits, uint type to byte array
   */
  void WriteVuint8(uint8_t val);

  /**
   * @brief write varint, 16-bits, int type to byte array
   */
  void WriteVint16(int16_t val);

  /**
   * @brief write varint, 16-bits, uint type to byte array
   */
  void WriteVuint16(uint16_t val);

  /**
   * @brief write varint, 32-bits, int type to byte array
   */
  void WriteVint32(int32_t val);

  /**
   * @brief write varint, 32-bits, uint type to byte array
   */
  void WriteVuint32(uint32_t val);

  /**
   * @brief write varint, 64-bits, int type to byte array
   */
  void WriteVint64(int64_t val);

  /**
   * @brief write varint, 64-bits, uint type to byte array
   */
  void WriteVuint64(uint64_t val);

  /**
   * @brief write float to byte array
   */
  void WriteFloat(float val);

  /**
   * @brief write double to byte array
   */
  void WriteDouble(double val);

  /**
   * @brief write string with arbitrary length
   */
  void WriteStringWithoutLength(std::string val);

  /**
   * @brief read fixed, 8-bits, int type to byte array
   */
  int8_t ReadFint8();

  /**
   * @brief read fixed, 8-bits, uint type to byte array
   */
  uint8_t ReadFuint8();

  /**
   * @brief read fixed, 16-bits, int type to byte array
   */
  int16_t ReadFint16();

  /**
   * @brief read fixed, 16-bits, uint type to byte array
   */
  uint16_t ReadFuint16();

  /**
   * @brief read fixed, 32-bits, int type to byte array
   */
  int32_t ReadFint32();

  /**
   * @brief read fixed, 32-bits, uint type to byte array
   */
  uint32_t ReadFuint32();

  /**
   * @brief read fixed, 64-bits, int type to byte array
   */
  int64_t ReadFint64();

  /**
   * @brief read fixed, 64-bits, uint type to byte array
   */
  uint64_t ReadFuint64();

  /**
   * @brief read varint, 8-bits, int type to byte array
   */
  int8_t ReadVint8();

  /**
   * @brief read varint, 8-bits, uint type to byte array
   */
  uint8_t ReadVuint8();

  /**
   * @brief read varint, 16-bits, int type to byte array
   */
  int16_t ReadVint16();

  /**
   * @brief read varint, 16-bits, uint type to byte array
   */
  uint16_t ReadVuint16();

  /**
   * @brief read varint, 32-bits, int type to byte array
   */
  int32_t ReadVint32();

  /**
   * @brief read varint, 32-bits, uint type to byte array
   */
  uint32_t ReadVuint32();

  /**
   * @brief read varint, 64-bits, int type to byte array
   */
  int64_t ReadVint64();

  /**
   * @brief read varint, 64-bits, uint type to byte array
   */
  uint64_t ReadVuint64();

  /**
   * @brief read float to byte array
   */
  float ReadFloat();

  /**
   * @brief read double to byte array
   */
  double ReadDouble();

  /**
   * @brief read string with arbitrary length
   */
  std::string ReadStringWithoutLength();

private:
  /**
   * @brief extend capacity by increasing at least delta
   *
   * @param delta increased size
   */
  void ExtendCapacity(size_t delta);

  const size_t NODE_SIZE = 8;
  size_t position_; // the current position to be write(this position don't have
  // data)
  size_t read_pos_;
  size_t capacity_;                 // the capacity of byte array
  std::shared_ptr<Node> root_ptr_;  // the header linker of link list
  std::shared_ptr<Node> write_ptr_; // always point to Node that has empty space
  std::shared_ptr<Node> read_ptr_;
};

} // namespace Sylar

#endif
