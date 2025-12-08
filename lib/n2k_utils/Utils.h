#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <time.h>
#include <cstring>
#include <stdint.h>

class N2KSid
{
  public:
    N2KSid();
    unsigned char getNew();
    unsigned char getCurrent();
  private:
    unsigned char sid;
};

class ByteBuffer
{
public:
  ByteBuffer(const ByteBuffer& b)
  {
    buf_size = b.buf_size;
    buffer = new uint8_t[buf_size];
    offset = b.offset;
    memcpy(buffer, b.buffer, offset);
  }

  ByteBuffer(size_t size) : buf_size(size), offset(0) {
      buffer = new uint8_t[size];
  }

  ByteBuffer(void* data, size_t size) : buf_size(size), offset(size) {
      buffer = new uint8_t[size];
      memcpy(buffer, data, size);
  }

  ~ByteBuffer()
  {
      delete buffer;
  }

  ByteBuffer& operator= (const ByteBuffer& b)
  {
      if (this != &b) {
          delete buffer;
          buf_size = b.buf_size;
          buffer = new uint8_t[buf_size];
          offset = b.offset;
          memcpy(buffer, b.buffer, offset);
      }
      return *this;
  }

    ByteBuffer &operator<<(char* t)
    {
        return (*this) << (const char*)t;
    }

    ByteBuffer &operator<< (const char* t)
    {
        size_t t_size = strlen(t);
        if (t_size<255 && (offset + t_size) < buf_size) {
            *this << (uint8_t)t_size;
            memcpy(buffer + offset, t, t_size);
            offset += t_size;
        }
        return *this;
    }

    template<typename T>
    ByteBuffer &operator<< (T t)
    {
        size_t t_size = sizeof(T);
        if (offset + t_size <= buf_size) {
            memcpy(buffer + offset, &t, t_size);
            offset += t_size;
        }
        return *this;
    }

    ByteBuffer& reset()
    { 
      offset = 0; 
      return *this;
    }

    void get_data(uint8_t* dest, size_t len) const
    {
        if (len > offset) len = offset;
        if (offset == 0) return;
        memcpy(dest, buffer, len);
    }

    uint8_t* data() { return buffer; }
    size_t size() const { return buf_size; }
    size_t length() const { return offset; }

    bool operator==(const ByteBuffer& other) const
    {
        if (offset != other.offset) return false;
        return (memcmp(buffer, other.buffer, offset) == 0);
    }

private:
    uint8_t *buffer;
    size_t buf_size;
    size_t offset;
};

typedef ByteBuffer* ByteBufferPtr;

bool startswith(const char* str_to_find, const char* str);
int getDaysSince1970(int y, int m, int d);
const char* time_to_ISO(time_t t, int millis);
char* replace(char const * const original, char const * const pattern, char const * const replacement, bool first = false);
char *replace_and_free(char *orig, const char *pattern, const char *new_string, bool first);
int indexOf(const char* haystack, const char* needle);
bool array_contains(short test, short* int_set, int sz);
ulong _millis();
int msleep(long msec);
unsigned long get_free_mem();
unsigned long check_elapsed(ulong time, ulong &last_time, ulong period);
void format_thousands_sep(char* buffer, long l);
double lpf(double value, double previous_value, double alpha);

#endif
