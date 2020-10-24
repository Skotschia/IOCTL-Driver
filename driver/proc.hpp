#pragma once
#include "exports.hpp"
#include <memory>
#include <cstddef>
#include <cstdint>

namespace win {
	using e_process = std::unique_ptr<std::remove_pointer_t<PEPROCESS> , decltype( &ObfDereferenceObject )>;

	e_process attain_process( const std::uintptr_t process_id ) {
		PEPROCESS process_struct {};
		PsLookupProcessByProcessId( reinterpret_cast< HANDLE >( process_id ) , &process_struct );
		return e_process( process_struct , &ObfDereferenceObject );
	}
}