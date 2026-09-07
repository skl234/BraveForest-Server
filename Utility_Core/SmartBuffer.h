#pragma once
#include <WinSock2.h>
#include <cinttypes>

class CSmartBuffer
{
public:
	template<typename _T>
	static _T		Read(byte** _pSrc);
	static void		Read(void* _des, byte** _pSrc, uint32_t _size);

	template<typename _T>
	static void		Write(byte** _pDes, _T _data);
	static void		Write(byte** _pDes, const void* _src, uint32_t _size);
};

template<typename _T>
_T CSmartBuffer::Read(byte** _pSrc)
{
	_T data = *reinterpret_cast<_T*>(*_pSrc);
	*_pSrc += sizeof(_T);
	return data;
};

template<typename _T>
void CSmartBuffer::Write(byte** _pDes, _T _data)
{
	*reinterpret_cast<_T*>(*_pDes) = _data;
	*_pDes += sizeof(_T);
};
