#include <memory>
#include <string.h>
#include "../../common/log.h"
#include "tcp_buffer.h"
#include <algorithm>

namespace rocket {

TcpBuffer::TcpBuffer(int size) : m_buffer(size)
{}

int TcpBuffer::readIndex() const {
  return m_read_index;
}

int TcpBuffer::writeIndex() const {
  return m_write_index;
}

void TcpBuffer::writeToBuffer(const char* buf, int size){
  int writable = m_buffer.size() - m_write_index;
  if (size > writable){
    if(writable+m_read_index >= size){
      memmove(m_buffer.data(),m_buffer.data()+m_read_index,m_write_index -= m_read_index);
      m_read_index = 0;
    }else 
      resizeBuffer((m_write_index - m_read_index + size)<<1);
  }
  std::copy_n(buf,size,m_buffer.begin()+m_write_index);
  m_write_index += size; 
}

void TcpBuffer::readFromBuffer(std::vector<char>& re, int size) {
  int readable = m_write_index - m_read_index;
  if (!readable)
    return;

  size = std::min(size , readable);
  
  re.resize(size);
  std::copy_n(m_buffer.begin()+m_read_index, size, re.begin());

  m_read_index += size;
}

void TcpBuffer::resizeBuffer(int new_size){
    int relocate = std::min(m_write_index - m_read_index ,new_size);

    if (new_size <= m_buffer.capacity()){
      memmove(m_buffer.data(),m_buffer.data()+m_read_index,relocate);
      m_buffer.resize(new_size);
    }else{
      std::vector<char> tmp (new_size);
      std::copy_n(m_buffer.begin() + m_read_index,relocate,tmp.begin() );
      m_buffer.swap(tmp);
    }

    m_read_index = 0;
    m_write_index = relocate;
}

void TcpBuffer::moveReadIndex(int size) {
  size_t j = m_read_index + size;
  if (j >= m_buffer.size()) {
    ERRORLOG("moveReadIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index, m_buffer.size());
    return;
  }
  m_read_index = j;
  adjustBuffer();
}

void TcpBuffer::moveWriteIndex(int size) {
  size_t j = m_write_index + size;
  if (j >= m_buffer.size()) {
    ERRORLOG("moveWriteIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index, m_buffer.size());
    return;
  }
  m_write_index = j;
  adjustBuffer();
}

}