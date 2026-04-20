/*
 * Objects tree functions
 *
 * Copyright (C) 2012-2025, Joachim Metz <joachim.metz@gmail.com>
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <byte_stream.h>
#include <memory.h>
#include <types.h>

#include "libfsrefs_block_reference.h"
#include "libfsrefs_checkpoint.h"
#include "libfsrefs_file_system.h"
#include "libfsrefs_io_handle.h"
#include "libfsrefs_libcerror.h"
#include "libfsrefs_ministore_node.h"
#include "libfsrefs_node_record.h"
#include "libfsrefs_superblock.h"

/* Retrieves the physical block number of a specific container
 * Returns 1 if successful, 0 if not available or -1 on error
 */
int libfsrefs_file_system_compare_record_key_with_key_data(
     const uint8_t *record_key_data,
     size_t record_key_data_size,
     const uint8_t *key_data,
     size_t key_data_size,
     int *result,
     libcerror_error_t **error )
{
	size_t key_data_offset = 0;
	static char *function  = "libfsrefs_file_system_compare_record_key_with_key_data";

	if( record_key_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid record key data.",
		 function );

		return( -1 );
	}
	if( key_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid key data.",
		 function );

		return( -1 );
	}
	if( result == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid result.",
		 function );

		return( -1 );
	}
	if( record_key_data_size == 0 )
	{
		*result = 1;

		return( 1 );
	}
	if( record_key_data_size != key_data_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: record key data size mismatch.",
		 function );

		return( -1 );
	}
	key_data_offset = key_data_size;

	do
	{
		key_data_offset--;

		*result = (int) key_data[ key_data_offset ] - (int) record_key_data[ key_data_offset ];

		if( *result != 0 )
		{
			break;
		}
	}
	while( key_data_offset > 0 );

	return( 1 );
}

int libfsrefs_file_system_get_container_record_by_identifier_from_node(
     libfsrefs_file_system_t *file_system,
     libfsrefs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libfsrefs_ministore_node_t *ministore_node,
     const uint8_t *key_data,
     size_t key_data_size,
     libfsrefs_node_record_t **node_record,
     libcerror_error_t **error )
{
	libfsrefs_block_reference_t *block_reference = NULL;
	libfsrefs_ministore_node_t *sub_node         = NULL;
	libfsrefs_node_record_t *safe_node_record    = NULL;
	static char *function                        = "libfsrefs_file_system_get_container_record_by_identifier_from_node";
	int comparison_result                        = 0;
	int number_of_records                        = 0;
	int record_index                             = 0;
	int result                                   = 0;

	if( ministore_node == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid ministore node.",
		 function );

		return( -1 );
	}
	if( node_record == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid node record.",
		 function );

		return( -1 );
	}
	if( ( ministore_node->node_type_flags & 0x01 ) == 0 )
	{
		return( libfsrefs_ministore_node_get_record_by_key(
		         ministore_node,
		         key_data,
		         key_data_size,
		         node_record,
		         error ) );
	}
	if( libfsrefs_ministore_node_get_number_of_records(
	     ministore_node,
	     &number_of_records,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of records.",
		 function );

		return( -1 );
	}
	for( record_index = 0;
	     record_index < number_of_records;
	     record_index++ )
	{
		if( libfsrefs_ministore_node_get_record_by_index(
		     ministore_node,
		     record_index,
		     &safe_node_record,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve record: %d.",
			 function,
			 record_index );

			goto on_error;
		}
		if( safe_node_record == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: missing record: %d.",
			 function,
			 record_index );

			goto on_error;
		}
		if( safe_node_record->key_data == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: invalid record: %d - missing key data.",
			 function,
			 record_index );

			goto on_error;
		}
		if( libfsrefs_file_system_compare_record_key_with_key_data(
		     safe_node_record->key_data,
		     safe_node_record->key_data_size,
		     key_data,
		     key_data_size,
		     &comparison_result,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to compare record: %d key data.",
			 function,
			 record_index );

			goto on_error;
		}
		if( ( comparison_result <= 0 )
		 || ( safe_node_record->key_data_size == 0 ) )
		{
			if( libfsrefs_block_reference_initialize(
			     &block_reference,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
				 "%s: unable to create block reference.",
				 function );

				goto on_error;
			}
			if( libfsrefs_block_reference_read_data(
			     block_reference,
			     io_handle,
			     safe_node_record->value_data,
			     safe_node_record->value_data_size,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_READ_FAILED,
				 "%s: unable to read branch record block reference: %d.",
				 function,
				 record_index );

				goto on_error;
			}
			for( result = 0;
			     result < 4;
			     result++ )
			{
				if( block_reference->block_numbers[ result ] == 0 )
				{
					break;
				}
				block_reference->block_offsets[ result ] = (off64_t) ( block_reference->block_numbers[ result ] * io_handle->metadata_block_size );
			}
			if( libfsrefs_ministore_node_initialize(
			     &sub_node,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
				 "%s: unable to create sub node.",
				 function );

				goto on_error;
			}
			if( libfsrefs_ministore_node_read_file_io_handle(
			     sub_node,
			     io_handle,
			     file_io_handle,
			     block_reference,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_READ_FAILED,
				 "%s: unable to read sub node: %d.",
				 function,
				 record_index );

				goto on_error;
			}
			result = libfsrefs_file_system_get_container_record_by_identifier_from_node(
			          file_system,
			          io_handle,
			          file_io_handle,
			          sub_node,
			          key_data,
			          key_data_size,
			          node_record,
			          error );

			if( result == -1 )
			{
				goto on_error;
			}
			else if( result == 1 )
			{
				libfsrefs_ministore_node_free(
				 &sub_node,
				 NULL );
				libfsrefs_block_reference_free(
				 &block_reference,
				 NULL );

				return( 1 );
			}
			libfsrefs_ministore_node_free(
			 &sub_node,
			 NULL );
			libfsrefs_block_reference_free(
			 &block_reference,
			 NULL );
			sub_node         = NULL;
			block_reference = NULL;
		}
	}
	return( 0 );

on_error:
	if( sub_node != NULL )
	{
		libfsrefs_ministore_node_free(
		 &sub_node,
		 NULL );
	}
	if( block_reference != NULL )
	{
		libfsrefs_block_reference_free(
		 &block_reference,
		 NULL );
	}
	return( -1 );
}

int libfsrefs_file_system_get_container_block_number_by_identifier(
     libfsrefs_file_system_t *file_system,
     libfsrefs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     uint64_t container_identifier,
     uint64_t *container_block_number,
     libcerror_error_t **error )
{
	uint8_t key_data[ 16 ]                    = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	libfsrefs_node_record_t *node_record     = NULL;
	static char *function                     = "libfsrefs_file_system_get_container_block_number_by_identifier";
	int result                                = 0;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( file_system->containers_root_node == NULL )
	{
		return( 0 );
	}
	if( container_block_number == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid container block number.",
		 function );

		return( -1 );
	}
	byte_stream_copy_from_uint64_little_endian(
	 key_data,
	 container_identifier );
	byte_stream_copy_from_uint32_little_endian(
	 &( key_data[ 12 ] ),
	 1 );

	result = libfsrefs_file_system_get_container_record_by_identifier_from_node(
	          file_system,
	          io_handle,
	          file_io_handle,
	          file_system->containers_root_node,
	          key_data,
	          16,
	          &node_record,
	          error );

	if( result != 1 )
	{
		return( result );
	}
	if( node_record == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: missing node record.",
		 function );

		return( -1 );
	}
	if( node_record->value_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid node record - missing value data.",
		 function );

		return( -1 );
	}
	if( node_record->value_data_size < 152 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid node record value data size value out of bounds.",
		 function );

		return( -1 );
	}
	byte_stream_copy_to_uint64_little_endian(
	 &( node_record->value_data[ 144 ] ),
	 *container_block_number );

	return( 1 );
}

/* Creates a file system
 * Make sure the value file_system is referencing, is set to NULL
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_initialize(
     libfsrefs_file_system_t **file_system,
     libcerror_error_t **error )
{
	static char *function = "libfsrefs_file_system_initialize";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( *file_system != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file system value already set.",
		 function );

		return( -1 );
	}
	*file_system = memory_allocate_structure(
	                libfsrefs_file_system_t );

	if( *file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create file system.",
		 function );

		goto on_error;
	}
	if( memory_set(
	     *file_system,
	     0,
	     sizeof( libfsrefs_file_system_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear file system.",
		 function );

		memory_free(
		 *file_system );

		*file_system = NULL;

		return( -1 );
	}
	return( 1 );

on_error:
	if( *file_system != NULL )
	{
		memory_free(
		 *file_system );

		*file_system = NULL;
	}
	return( -1 );
}

/* Frees a file system
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_free(
     libfsrefs_file_system_t **file_system,
     libcerror_error_t **error )
{
	static char *function = "libfsrefs_file_system_free";
	int result            = 1;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( *file_system != NULL )
	{
		if( ( *file_system )->containers_root_node != NULL )
		{
			if( libfsrefs_ministore_node_free(
			     &( ( *file_system )->containers_root_node ),
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
				 "%s: unable to free containers root node.",
				 function );

				result = -1;
			}
		}
		if( ( *file_system )->checkpoint != NULL )
		{
			if( libfsrefs_checkpoint_free(
			     &( ( *file_system )->checkpoint ),
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
				 "%s: unable to free checkpoint.",
				 function );

				result = -1;
			}
		}
		if( ( *file_system )->superblock != NULL )
		{
			if( libfsrefs_superblock_free(
			     &( ( *file_system )->superblock ),
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
				 "%s: unable to free superblock.",
				 function );

				result = -1;
			}
		}
		memory_free(
		 *file_system );

		*file_system = NULL;
	}
	return( result );
}

/* Reads the superblock
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_read_superblock(
     libfsrefs_file_system_t *file_system,
     libfsrefs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     off64_t file_offset,
     libcerror_error_t **error )
{
	static char *function = "libfsrefs_file_system_read_superblock";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( file_system->superblock != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file system - superblock value already set.",
		 function );

		return( -1 );
	}
	if( libfsrefs_superblock_initialize(
	     &( file_system->superblock ),
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create superblock.",
		 function );

		goto on_error;
	}
	if( libfsrefs_superblock_read_file_io_handle(
	     file_system->superblock,
	     io_handle,
	     file_io_handle,
	     file_offset,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read superblock at offset: %" PRIi64 " (0x%08" PRIx64 ").",
		 function,
		 file_offset,
		 file_offset );

		goto on_error;
	}
	return( 1 );

on_error:
	if( file_system->superblock != NULL )
	{
		libfsrefs_superblock_free(
		 &( file_system->superblock ),
		 NULL );
	}
	return( -1 );
}

/* Reads the checkpoints
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_read_checkpoints(
     libfsrefs_file_system_t *file_system,
     libfsrefs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libcerror_error_t **error )
{
	libfsrefs_checkpoint_t *primary_checkpoint   = NULL;
	libfsrefs_checkpoint_t *secondary_checkpoint = NULL;
	static char *function                        = "libfsrefs_file_system_read_checkpoints";
	off64_t checkpoint_offset                    = 0;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( file_system->superblock == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid file system - missing superblock.",
		 function );

		return( -1 );
	}
	if( file_system->checkpoint != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file system - checkpoint value already set.",
		 function );

		return( -1 );
	}
	if( io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid IO handle.",
		 function );

		return( -1 );
	}
/* TODO support more than 2 checkpoints */

	if( libfsrefs_checkpoint_initialize(
	     &primary_checkpoint,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create primary checkpoint.",
		 function );

		goto on_error;
	}
	checkpoint_offset = file_system->superblock->primary_checkpoint_block_number * io_handle->metadata_block_size;

	if( libfsrefs_checkpoint_read_file_io_handle(
	     primary_checkpoint,
	     io_handle,
	     file_io_handle,
	     checkpoint_offset,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read primary checkpoint at offset: %" PRIi64 " (0x%08" PRIx64 ").",
		 function,
		 checkpoint_offset,
		 checkpoint_offset );

		goto on_error;
	}
	if( libfsrefs_checkpoint_initialize(
	     &secondary_checkpoint,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create secondary checkpoint.",
		 function );

		goto on_error;
	}
	checkpoint_offset = file_system->superblock->secondary_checkpoint_block_number * io_handle->metadata_block_size;

	if( libfsrefs_checkpoint_read_file_io_handle(
	     secondary_checkpoint,
	     io_handle,
	     file_io_handle,
	     checkpoint_offset,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read secondary checkpoint at offset: %" PRIi64 " (0x%08" PRIx64 ").",
		 function,
		 checkpoint_offset,
		 checkpoint_offset );

		goto on_error;
	}
	if( primary_checkpoint->sequence_number >= secondary_checkpoint->sequence_number )
	{
		if( libfsrefs_checkpoint_free(
		     &secondary_checkpoint,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free secondary checkpoint.",
			 function );

			goto on_error;
		}
		file_system->checkpoint = primary_checkpoint;
	}
	else
	{
		if( libfsrefs_checkpoint_free(
		     &primary_checkpoint,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free primary checkpoint.",
			 function );

			goto on_error;
		}
		file_system->checkpoint = secondary_checkpoint;
	}
	return( 1 );

on_error:
	if( secondary_checkpoint != NULL )
	{
		libfsrefs_checkpoint_free(
		 &secondary_checkpoint,
		 NULL );
	}
	if( primary_checkpoint != NULL )
	{
		libfsrefs_checkpoint_free(
		 &primary_checkpoint,
		 NULL );
	}
	file_system->checkpoint = NULL;

	return( -1 );
}

/* Reads the container trees
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_read_container_trees(
     libfsrefs_file_system_t *file_system,
     libfsrefs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libcerror_error_t **error )
{
	libfsrefs_ministore_node_t *copy_root_node = NULL;
	libfsrefs_ministore_node_t *root_node      = NULL;
	static char *function                      = "libfsrefs_file_system_read_container_trees";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
			"%s: invalid file system.",
			function );

		return( -1 );
	}
	if( file_system->containers_root_node != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file system - containers root node value already set.",
		 function );

		return( -1 );
	}
	if( libfsrefs_file_system_get_ministore_tree(
	     file_system,
	     io_handle,
	     file_io_handle,
	     7,
	     &root_node,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve ministore tree: 7 (containers) root node.",
		 function );

		goto on_error;
	}
	if( ( root_node->node_type_flags & 0x02 ) == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
			"%s: unsupported ministore tree: 7 (containers) root node - missing is root (0x02) flag.",
			function );

		goto on_error;
	}
	file_system->containers_root_node = root_node;
	root_node                         = NULL;
	if( libfsrefs_file_system_get_ministore_tree(
	     file_system,
	     io_handle,
	     file_io_handle,
	     8,
	     &copy_root_node,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			"%s: unable to retrieve ministore tree: 8 (containers) root node.",
			function );

		goto on_error;
	}
	if( ( copy_root_node->node_type_flags & 0x02 ) == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
			"%s: unsupported ministore tree: 8 (containers) root node - missing is root (0x02) flag.",
			function );

		goto on_error;
	}
	if( libfsrefs_ministore_node_free(
	     &copy_root_node,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
		 "%s: unable to free ministore tree: 8 (containers) root node.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( copy_root_node != NULL )
	{
		libfsrefs_ministore_node_free(
		 &copy_root_node,
		 NULL );
	}
	if( root_node != NULL )
	{
		libfsrefs_ministore_node_free(
		 &root_node,
		 NULL );
	}
	if( file_system->containers_root_node != NULL )
	{
		libfsrefs_ministore_node_free(
		 &( file_system->containers_root_node ),
		 NULL );
	}
	return( -1 );
}

/* Resolves the offsets of the block numbers in a block reference
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_get_block_offsets(
     libfsrefs_file_system_t *file_system,
     libfsrefs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libfsrefs_block_reference_t *block_reference,
     libcerror_error_t **error )
{
	static char *function         = "libfsrefs_file_system_get_block_offsets";
	uint64_t block_number         = 0;
	uint64_t container_identifier = 0;
	uint64_t mapped_block_number  = 0;
	uint64_t physical_block_number = 0;
	uint8_t block_number_index    = 0;
	uint8_t signature_data[ 4 ]   = { 0, 0, 0, 0 };
	int block_reference_mode      = 0;
	int result                    = 0;

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid IO handle.",
		 function );

		return( -1 );
	}
	if( file_io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file IO handle.",
		 function );

		return( -1 );
	}
	if( block_reference == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid block reference.",
		 function );

		return( -1 );
	}
	for( block_number_index = 0;
	     block_number_index < 4;
	     block_number_index++ )
	{
		block_number = block_reference->block_numbers[ block_number_index ];
		physical_block_number = block_number;

		if( block_number == 0 )
		{
			break;
		}
		if( ( io_handle->major_format_version == 3 )
		 && ( io_handle->container_size != 0 )
		 && ( block_number >= io_handle->container_size ) )
		{
			container_identifier = physical_block_number / io_handle->container_size;

			if( block_reference_mode == 0 )
			{
				result = libbfio_handle_read_buffer_at_offset(
				          file_io_handle,
				          signature_data,
				          4,
				          (off64_t) ( physical_block_number * io_handle->metadata_block_size ),
				          error );

				if( result != 4 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_IO,
					 LIBCERROR_IO_ERROR_READ_FAILED,
					 "%s: unable to read physical block signature at offset: %" PRIi64 ".",
					 function,
					 (off64_t) ( physical_block_number * io_handle->metadata_block_size ) );

					return( -1 );
				}
				if( memory_compare(
				     signature_data,
				     "MSB+",
				     4 ) == 0 )
				{
					block_reference_mode = 1;
				}
				else
				{
					block_number = physical_block_number % io_handle->container_size;

					result = libbfio_handle_read_buffer_at_offset(
					          file_io_handle,
					          signature_data,
					          4,
					          (off64_t) ( block_number * io_handle->metadata_block_size ),
					          error );

					if( result != 4 )
					{
						libcerror_error_set(
						 error,
						 LIBCERROR_ERROR_DOMAIN_IO,
						 LIBCERROR_IO_ERROR_READ_FAILED,
						 "%s: unable to read direct block signature at offset: %" PRIi64 ".",
						 function,
						 (off64_t) ( block_number * io_handle->metadata_block_size ) );

						return( -1 );
					}
					if( memory_compare(
					     signature_data,
					     "MSB+",
					     4 ) == 0 )
					{
						block_reference_mode = 2;
					}
					else
					{
						result = libfsrefs_file_system_get_container_block_number_by_identifier(
						          file_system,
						          io_handle,
						          file_io_handle,
						          container_identifier,
						          &mapped_block_number,
						          error );

						if( result == -1 )
						{
							libcerror_error_set(
							 error,
							 LIBCERROR_ERROR_DOMAIN_RUNTIME,
							 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
							 "%s: unable to retrieve container: %" PRIu64 " block number.",
							 function,
							 container_identifier );

							return( -1 );
						}
						if( result == 1 )
						{
							mapped_block_number += ( physical_block_number % io_handle->container_size );

							result = libbfio_handle_read_buffer_at_offset(
							          file_io_handle,
							          signature_data,
							          4,
							          (off64_t) ( mapped_block_number * io_handle->metadata_block_size ),
							          error );

							if( result != 4 )
							{
								libcerror_error_set(
								 error,
								 LIBCERROR_ERROR_DOMAIN_IO,
								 LIBCERROR_IO_ERROR_READ_FAILED,
								 "%s: unable to read mapped block signature at offset: %" PRIi64 ".",
								 function,
								 (off64_t) ( mapped_block_number * io_handle->metadata_block_size ) );

								return( -1 );
							}
							if( memory_compare(
							     signature_data,
							     "MSB+",
							     4 ) == 0 )
							{
								block_reference_mode = 3;
							}
						}
					}
				}
			}
			if( block_reference_mode == 1 )
			{
				block_number = physical_block_number;
			}
			else if( block_reference_mode == 2 )
			{
				block_number = physical_block_number % io_handle->container_size;
			}
			else if( block_reference_mode == 3 )
			{
				result = libfsrefs_file_system_get_container_block_number_by_identifier(
				          file_system,
				          io_handle,
				          file_io_handle,
				          container_identifier,
				          &block_number,
				          error );

				if( result != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to retrieve container: %" PRIu64 " block number.",
					 function,
					 container_identifier );

					return( -1 );
				}
				block_number += ( physical_block_number % io_handle->container_size );
			}
			else
			{
				block_number = physical_block_number % io_handle->container_size;
			}
		}
		block_reference->block_offsets[ block_number_index ] = (off64_t) ( block_number * io_handle->metadata_block_size );
	}
	return( 1 );
}

/* Retrieves the number of ministore trees
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_get_number_of_ministore_trees(
     libfsrefs_file_system_t *file_system,
     int *number_of_ministore_trees,
     libcerror_error_t **error )
{
	static char *function = "libfsrefs_file_system_get_number_of_ministore_trees";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( libfsrefs_checkpoint_get_number_of_ministore_tree_block_references(
	     file_system->checkpoint,
	     number_of_ministore_trees,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of ministore tree block references from checkpoint.",
		 function );

		return( -1 );
	}
	return( 1 );
}

/* Retrieves a specific ministore tree
 * Returns 1 if successful or -1 on error
 */
int libfsrefs_file_system_get_ministore_tree(
     libfsrefs_file_system_t *file_system,
     libfsrefs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     int ministore_tree_index,
     libfsrefs_ministore_node_t **root_node,
     libcerror_error_t **error )
{
	libfsrefs_block_reference_t *block_reference = NULL;
	libfsrefs_ministore_node_t *safe_root_node   = NULL;
	static char *function                        = "libfsrefs_file_system_get_ministore_tree";

	if( file_system == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file system.",
		 function );

		return( -1 );
	}
	if( root_node == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid root node.",
		 function );

		return( -1 );
	}
	if( libfsrefs_checkpoint_get_ministore_tree_block_reference_by_index(
	     file_system->checkpoint,
	     ministore_tree_index,
	     &block_reference,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve ministore tree: %d block reference from checkpoint.",
		 function,
		 ministore_tree_index );

		return( -1 );
	}
	if( libfsrefs_file_system_get_block_offsets(
	     file_system,
	     io_handle,
	     file_io_handle,
	     block_reference,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve ministore tree: %d block offsets.",
		 function,
		 ministore_tree_index );

		goto on_error;
	}
	if( libfsrefs_ministore_node_initialize(
	     &safe_root_node,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create ministore tree: %d root node.",
		 function,
		 ministore_tree_index );

		goto on_error;
	}
	if( libfsrefs_ministore_node_read_file_io_handle(
	     safe_root_node,
	     io_handle,
	     file_io_handle,
	     block_reference,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read ministore tree: %d root node at offset: %" PRIi64 " (0x%08" PRIx64 ").",
		 function,
		 ministore_tree_index,
		 block_reference->block_offsets[ 0 ],
		 block_reference->block_offsets[ 0 ] );

		goto on_error;
	}
	*root_node = safe_root_node;

	return( 1 );

on_error:
	if( safe_root_node != NULL )
	{
		libfsrefs_ministore_node_free(
		 &safe_root_node,
		 NULL );
	}
	return( -1 );
}

