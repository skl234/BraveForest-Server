#include "MemoryStream.h"

CMemoryStream::CMemoryStream(uint16_t _capacity) :
	m_buff(new byte[_capacity]),
	m_writePointer(m_buff),
	m_size(0),
	m_capacity(_capacity)
{
}

CMemoryStream::CMemoryStream(const CMemoryStream& _memoryStream)
{
	m_size = _memoryStream.m_size;
	m_capacity = _memoryStream.m_capacity;

	//메모리 복사
	m_buff = new byte[m_capacity];
	memcpy(m_buff, _memoryStream.m_buff, m_size);
	m_writePointer = m_buff + m_size;
}

CMemoryStream::~CMemoryStream()
{
	if (m_buff) delete[] m_buff;
}

CMemoryStream& CMemoryStream::operator=(const CMemoryStream& _memoryStream)
{
	if (this == &_memoryStream) {
		return *this;
	}

	//기존 메모리 제거
	if (m_buff) delete[] m_buff;

	m_size = _memoryStream.m_size;
	m_capacity = _memoryStream.m_capacity;

	//메모리 복사
	m_buff = new byte[m_capacity];
	memcpy(m_buff, _memoryStream.m_buff, m_size);
	m_writePointer = m_buff + m_size;

	return *this;
}

void CMemoryStream::Clear()
{
	m_writePointer = m_buff;
	m_size = 0;
}

void CMemoryStream::WriteSizeToFrontWord()
{
	if (m_capacity < sizeof(uint16_t)) *m_buff = static_cast<byte>(m_size);
	else *reinterpret_cast<uint16_t*>(m_buff) = m_size;
}

uint16_t CMemoryStream::Write(const void* _src, uint16_t _size)
{
	assert(m_size + _size <= m_capacity);

	CSmartBuffer::Write(&m_writePointer, _src, _size);
	m_size += _size;

	return m_size;
}

byte* CMemoryStream::GetBuff()
{
	return m_buff;
}

uint16_t CMemoryStream::GetSize()
{
	return m_size;
}

uint16_t CMemoryStream::GetCapacity()
{
	return m_capacity;
}
