#include "include/bytearray.hh"

namespace Sylar {

template <typename T> static T RoundUp(T n, T m) { return (n + m - 1) / m; }
template <typename T> static T RoundDown(T n, T m) { return n / m; }

template <typename T> static std::string PrintAllBits(T number) {
  const size_t bits = number * sizeof(T);
  std::bitset<bits> bit(number);
  std::stringstream ss;
  ss << bit;
  return ss.str();
}

static void PrintMemory(void *ptr, size_t numBytes) {
  if (numBytes != sizeof(uint8_t) && numBytes != sizeof(uint16_t) &&
      numBytes != sizeof(uint32_t) && numBytes != sizeof(uint64_t)) {
    std::cerr << "PrintMemory numBytes Invalid" << std::endl;
    return;
  }
  uint8_t *byte_ptr = reinterpret_cast<uint8_t *>(ptr);
  for (size_t i = 0; i < numBytes; i++) {
    auto x = static_cast<uint8_t>(byte_ptr[i]);
    std::cout << std::hex << +x << ' ' << std::dec;
  }
  std::cout << std::endl;
}

ByteArray::Node::Node(size_t size)
    : base_ptr_(nullptr, free), rest_size_(size), next_(nullptr) {
  void *ptr = malloc(size * sizeof(uint8_t));
  if (!ptr) {
    throw std::bad_alloc();
  }

  base_ptr_ = std::move(std::unique_ptr<uint8_t, void (*)(void *)>(
      reinterpret_cast<uint8_t *>(ptr), ::free));
}

ByteArray::Node::~Node() {}

ByteArray::ByteArray(size_t baseSize) {
  if (baseSize < NODE_SIZE) {
    baseSize = NODE_SIZE;
  }
  capacity_ = baseSize;
  position_ = 0;
  read_pos_ = 0;
  root_ptr_ = std::make_shared<Node>(baseSize);
  write_ptr_ = root_ptr_;
  read_ptr_ = root_ptr_;
}

ByteArray::~ByteArray() {}

void ByteArray::ExtendCapacity(size_t delta) {
  size_t node_cnt = RoundUp(delta, NODE_SIZE);
  std::shared_ptr<Node> tmp = write_ptr_;
  for (size_t i = 0; i < node_cnt; i++) {
    std::shared_ptr<Node> n_node(new Node(NODE_SIZE));
    tmp->next_ = n_node;
    tmp = tmp->next_;
    capacity_ += NODE_SIZE;
  }
}

std::string ByteArray::BitsStream() {
  std::stringstream ss;
  std::shared_ptr<uint8_t> buf(new uint8_t[capacity_],
                               [&](void *ptr) { ::free(ptr); });
  std::shared_ptr<Node> tmp = root_ptr_;
  size_t pos = 0;
  while (tmp) {
    if (tmp->next_) {
      memcpy(buf.get() + pos, tmp->base_ptr_.get(), NODE_SIZE);
      pos += NODE_SIZE;
      tmp = tmp->next_;
    } else {
      memcpy(buf.get() + pos, tmp->base_ptr_.get(),
             NODE_SIZE - tmp->rest_size_);
      pos += NODE_SIZE - tmp->rest_size_;
      tmp = tmp->next_;
    }
  }
  ss.write(reinterpret_cast<const char *>(buf.get()), pos);
  return ss.str();
}

void ByteArray::Write(const void *buf, size_t size) {
  if (!write_ptr_ || size == 0) {
    return;
  }
  if (write_ptr_->rest_size_ < size) {
    ExtendCapacity(size);
  }

  size_t buf_pos = 0;
  while (size > 0) {
    if (write_ptr_ && write_ptr_->rest_size_ >= size) {
      size_t start = NODE_SIZE - write_ptr_->rest_size_;
      memcpy(write_ptr_->base_ptr_.get() + start, (const char *)buf + buf_pos,
             size);

      position_ += size;
      buf_pos += size;
      write_ptr_->rest_size_ -= size;
      size = 0;
    } else if (write_ptr_) {
      size_t start = NODE_SIZE - write_ptr_->rest_size_;
      memcpy(write_ptr_->base_ptr_.get() + start, (const char *)buf + buf_pos,
             write_ptr_->rest_size_);

      position_ += write_ptr_->rest_size_;
      buf_pos += write_ptr_->rest_size_;
      size -= write_ptr_->rest_size_;

      write_ptr_->rest_size_ = 0;
      write_ptr_ = write_ptr_->next_;
    }
  }
}

void ByteArray::Read(void *buf, size_t size) {
  if (size > capacity_) {
    throw std::out_of_range("ByteArray out of range");
  }
  size_t bpos = 0;
  while (size > 0) {
    size_t start = read_pos_ % NODE_SIZE;
    size_t read_sz = std::min(NODE_SIZE - start, size);
    memcpy((char *)buf + bpos, read_ptr_->base_ptr_.get() + start, read_sz);
    if (start + read_sz == NODE_SIZE) {
      read_ptr_ = read_ptr_->next_;
    }
    read_pos_ += read_sz;
    bpos += read_sz;
    size -= read_sz;
  }
}

void ByteArray::Dump() {
  std::shared_ptr<Node> tmp = root_ptr_;
  while (tmp) {
    PrintMemory(tmp->base_ptr_.get(), NODE_SIZE);
    tmp = tmp->next_;
  }
  return;
}

void ByteArray::WriteFint8(int8_t val) { Write(&val, sizeof(val)); }

void ByteArray::WriteFuint8(uint8_t val) { Write(&val, sizeof(val)); }

void ByteArray::WriteFint16(int16_t val) {
  val = SwapToLittle(val);
  Write(&val, sizeof(val));
}

void ByteArray::WriteFuint16(uint16_t val) {
  val = SwapToLittle(val);
  Write(&val, sizeof(val));
}

void ByteArray::WriteFint32(int32_t val) {
  val = SwapToLittle(val);
  Write(&val, sizeof(val));
}

void ByteArray::WriteFuint32(uint32_t val) {
  val = SwapToLittle(val);
  Write(&val, sizeof(val));
}

void ByteArray::WriteFint64(int64_t val) {
  val = SwapToLittle(val);
  Write(&val, sizeof(val));
}

void ByteArray::WriteFuint64(uint64_t val) {
  val = SwapToLittle(val);
  Write(&val, sizeof(val));
}

static int8_t EncodeZigZag(int8_t val) { return (val << 1) ^ (val >> 7); }

static int16_t EncodeZigZag(int16_t val) { return (val << 1) ^ (val >> 15); }

static int32_t EncodeZigZag(int32_t val) { return (val << 1) ^ (val >> 31); }

static int64_t EncodeZigZag(int64_t val) { return (val << 1) ^ (val >> 63); }

static uint8_t EncodeZigZag(uint8_t val) { return (val << 1) ^ (val >> 7); }

static uint16_t EncodeZigZag(uint16_t val) { return (val << 1) ^ (val >> 15); }

static uint32_t EncodeZigZag(uint32_t val) { return (val << 1) ^ (val >> 31); }

static uint64_t EncodeZigZag(uint64_t val) { return (val << 1) ^ (val >> 63); }

template <typename T> static T DecodeZigzag(T val) {
  return ((unsigned)val >> 1) ^ -(val & 1);
}

static int8_t DecodeZigZag(int8_t val) {
  return ((uint8_t)val >> 1) ^ -(val & 1);
}

static int16_t DecodeZigZag(int16_t val) {
  return ((uint16_t)val >> 1) ^ -(val & 1);
}

static int32_t DecodeZigZag(int32_t val) {
  return ((uint32_t)val >> 1) ^ -(val & 1);
}

static int64_t DecodeZigZag(int64_t val) {
  return ((uint64_t)val >> 1) ^ -(val & 1);
}

static uint8_t DecodeZigZag(uint8_t val) { return (val >> 1) ^ -(val & 1); }

static uint16_t DecodeZigZag(uint16_t val) { return (val >> 1) ^ -(val & 1); }

static uint32_t DecodeZigZag(uint32_t val) { return (val >> 1) ^ -(val & 1); }

static uint64_t DecodeZigZag(uint64_t val) { return (val >> 1) ^ -(val & 1); }

/**
 * @brief
 *
 * @param [in] val value to be encoded to varint
 * @param [out] bytes the number of bytes of varint
 * @param [out] buf output buffer
 */
template <typename T>
static void EncodeVarint(T val, size_t &bytes, uint8_t *buf) {
  size_t max_sz = 0;
  if (sizeof(val) == sizeof(int8_t)) {
    max_sz = 2;
  } else if (sizeof(val) == sizeof(int16_t)) {
    max_sz = 3;
  } else if (sizeof(val) == sizeof(int32_t)) {
    max_sz = 5;
  } else if (sizeof(val) == sizeof(int64_t)) {
    max_sz = 10;
  }
  for (size_t i = 0; i < max_sz; i++) {
    if ((val & (~0x7f)) == 0) {
      buf[bytes] = (val & 0x7f);
      bytes = i + 1;
      break;
    } else {
      buf[bytes] = (val & 0x7f) | 0x80;
      val >>= 7;
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, int8_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, uint8_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, int16_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, uint16_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, int32_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, uint32_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, int64_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

static void DecodeVarint(uint8_t *buf, size_t bytes, uint64_t &val) {
  val = 0;
  size_t offset = 0;
  for (size_t i = 0; i < bytes; i++, offset += 7) {
    if (buf[i] & 0x80) {
      val |= (buf[i] << offset);
      break;
    } else {
      val |= ((buf[i] & 0x7f) << offset);
    }
  }
}

void ByteArray::WriteVint8(int8_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteVuint8(uint8_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteVint16(int16_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteVuint16(uint16_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteVint32(int32_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteVuint32(uint32_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteVint64(int64_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteVuint64(uint64_t val) {
  val = EncodeZigZag(SwapToLittle(val));
  uint8_t buf[10];
  size_t bytes = 0;
  EncodeVarint(val, bytes, buf);
  Write(buf, bytes);
}

void ByteArray::WriteFloat(float val) { Write(&val, sizeof(val)); }

void ByteArray::WriteDouble(double val) { Write(&val, sizeof(val)); }

void ByteArray::WriteStringWithoutLength(std::string val) {
  WriteVuint64(val.length());
  Write(val.c_str(), val.length());
}

#define XX(type)                                                               \
  type val;                                                                    \
  Read(&val, sizeof(val));                                                     \
  if (SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN) {                                  \
    val = ByteSwap(val);                                                       \
  }                                                                            \
  return val

int8_t ByteArray::ReadFint8() { XX(int8_t); }

uint8_t ByteArray::ReadFuint8() { XX(uint8_t); }

int16_t ByteArray::ReadFint16() { XX(int16_t); }

uint16_t ByteArray::ReadFuint16() { XX(uint16_t); }

int32_t ByteArray::ReadFint32() { XX(int32_t); }

uint32_t ByteArray::ReadFuint32() { XX(uint32_t); }

int64_t ByteArray::ReadFint64() { XX(int64_t); }

uint64_t ByteArray::ReadFuint64() { XX(uint64_t); }

#undef XX

#define XX(type)                                                               \
  do {                                                                         \
    type val;                                                                  \
    uint8_t buf[10];                                                           \
    size_t num = 0;                                                            \
    for (size_t i = 0; i < 10; i++) {                                          \
      uint8_t byte = ReadFint8();                                              \
      buf[i] = byte;                                                           \
      if ((byte & 0x80) == 0) {                                                \
        num = i + 1;                                                           \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    DecodeVarint(buf, num, val);                                               \
    val = DecodeZigZag(val);                                                   \
    return val;                                                                \
  } while (0)

int8_t ByteArray::ReadVint8() { XX(int8_t); }

uint8_t ByteArray::ReadVuint8() { XX(uint8_t); }

int16_t ByteArray::ReadVint16() { XX(int16_t); }

uint16_t ByteArray::ReadVuint16() { XX(uint16_t); }

int32_t ByteArray::ReadVint32() { XX(int32_t); }

uint32_t ByteArray::ReadVuint32() { XX(uint32_t); }

int64_t ByteArray::ReadVint64() { XX(int64_t); }

uint64_t ByteArray::ReadVuint64() { XX(uint64_t); }

#undef XX

float ByteArray::ReadFloat() {
  float val;
  Read(&val, sizeof(val));
  return val;
}

double ByteArray::ReadDouble() {
  double val;
  Read(&val, sizeof(val));
  return val;
}

std::string ByteArray::ReadStringWithoutLength() {
  uint64_t size = ReadVint64();
  std::string ret;
  ret.resize(size);
  Read(&ret[0], size);
  return ret;
}

} // namespace Sylar