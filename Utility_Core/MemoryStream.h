#pragma once
#include "SmartBuffer.h"
#include <stdint.h>
#include <cassert>

class CMemoryStream
{
private:
	byte* m_buff;
	byte* m_writePointer;

	uint16_t m_size;
	uint16_t m_capacity;

public:
	CMemoryStream(uint16_t _capacity);
	CMemoryStream(const CMemoryStream& _memoryStream);
	~CMemoryStream();

	CMemoryStream& operator=(const CMemoryStream& _memoryStream);

	void Clear();
	void WriteSizeToFrontWord();

	template<typename _T>
	uint16_t Write(_T _data);
	uint16_t Write(const void* _src, uint16_t _size);

	byte*		GetBuff();
	uint16_t	GetSize();
	uint16_t	GetCapacity();
};

template<typename _T>
uint16_t CMemoryStream::Write(_T _data)
{
	assert(m_size + sizeof(_T) <= m_capacity);

	CSmartBuffer::Write(&m_writePointer, _data);
	m_size += sizeof(_T);

	return m_size;
};
