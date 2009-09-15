/*
 * libesedb Input/Output (IO) handle
 *
 * Copyright (c) 2008-2009, Joachim Metz <forensics@hoffmannbv.nl>,
 * Hoffmann Investigations. All rights reserved.
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This software is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <endian.h>
#include <memory.h>
#include <types.h>

#include <liberror.h>
#include <libnotify.h>

#include "libesedb_checksum.h"
#include "libesedb_debug.h"
#include "libesedb_definitions.h"
#include "libesedb_guid.h"
#include "libesedb_io_handle.h"
#include "libesedb_libbfio.h"
#include "libesedb_libfdatetime.h"
#include "libesedb_string.h"

#include "esedb_file_header.h"

const uint8_t esedb_file_signature[ 4 ] = { 0xef, 0xcd, 0xab, 0x89 };

/* Initialize an io handle
 * Make sure the value io_handle is pointing to is set to NULL
 * Returns 1 if successful or -1 on error
 */
int libesedb_io_handle_initialize(
     libesedb_io_handle_t **io_handle,
     liberror_error_t **error )
{
	static char *function = "libesedb_io_handle_initialize";

	if( io_handle == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid io handle.",
		 function );

		return( -1 );
	}
	if( *io_handle == NULL )
	{
		*io_handle = (libesedb_io_handle_t *) memory_allocate(
		                                     sizeof( libesedb_io_handle_t ) );

		if( *io_handle == NULL )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_MEMORY,
			 LIBERROR_MEMORY_ERROR_INSUFFICIENT,
			 "%s: unable to create io handle.",
			 function );

			return( -1 );
		}
		if( memory_set(
		     *io_handle,
		     0,
		     sizeof( libesedb_io_handle_t ) ) == NULL )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_MEMORY,
			 LIBERROR_MEMORY_ERROR_SET_FAILED,
			 "%s: unable to clear file.",
			 function );

			memory_free(
			 *io_handle );

			*io_handle = NULL;

			return( -1 );
		}
	}
	return( 1 );
}

/* Frees an exisisting io handle
 * Returns 1 if successful or -1 on error
 */
int libesedb_io_handle_free(
     libesedb_io_handle_t **io_handle,
     liberror_error_t **error )
{
	static char *function = "libesedb_io_handle_free";
	int result            = 1;

	if( io_handle == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid io handle.",
		 function );

		return( -1 );
	}
	if( *io_handle != NULL )
	{
		if( ( ( *io_handle )->handle_created_in_library != 0 )
		 && ( ( *io_handle )->file_io_handle != NULL )
		 && ( libbfio_handle_free(
		       &( ( *io_handle )->file_io_handle ),
		       error ) != 1 ) )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free file io handle.",
			 function );

			result = -1;
		}
		memory_free(
		 *io_handle );

		*io_handle = NULL;
	}
	return( result );
}

/* Opens an io handle
 * Returns 1 if successful or -1 on error
 */
int libesedb_io_handle_open(
     libesedb_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     int flags,
     liberror_error_t **error )
{
        static char *function = "libesedb_io_handle_open";

        if( io_handle == NULL )
        {
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
                 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
                 "%s: invalid io handle.",
                 function );

                return( -1 );
        }
	if( io_handle->file_io_handle != NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid io handle - file io handle already set.",
		 function );

		return( -1 );
	}
        if( file_io_handle == NULL )
        {
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
                 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
                 "%s: invalid file io handle.",
                 function );

                return( -1 );
        }
	io_handle->file_io_handle = file_io_handle;

	if( libbfio_handle_open(
	     io_handle->file_io_handle,
	     flags,
	     error ) != 1 )
	{
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_IO,
                 LIBERROR_IO_ERROR_OPEN_FAILED,
                 "%s: unable to open file io handle.",
                 function );

                return( -1 );
	}
	return( 1 );
}

/* Closes an io handle
 * Returns 0 if successful or -1 on error
 */
int libesedb_io_handle_close(
     libesedb_io_handle_t *io_handle,
     liberror_error_t **error )
{
        static char *function = "libesedb_io_handle_close";

        if( io_handle == NULL )
        {
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
                 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
                 "%s: invalid io handle.",
                 function );

                return( -1 );
        }
#if defined( HAVE_DEBUG_OUTPUT )
	if( libesedb_debug_print_read_offsets(
	     io_handle->file_io_handle,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
                 LIBERROR_ERROR_DOMAIN_RUNTIME,
                 LIBERROR_RUNTIME_ERROR_PRINT_FAILED,
		 "%s: unable to print the read offsets.",
		 function );
	}
#endif
	if( libbfio_handle_close(
	     io_handle->file_io_handle,
	     error ) != 0 )
	{
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_IO,
                 LIBERROR_IO_ERROR_CLOSE_FAILED,
                 "%s: unable to close file io handle.",
                 function );

                return( -1 );
	}
	return( 0 );
}

/* Reads the file header
 * Returns 1 if successful or -1 on error
 */
int libesedb_io_handle_read_file_header(
     libesedb_io_handle_t *io_handle,
     size_t *page_size,
     liberror_error_t **error )
{
	uint8_t *file_header_data          = NULL;
	static char *function              = "libesedb_io_handle_read_file_header";
	size_t read_size                   = 4096;
	ssize_t read_count                 = 0;
	uint32_t calculated_xor32_checksum = 0;
	uint32_t stored_xor32_checksum     = 0;

#if defined( HAVE_VERBOSE_OUTPUT )
	uint32_t test                = 0;
#endif

	if( io_handle == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid io handle.",
		 function );

		return( -1 );
	}
	if( io_handle->file_io_handle == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid io handle - missing file io handle.",
		 function );

		return( -1 );
	}
	if( page_size == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid page size.",
		 function );

		return( -1 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	libnotify_verbose_printf(
	 "%s: reading file header at offset: %" PRIu64 " (0x%08" PRIx64 ")\n",
	 function,
	 0,
	 0 );
#endif

	if( libbfio_handle_seek_offset(
	     io_handle->file_io_handle,
	     0,
	     SEEK_SET,
	     error ) == -1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek file header offset: %" PRIu64 ".",
		 function,
		 0 );

		return( -1 );
	}
	file_header_data = (uint8_t *) memory_allocate(
	                                read_size );

	if( file_header_data == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_MEMORY,
		 LIBERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create file header data.",
		 function );

		return( -1 );
	}
	read_count = libbfio_handle_read(
	              io_handle->file_io_handle,
	              file_header_data,
	              read_size,
	              error );

	if( read_count != (ssize_t) read_size )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read file header.",
		 function );

		memory_free(
		 file_header_data );

		return( -1 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	libnotify_verbose_printf(
	 "%s: file header data:\n",
	 function );
	libnotify_verbose_print_data(
	 file_header_data,
	 sizeof( esedb_file_header_t ) );
#endif

	if( memory_compare(
	     ( (esedb_file_header_t *) file_header_data )->signature,
	     esedb_file_signature,
	     4 ) != 0 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported file signature.",
		 function );

		memory_free(
		 file_header_data );

		return( -1 );
	}
	if( libesedb_checksum_calculate_little_endian_xor32(
	     &calculated_xor32_checksum,
	     &( file_header_data[ 4 ] ),
	     read_size - 4,
	     0x89abcdef,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unable to calculate XOR-32 checksum.",
		 function );

		memory_free(
		 file_header_data );

		return( -1 );
	}
	endian_little_convert_32bit(
	 stored_xor32_checksum,
	 ( (esedb_file_header_t *) file_header_data )->checksum );

	if( stored_xor32_checksum != calculated_xor32_checksum )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_INPUT,
		 LIBERROR_INPUT_ERROR_CRC_MISMATCH,
		 "%s: mismatch in file header checksum ( 0x%08" PRIx32 " != 0x%08" PRIx32 " ).",
		 function,
		 stored_xor32_checksum,
		 calculated_xor32_checksum );

		memory_free(
		 file_header_data );

		return( -1 );
	}
	/* TODO */

	endian_little_convert_32bit(
	 io_handle->format_version,
	 ( (esedb_file_header_t *) file_header_data )->format_version );

	endian_little_convert_32bit(
	 io_handle->format_revision,
	 ( (esedb_file_header_t *) file_header_data )->format_revision );

	endian_little_convert_32bit(
	 *page_size,
	 ( (esedb_file_header_t *) file_header_data )->page_size );

#if defined( HAVE_VERBOSE_OUTPUT )
	libnotify_verbose_printf(
	 "%s: checksum\t\t\t\t: 0x%08" PRIx32 "\n",
	 function,
	 stored_xor32_checksum );

	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->signature );
	libnotify_verbose_printf(
	 "%s: signature\t\t\t\t: 0x%08" PRIx32 "\n",
	 function,
	 test );

	libnotify_verbose_printf(
	 "%s: format version\t\t\t: 0x%08" PRIx32 "\n",
	 function,
	 io_handle->format_version );

	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->file_type );
	libnotify_verbose_printf(
	 "%s: file type\t\t\t\t: %" PRIu32 "\n",
	 function,
	 test );

	libnotify_verbose_printf(
	 "%s: database time:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->database_time,
	 8 );

	libnotify_verbose_printf(
	 "%s: database signature:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->database_signature,
	 28 );
	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->database_state );
	libnotify_verbose_printf(
	 "%s: database state\t\t\t: 0x%08" PRIx32 "\n",
	 function,
	 test );

	libnotify_verbose_printf(
	 "%s: consistent position:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->consistent_postition,
	 8 );
	libesedb_debug_print_log_time(
	 ( (esedb_file_header_t *) file_header_data )->consistent_time,
	 8,
	 "consistent time\t\t\t\t",
	 NULL );

	libesedb_debug_print_log_time(
	 ( (esedb_file_header_t *) file_header_data )->attach_time,
	 8,
	 "attach time\t\t\t\t",
	 NULL );
	libnotify_verbose_printf(
	 "%s: attach position:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->attach_postition,
	 8 );

	libesedb_debug_print_log_time(
	 ( (esedb_file_header_t *) file_header_data )->detach_time,
	 8,
	 "detach time\t\t\t\t",
	 NULL );
	libnotify_verbose_printf(
	 "%s: detach position:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->detach_postition,
	 8 );

	libnotify_verbose_printf(
	 "%s: log signature:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->log_signature,
	 28 );

	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->unknown4 );
	libnotify_verbose_printf(
	 "%s: unknown4\t\t\t\t: 0x%08" PRIx32 " (%" PRIu32 ")\n",
	 function,
	 test,
	 test );
	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->unknown5 );
	libnotify_verbose_printf(
	 "%s: unknown5\t\t\t\t: 0x%08" PRIx32 " (%" PRIu32 ")\n",
	 function,
	 test,
	 test );

	libnotify_verbose_printf(
	 "%s: previous full backup:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->previous_full_backup,
	 24 );
	libnotify_verbose_printf(
	 "%s: previous incremental backup:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->previous_incremental_backup,
	 24 );
	libnotify_verbose_printf(
	 "%s: current full backup:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->current_full_backup,
	 24 );

	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->last_object_identifier );
	libnotify_verbose_printf(
	 "%s: last object identifier\t\t: %" PRIu32 "\n",
	 function,
	 test );

	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->index_update_major_version );
	libnotify_verbose_printf(
	 "%s: index update major version\t\t: %" PRIu32 "\n",
	 function,
	 test );
	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->index_update_minor_version );
	libnotify_verbose_printf(
	 "%s: index update minor version\t\t: %" PRIu32 "\n",
	 function,
	 test );
	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->index_update_build_number );
	libnotify_verbose_printf(
	 "%s: index update build number\t\t: %" PRIu32 "\n",
	 function,
	 test );
	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->index_update_service_pack_number );
	libnotify_verbose_printf(
	 "%s: index update service pack number\t: %" PRIu32 "\n",
	 function,
	 test );

	libnotify_verbose_printf(
	 "%s: format revision\t\t\t: 0x%08" PRIx32 "\n",
	 function,
	 io_handle->format_revision );
	libnotify_verbose_printf(
	 "%s: page size\t\t\t\t: %" PRIu32 "\n",
	 function,
	 *page_size );

	libnotify_verbose_printf(
	 "%s: unknown7:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->unknown7,
	 100 );

	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->creation_format_version );
	libnotify_verbose_printf(
	 "%s: creation format version\t\t: 0x%08" PRIx32 "\n",
	 function,
	 test );
	endian_little_convert_32bit(
	 test,
	 ( (esedb_file_header_t *) file_header_data )->creation_format_revision );
	libnotify_verbose_printf(
	 "%s: creation format revision\t\t: 0x%08" PRIx32 "\n",
	 function,
	 test );

	libnotify_verbose_printf(
	 "%s: unknown8:\n",
	 function );
	libnotify_verbose_print_data(
	 ( (esedb_file_header_t *) file_header_data )->unknown8,
	 320 );

	libnotify_verbose_printf(
	 "\n" );
#endif

	memory_free(
	 file_header_data );

	return( 1 );
}
