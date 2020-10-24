#pragma once
#include <ntifs.h>
#include <cstdint>
#include <cstddef>

#define operation_memory CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )
#define operation_module CTL_CODE( FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS )

struct memory_request {
	std::uintptr_t process_id;
	std::uintptr_t virtual_address;
	std::size_t memory_size;
	std::uintptr_t memory_buffer;
	bool memory_state;
};

struct module_request {
	std::uintptr_t process_id;
	std::uintptr_t memory_buffer;
};