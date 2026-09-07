#pragma once

#define DELETE_COPY(ClassName)                         \
	ClassName(const ClassName&) = delete;              \
	ClassName& operator=(const ClassName&) = delete

#define DELETE_MOVE(ClassName)                         \
	ClassName(ClassName&&) = delete;                   \
	ClassName& operator=(ClassName&&) = delete

#define DELETE_COPY_MOVE(ClassName)                    \
	ClassName(const ClassName&) = delete;              \
	ClassName& operator=(const ClassName&) = delete;   \
	ClassName(ClassName&&) = delete;                   \
	ClassName& operator=(ClassName&&) = delete
