//
// Created by Anshul Gupta on 4/4/25.
//

#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	/// Pointer to the start of the buffer
	uint8_t *data;
	/// Size of the buffer
	size_t len;
	/// Size of the allocated buffer
	size_t cap;
} buffer_t;

/// Create a new buffer with the given initial capacity
buffer_t buffer_new(size_t cap);

/// Free the buffer
void buffer_free(buffer_t buf);

/// Resize the buffer to the given capacity
/// Does nothing if the new capacity is less than or equal to the
/// current capacity.
void buffer_reserve(buffer_t *buf, size_t cap);

/// Append data to the buffer
void buffer_append(buffer_t *buf, const void *data, size_t len);

#endif // BUFFER_H
