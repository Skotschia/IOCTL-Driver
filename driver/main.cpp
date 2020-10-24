#include "proc.hpp"
#include "IOCTL.hpp"
#include <algorithm>


UNICODE_STRING unicode( const wchar_t *text ) {
	UNICODE_STRING output_string {};
	RtlInitUnicodeString( &output_string , text );
	return output_string;
}

NTSTATUS device_population( DEVICE_OBJECT *device_object , IRP *irp_call ) {
	UNREFERENCED_PARAMETER( &device_object );
	UNREFERENCED_PARAMETER( &irp_call );
	return STATUS_SUCCESS;
}

NTSTATUS device_control( DEVICE_OBJECT *device_object , IRP *irp_call ) {
	ULONG bytes = 0;
	NTSTATUS status = STATUS_SUCCESS;

	const auto stack_location = IoGetCurrentIrpStackLocation( irp_call );

	if ( !stack_location )
		return STATUS_INTERNAL_ERROR;

	if ( !irp_call->AssociatedIrp.SystemBuffer )
		return STATUS_INVALID_PARAMETER;

	auto kernel_memory = [ & ] ( const std::uintptr_t virtual_address ) {
		return virtual_address >= ( ( std::uintptr_t )1 << ( 8 * sizeof( std::uintptr_t ) - 1 ) );
	};

	const auto buffer_length = std::max( stack_location->Parameters.DeviceIoControl.InputBufferLength , stack_location->Parameters.DeviceIoControl.OutputBufferLength );

	switch ( stack_location->Parameters.DeviceIoControl.IoControlCode ) {
	case operation_memory: {
		if ( buffer_length >= sizeof( memory_request ) ) {
			const auto request = reinterpret_cast< memory_request * >( irp_call->AssociatedIrp.SystemBuffer );

			if ( !request->virtual_address ) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			const auto process = win::attain_process( request->process_id );

			if ( !process ) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if ( kernel_memory( request->virtual_address ) ) {
				status = STATUS_ACCESS_DENIED;
				break;
			}

			std::size_t processed_bytes = 0;

			request->memory_state ?
				MmCopyVirtualMemory( IoGetCurrentProcess( ) , reinterpret_cast< void * >( request->memory_buffer ) , process.get( ) , reinterpret_cast< void * >( request->virtual_address ) , request->memory_size , UserMode , &processed_bytes )
				:
				MmCopyVirtualMemory( process.get( ) , reinterpret_cast< void * >( request->virtual_address ) , IoGetCurrentProcess( ) , reinterpret_cast< void * >( request->memory_buffer ) , request->memory_size , UserMode , &processed_bytes );

			bytes = sizeof( memory_request );
			status = STATUS_SUCCESS;
		}
		else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	}
	case operation_module: {
		if ( buffer_length >= sizeof( module_request ) ) {
			auto request = reinterpret_cast< module_request * >( irp_call->AssociatedIrp.SystemBuffer );

			const auto process = win::attain_process( request->process_id );

			if ( !process) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			request->memory_buffer = reinterpret_cast< std::uintptr_t >( PsGetProcessSectionBaseAddress( process.get( ) ) );

			bytes = sizeof( module_request );
			status = STATUS_SUCCESS;
		}
		else {
			status = STATUS_INFO_LENGTH_MISMATCH;
		}
		break;
	}
	default:
		break;
	}

	irp_call->IoStatus.Status = status;
	irp_call->IoStatus.Information = bytes;
	IoCompleteRequest( irp_call , IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}



NTSTATUS unload_driver( DRIVER_OBJECT *driver_object ) {
	auto dos_device = unicode( L"\\DosDevices\\Example" ); // use a legit device name.
	IoDeleteSymbolicLink( &dos_device );
	IoDeleteDevice( driver_object->DeviceObject );
	return STATUS_SUCCESS;
}

NTSTATUS ENTRY_POINT( DRIVER_OBJECT *driver_object , UNICODE_STRING *registry_path ) {
	UNREFERENCED_PARAMETER( registry_path );

	auto dos_device = unicode( L"\\DosDevices\\Example" );// use a legit device name.
	auto device = unicode( L"\\Device\\Example" );// use a legit device name.
	DEVICE_OBJECT *device_object {};

	IoCreateDevice( driver_object , 0 , &device , FILE_DEVICE_UNKNOWN , FILE_DEVICE_SECURE_OPEN , false , &device_object );
	IoCreateSymbolicLink( &dos_device , &device );

	if ( !device_object )
		return STATUS_UNSUCCESSFUL;

	driver_object->MajorFunction [ IRP_MJ_DEVICE_CONTROL ] = device_control;
	driver_object->MajorFunction [ IRP_MJ_CREATE ] = device_population;
	driver_object->MajorFunction [ IRP_MJ_CLOSE ] = device_population;
	driver_object->DriverUnload = reinterpret_cast< PDRIVER_UNLOAD >( unload_driver );

	device_object->Flags |= DO_BUFFERED_IO;
	device_object->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}