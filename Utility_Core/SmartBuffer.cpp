#include "SmartBuffer.h"

void CSmartBuffer::Read(void* _des, byte** _pSrc, uint32_t _size)
{
	memcpy(_des, *_pSrc, _size);
	*_pSrc += _size;
};

void CSmartBuffer::Write(byte** _pDes, const void* _src, uint32_t _size)
{
	memcpy(*_pDes, _src, _size);
	*_pDes += _size;
}
