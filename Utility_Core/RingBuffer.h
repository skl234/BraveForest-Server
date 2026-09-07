#pragma once
#include <WinSock2.h>
#include <vector>

class CRingBuffer
{
private:
	std::vector<byte>		m_buffer;
	byte*					m_begin;
	byte*					m_end;
	byte*					m_writePointer;
	byte*					m_readPointer;

public:
	CRingBuffer(uint64_t _size);
	~CRingBuffer() = default;

	void		Cleanup();
	bool		MoveReadPointer(uint64_t _size);
	bool		MoveWritePointer(uint64_t _size);
	uint64_t	GetWriteableSize() const ;
	uint64_t	GetDirectWriteableSize();
	uint64_t	GetReadableSize();
	uint64_t	GetDirectReadableSize();
	byte*		GetWritePointer();
	byte*		GetReadPointer();

	template<typename _T>
	bool		Read(_T& _out);
	bool		Read(std::vector<byte>& _buff, uint64_t _size);

	bool		Write(const byte* _buff, uint64_t _size);

	template<typename _T>
	bool		Peek(_T& _out);
};

template<typename _T>
bool CRingBuffer::Read(_T& _out)
{
	uint64_t readableSize = GetReadableSize();
	uint64_t typeSize = sizeof(_T);
	if (readableSize < typeSize) return false;

	uint64_t directReadableSize = static_cast<uint64_t>(m_end - m_readPointer);

	if (directReadableSize >= typeSize)
	{
		memcpy(&_out, m_readPointer, typeSize);
	}
	else
	{
		memcpy(&_out, m_readPointer, directReadableSize);
		memcpy(reinterpret_cast<byte*>(&_out) + directReadableSize,
			m_begin,
			typeSize - directReadableSize);
	}
	//-----------------------------------------------------------

	// readPointer 이동
	MoveReadPointer(typeSize);

	return true;
};

template<typename _T>
bool CRingBuffer::Peek(_T& _out)
{
	uint64_t readableSize = GetReadableSize();
	uint64_t typeSize = sizeof(_T);
	if (readableSize < typeSize) return false;

	uint64_t directReadableSize = static_cast<uint64_t>(m_end - m_readPointer);

	if (directReadableSize >= typeSize)
	{
		memcpy(&_out, m_readPointer, typeSize);
	}
	else
	{
		memcpy(&_out, m_readPointer, directReadableSize);
		memcpy(reinterpret_cast<byte*>(&_out) + directReadableSize,
			m_begin,
			typeSize - directReadableSize);
	}

	return true;
};
