//
// Created by Anshul Gupta on 4/4/25.
//

#include "buffer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"

buffer_t buffer_new(size_t cap)
{
	buffer_t buf;

	if (cap == 0)
		return (buffer_t) {NULL, 0, 0};

	cap = (cap + 7) & ~7; // Align to 8 bytes
	buf.data = gmalloc(cap);
	if (!buf.data)
		abort();

	return (buffer_t) {buf.data, 0, cap};
}

void buffer_free(buffer_t buf) { gfree(buf.data); }

void buffer_reserve(buffer_t *buf, size_t cap)
{
	if (buf->cap >= cap)
		return;

	cap = (cap + 7) & ~7; // Align to 8 bytes
	uint8_t *new_data = grealloc(buf->data, cap);
	if (!new_data)
		abort();
	buf->data = new_data;
	buf->cap = cap;
}

void buffer_append(buffer_t *buf, const void *data, size_t len)
{
	if (buf->len + len > buf->cap)
		buffer_reserve(buf, buf->len + len);

	memcpy(buf->data + buf->len, data, len);
	buf->len += len;
	assert(buf->len <= buf->cap);
}
