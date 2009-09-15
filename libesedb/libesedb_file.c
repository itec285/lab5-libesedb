/*
 * libesedb file
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
#include <narrow_string.h>
#include <memory.h>
#include <types.h>
#include <wide_string.h>

#include <liberror.h>
#include <libnotify.h>

#include <libesedb/codepage.h>

#include "libesedb_array_type.h"
#include "libesedb_definitions.h"
#include "libesedb_io_handle.h"
#include "libesedb_file.h"
#include "libesedb_libbfio.h"
#include "libesedb_page.h"

/* Initialize a file
 * Make sure the value file is pointing to is set to NULL
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_initialize(
     libesedb_file_t **file,
     liberror_error_t **error )
{
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_initialize";

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	if( *file == NULL )
	{
		internal_file = (libesedb_internal_file_t *) memory_allocate(
		                                              sizeof( libesedb_internal_file_t ) );

		if( internal_file == NULL )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_MEMORY,
			 LIBERROR_MEMORY_ERROR_INSUFFICIENT,
			 "%s: unable to create file.",
			 function );

			return( -1 );
		}
		if( memory_set(
		     internal_file,
		     0,
		     sizeof( libesedb_internal_file_t ) ) == NULL )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_MEMORY,
			 LIBERROR_MEMORY_ERROR_SET_FAILED,
			 "%s: unable to clear file.",
			 function );

			memory_free(
			 internal_file );

			return( -1 );
		}
		if( libesedb_array_initialize(
		     &( internal_file->page_table ),
		     0,
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to initialize page table.",
			 function );

			memory_free(
			 internal_file );

			return( -1 );
		}
		if( libesedb_io_handle_initialize(
		     &( internal_file->io_handle ),
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to initialize io handle.",
			 function );

			libesedb_array_free(
			 &( internal_file->page_table ),
			 NULL,
			 NULL );
			memory_free(
			 internal_file );

			return( -1 );
		}
		internal_file->ascii_codepage = LIBESEDB_CODEPAGE_WINDOWS_1250;

		*file = (libesedb_file_t *) internal_file;
	}
	return( 1 );
}

/* Frees an exisisting file
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_free(
     libesedb_file_t **file,
     liberror_error_t **error )
{
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_free";
	int result                              = 1;

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	if( *file != NULL )
	{
		internal_file = (libesedb_internal_file_t *) *file;

		/* TODO add page table entry free functions */
		if( libesedb_array_free(
		     &( internal_file->page_table ),
		     NULL,
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free page table.",
			 function );

			result = -1;
		}
		if( ( internal_file->io_handle != NULL )
		 && ( libesedb_io_handle_free(
		       &( internal_file->io_handle ),
		       error ) != 1 ) )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_RUNTIME,
			 LIBERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free io handle.",
			 function );

			result = -1;
		}
		memory_free(
		 internal_file );

		*file = NULL;
	}
	return( result );
}

/* Signals the libesedb file to abort its current activity
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_signal_abort(
     libesedb_file_t *file,
     liberror_error_t **error )
{
	static char *function = "libesedb_file_signal_abort";

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	( (libesedb_internal_file_t *) file )->abort = 1;

	return( 1 );
}

/* Opens a Extensible Storage Engine Database file
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_open(
     libesedb_file_t *file,
     const char *filename,
     int flags,
     liberror_error_t **error )
{
	libbfio_handle_t *file_io_handle        = NULL;
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_open";

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	if( filename == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid filename.",
		 function );

		return( -1 );
	}
	if( ( ( flags & LIBESEDB_FLAG_READ ) != LIBESEDB_FLAG_READ )
	 && ( ( flags & LIBESEDB_FLAG_WRITE ) != LIBESEDB_FLAG_WRITE ) )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported flags.",
		 function );

		return( -1 );
	}
	if( ( flags & LIBESEDB_FLAG_WRITE ) == LIBESEDB_FLAG_WRITE )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: write access to Extensible Storage Engine Database files currently not supported.",
		 function );

		return( -1 );
	}
	internal_file = (libesedb_internal_file_t *) file;

	if( libbfio_file_initialize(
	     &file_io_handle,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to initialize file io handle.",
		 function );

		return( -1 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	if( libbfio_handle_set_track_offsets_read(
	     file_io_handle,
	     1,
	     error ) != 1 )
	{
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_RUNTIME,
                 LIBERROR_RUNTIME_ERROR_SET_FAILED,
                 "%s: unable to set track offsets read in file io handle.",
                 function );

                return( -1 );
	}
#endif
	if( libbfio_file_set_name(
	     file_io_handle,
	     filename,
	     narrow_string_length(
	      filename ) + 1,
	     error ) != 1 )
	{
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_RUNTIME,
                 LIBERROR_RUNTIME_ERROR_SET_FAILED,
                 "%s: unable to set filename in file io handle.",
                 function );

                return( -1 );
	}
	if( libesedb_file_open_file_io_handle(
	     file,
	     file_io_handle,
	     flags,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to open file: %s.",
		 function,
		 filename );

		return( -1 );
	}
	internal_file->io_handle->handle_created_in_library = 1;

	return( 1 );
}

#if defined( HAVE_WIDE_CHARACTER_TYPE )

/* Opens a Extensible Storage Engine Database file
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_open_wide(
     libesedb_file_t *file,
     const wchar_t *filename,
     int flags,
     liberror_error_t **error )
{
	libbfio_handle_t *file_io_handle        = NULL;
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_open_wide";

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	if( filename == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid filename.",
		 function );

		return( -1 );
	}
	if( ( ( flags & LIBESEDB_FLAG_READ ) != LIBESEDB_FLAG_READ )
	 && ( ( flags & LIBESEDB_FLAG_WRITE ) != LIBESEDB_FLAG_WRITE ) )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported flags.",
		 function );

		return( -1 );
	}
	if( ( flags & LIBESEDB_FLAG_WRITE ) == LIBESEDB_FLAG_WRITE )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: write access to Extensible Storage Engine Database files currently not supported.",
		 function );

		return( -1 );
	}
	internal_file = (libesedb_internal_file_t *) file;

	if( libbfio_file_initialize(
	     &file_io_handle,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to initialize file io handle.",
		 function );

		return( -1 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	if( libbfio_handle_set_track_offsets_read(
	     file_io_handle,
	     1,
	     error ) != 1 )
	{
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_RUNTIME,
                 LIBERROR_RUNTIME_ERROR_SET_FAILED,
                 "%s: unable to set track offsets read in file io handle.",
                 function );

                return( -1 );
	}
#endif
	if( libbfio_file_set_name_wide(
	     file_io_handle,
	     filename,
	     wide_string_length(
	      filename ) + 1,
	     error ) != 1 )
	{
                liberror_error_set(
                 error,
                 LIBERROR_ERROR_DOMAIN_RUNTIME,
                 LIBERROR_RUNTIME_ERROR_SET_FAILED,
                 "%s: unable to set filename in file io handle.",
                 function );

                return( -1 );
	}
	if( libesedb_file_open_file_io_handle(
	     file,
	     file_io_handle,
	     flags,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to open file: %ls.",
		 function,
		 filename );

		return( -1 );
	}
	internal_file->io_handle->handle_created_in_library = 1;

	return( 1 );
}

#endif

/* Opens a Extensible Storage Engine Database file using a Basic File IO (bfio) handle
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_open_file_io_handle(
     libesedb_file_t *file,
     libbfio_handle_t *file_io_handle,
     int flags,
     liberror_error_t **error )
{
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_open_file_io_handle";
	int file_io_flags                       = 0;

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
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
	if( ( ( flags & LIBESEDB_FLAG_READ ) != LIBESEDB_FLAG_READ )
	 && ( ( flags & LIBESEDB_FLAG_WRITE ) != LIBESEDB_FLAG_WRITE ) )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported flags.",
		 function );

		return( -1 );
	}
	if( ( flags & LIBESEDB_FLAG_WRITE ) == LIBESEDB_FLAG_WRITE )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: write access to personal folder files currently not supported.",
		 function );

		return( -1 );
	}
	internal_file = (libesedb_internal_file_t *) file;

	if( ( flags & LIBESEDB_FLAG_READ ) == LIBESEDB_FLAG_READ )
	{
		file_io_flags = LIBBFIO_FLAG_READ;
	}
	if( libesedb_io_handle_open(
	     internal_file->io_handle,
	     file_io_handle,
	     file_io_flags,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_OPEN_FAILED,
		 "%s: unable to open file handle.",
		 function );

		return( -1 );
	}
	if( libesedb_file_open_read(
	     internal_file,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read from file handle.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Closes a Extensible Storage Engine Database file
 * Returns 0 if successful or -1 on error
 */
int libesedb_file_close(
     libesedb_file_t *file,
     liberror_error_t **error )
{
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_close";
	int result                              = 0;

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	internal_file = (libesedb_internal_file_t *) file;

	if( internal_file->io_handle == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid file - missing io handle.",
		 function );

		return( -1 );
	}
	result = libesedb_io_handle_close(
	          internal_file->io_handle,
	          error );

	if( result != 0 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_CLOSE_FAILED,
		 "%s: unable to close io handle.",
		 function );
	}
	return( result );
}

/* Opens a Extensible Storage Engine Database file for reading
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_open_read(
     libesedb_internal_file_t *internal_file,
     liberror_error_t **error )
{
	static char *function = "libesedb_file_open_read";
	off64_t file_offset   = 0;
	size_t page_size      = 0;

#if defined( HAVE_DEBUG_OUTPUT )
	size64_t file_size     = 0;
#endif

	if( internal_file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid internal file.",
		 function );

		return( -1 );
	}
	if( internal_file->io_handle == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid internal file - missing io handle.",
		 function );

		return( -1 );
	}
#if defined( HAVE_VERBOSE_OUTPUT )
	libnotify_verbose_printf(
	 "Reading file header:\n" );
#endif

	if( libesedb_io_handle_read_file_header(
	     internal_file->io_handle,
	     &page_size,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_IO,
		 LIBERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read file header.",
		 function );

		return( -1 );
	}
	/* TODO */

#if defined( HAVE_DEBUG_OUTPUT )
	if( libbfio_handle_get_size(
	     internal_file->io_handle->file_io_handle,
	     &file_size,
	     error ) != 1 )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_RUNTIME,
		 LIBERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve file size.",
		 function );

		return( -1 );
	}
	file_offset = 2 * page_size;

	while( file_offset < (off64_t) file_size )
	{
		if( libesedb_page_read(
		     NULL,
		     internal_file->io_handle,
		     file_offset,
		     page_size,
		     error ) != 1 )
		{
			liberror_error_set(
			 error,
			 LIBERROR_ERROR_DOMAIN_IO,
			 LIBERROR_IO_ERROR_READ_FAILED,
			 "%s: unable to read page.",
			 function );

			return( -1 );
		}
		file_offset += page_size;
	}
#endif
	return( 1 );
}

/* Retrieves the file ASCII codepage
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_get_ascii_codepage(
     libesedb_file_t *file,
     int *ascii_codepage,
     liberror_error_t **error )
{
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_get_ascii_codepage";

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	internal_file = (libesedb_internal_file_t *) file;

	if( ascii_codepage == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid ASCII codepage.",
		 function );

		return( -1 );
	}
	*ascii_codepage = internal_file->ascii_codepage;

	return( 1 );
}

/* Sets the file ASCII codepage
 * Returns 1 if successful or -1 on error
 */
int libesedb_file_set_ascii_codepage(
     libesedb_file_t *file,
     int ascii_codepage,
     liberror_error_t **error )
{
	libesedb_internal_file_t *internal_file = NULL;
	static char *function                   = "libesedb_file_set_ascii_codepage";

	if( file == NULL )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file.",
		 function );

		return( -1 );
	}
	internal_file = (libesedb_internal_file_t *) file;

	if( ( ascii_codepage != LIBESEDB_CODEPAGE_ASCII )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1250 )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1251 )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1252 )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1253 )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1254 )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1256 )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1257 )
	 || ( ascii_codepage != LIBESEDB_CODEPAGE_WINDOWS_1258 ) )
	{
		liberror_error_set(
		 error,
		 LIBERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported ASCII codepage.",
		 function );

		return( -1 );
	}
	internal_file->ascii_codepage = ascii_codepage;

	return( 1 );
}
