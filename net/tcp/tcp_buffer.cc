#include <memory>
#include <string.h>
#include "../../common/log.h"
#include "tcp_buffer.h"
#include <algorithm>

namespace rocket {

TcpBuffer::TcpBuffer(int size) : m_buffer(size)
{}

// 返回可写的字节数
// int TcpBuffer::writeAble() const {
//   return m_buffer.size() - m_write_index;
// }

int TcpBuffer::readIndex() const {
  return m_read_index;
}

int TcpBuffer::writeIndex() const {
  return m_write_index;
}

void TcpBuffer::writeToBuffer(const char* buf, int size) {
  if (size > m_buffer.size() - m_write_index)
    resizeBuffer((m_write_index + size)<<1);
  
  std::copy_n(buf,size,m_buffer.begin()+m_write_index);
//  memcpy(&m_buffer[m_write_index], buf, size);
  m_write_index += size; 
}

void TcpBuffer::readFromBuffer(std::vector<char>& re, int size) {
  if (readAble() == 0)
    return;

  int read_size = readAble() > size ? size : readAble();

  std::vector<char> tmp(read_size);
  memcpy(&tmp[0], &m_buffer[m_read_index], read_size);

  re.swap(tmp); 
  m_read_index += read_size;

  adjustBuffer();
}

void TcpBuffer::resizeBuffer(int new_size){
    int relocate  = std::min(new_size,m_write_index - m_read_index);    
    if (new_size<=m_buffer.capacity() ){
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

void TcpBuffer::adjustBuffer() {
  if (m_read_index < int(m_buffer.size() / 3)) {
    return;
  }
  std::vector<char> buffer(m_buffer.size());
  int count = readAble();

  memcpy(&buffer[0], &m_buffer[m_read_index], count);
  m_buffer.swap(buffer);
  m_read_index = 0;
  m_write_index = m_read_index + count;

  buffer.clear();
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