#include "RingBuffer.h"

CRingBuffer::CRingBuffer(uint64_t _size):
	m_buffer(_size),
	m_begin(m_buffer.data()),
	m_end(m_begin + _size),
	m_writePointer(m_begin),
	m_readPointer(m_begin)
{
}

void CRingBuffer::Cleanup()
{
	m_writePointer = m_begin;
	m_readPointer = m_begin;
}

bool CRingBuffer::MoveReadPointer(uint64_t _size)
{
	if (_size > GetReadableSize()) return false;

	m_readPointer += _size;

	if (m_readPointer >= m_end)
	{
		m_readPointer = m_begin + (m_readPointer - m_end);
	}

	return true;
}

bool CRingBuffer::MoveWritePointer(uint64_t _size)
{
	if (_size > GetWriteableSize()) return false;

	m_writePointer += _size;

	if (m_writePointer >= m_end)
	{
		m_writePointer = m_begin + (m_writePointer - m_end);
	}

	return true;
}

uint64_t CRingBuffer::GetWriteableSize() const
{
	uint64_t writeableSize = 0;

	if (m_writePointer >= m_readPointer)
	{
		uint64_t size1 = static_cast<uint64_t>(m_end - m_writePointer);
		uint64_t size2 = static_cast<uint64_t>(m_readPointer - m_begin);
		writeableSize = size1 + size2;
	}
	else
	{
		writeableSize = static_cast<uint64_t>(m_readPointer - m_writePointer);
	}

	if (writeableSize > 0) writeableSize -= 1;

	return writeableSize;
}

uint64_t CRingBuffer::GetDirectWriteableSize()
{
	uint64_t directWriteableSize = 0;

	if (m_writePointer >= m_readPointer)
	{
		directWriteableSize = static_cast<uint64_t>(m_end - m_writePointer);

		if (m_readPointer == m_begin && directWriteableSize > 0)
		{
			directWriteableSize -= 1;
		}
	}
	else
	{
		directWriteableSize = static_cast<uint64_t>(m_readPointer - m_writePointer);

		if (directWriteableSize > 0)
		{
			directWriteableSize -= 1;
		}
	}

	return directWriteableSize;
}

uint64_t CRingBuffer::GetReadableSize()
{
	if (m_writePointer >= m_readPointer)
	{
		return static_cast<uint64_t>(m_writePointer - m_readPointer);
	}

	return static_cast<uint64_t>((m_end - m_readPointer) + (m_writePointer - m_begin));
}

uint64_t CRingBuffer::GetDirectReadableSize()
{
	if (m_writePointer >= m_readPointer)
	{
		return static_cast<uint64_t>(m_writePointer - m_readPointer);
	}

	return static_cast<uint64_t>(m_end - m_readPointer);
}

byte* CRingBuffer::GetWritePointer()
{
	return m_writePointer;
}

byte* CRingBuffer::GetReadPointer()
{
	return m_readPointer;
}

bool CRingBuffer::Read(std::vector<byte>& _buff, uint64_t _size)
{
	if (_size == 0) return true;

	uint64_t readableSize = GetReadableSize();
	if (readableSize < _size) return false;

	uint64_t directReadableSize = static_cast<uint64_t>(m_end - m_readPointer);

	_buff.resize(_size);
	if (directReadableSize >= _size)
	{
		std::copy(m_readPointer, m_readPointer + _size, _buff.begin());
	}
	else
	{
		std::copy(m_readPointer, m_end, _buff.begin());
		std::copy(m_begin, m_begin + (_size - directReadableSize), _buff.begin() + directReadableSize);
	}

	MoveReadPointer(_size);

	return true;
}

bool CRingBuffer::Write(const byte* _buff, uint64_t _size)
{
	if (_buff == nullptr) return false;
	if (_size == 0) return true;

	uint64_t writeableSize = GetWriteableSize();
	if (writeableSize < _size) return false;

	uint64_t directWriteableSize = static_cast<uint64_t>(m_end - m_writePointer);

	if (directWriteableSize >= _size)
	{
		std::copy(_buff, _buff + _size, m_writePointer);
	}
	else
	{
		std::copy(_buff, _buff + directWriteableSize, m_writePointer);
		std::copy(_buff + directWriteableSize, _buff + _size, m_begin);
	}

	MoveWritePointer(_size);

	return true;
}
