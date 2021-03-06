
/**
 * @file /magma/core/strings/nuller.c
 *
 * @brief	Functions for handling null terminated strings.
 */

#include "magma.h"

/// LOW: The ns_ functions are designed to work with NULL terminated strings. They shouldn't require a length. If the length needs to be
/// 	provided, then the equivalent mm_ function should be used.

/**
 * @brief	Return the length of a null-terminated string.
 * @param	s	the input as a null-terminatd string.
 * @return	the length in bytes of the string.
 */
size_t ns_length_get(const chr_t *s) {
	size_t len = 0;
	while (*s++) len++;
	return len;
}

/**
 * @brief	Return whether a null-terminated string is empty or not.
 * @param	s	the input as a null-terminated string.
 * @return	true if string is NULL or zero length; false otherwise.
 */
bool_t ns_empty(chr_t *s) {
	if (!s || !ns_length_get(s)) return true;
	return false;
}

/**
 * @brief	Return whether a null-terminated string is empty or not, while storing its address and length.
 * @param	s	the input as a null-terminated string.
 * @param	ptr	a pointer address to receive a copy of the string's location.
 * @param	len	a pointer to a variable to receive the length of the string, in bytes.
 * @return	true if string is NULL or zero length; false otherwise.
 */
bool_t ns_empty_out(chr_t *s, chr_t **ptr, size_t *len) {
	if (!(*ptr = s) || !(*len = ns_length_get(s))) return true;
	return false;
}

/**
 * @brief	Return the length of a null-terminated string as an int, capped at INT_MAX.
 * @param	s	the null-terminated string to be evaluated.
 * @return	the length of the string.
 */
int ns_length_int(chr_t *s) {

	size_t len = ns_length_get(s);

	if (len > INT_MAX) {
		log_pedantic("Requested length is greater than INT_MAX. {nuller.length = %zu}", len);
		return INT_MAX;
	}

	return len;
}

/**
 * @brief	Allocate (and wipe) a buffer for a null-terminated string.
 * @param	len	the length of the buffer to be allocated.
 * @return	NULL on failure or if len was 0; a pointer to the newly allocated memory otherwise.
 */
chr_t * ns_alloc(size_t len) {

	chr_t *result;

	// No zero length strings.
	if (len == 0) {
		return NULL;
	}

	// Do the allocation. Include room for two sizer_ts plus a terminating NULL. If memory was allocated clear the buffer.
	if ((result = mm_alloc(len + 1)) != NULL) {
		ns_wipe(result, len);
	}

	// If no memory was allocated, discover that here.
	else {
		log_pedantic("Could not allocate %zu bytes.", len + 1);
	}

	return result;
}

/**
 * @brief	Free a null-terminated string.
 * @param	s	the string to be freed.
 * @return	This function returns no value.
 */
void ns_free(chr_t *s) {

#ifdef MAGMA_PEDANTIC
	if (s == NULL)
		log_pedantic("Attempted to free a NULL pointer.");
#endif

	if (s) mm_free(s);
	return;
}

/**
 * @brief	A simple method for checking multiple NULL terminated strings to ensure all of the provided pointers lead to a
 * 				buffer with at least one non-NULL character.
 *
 * @param	len		the number of pointers being passed in.
 * @param	va_list	the list of NULL terminated string pointers to validate.
 *
 * @return	true if none of the pointers are NULL, and all point to at least one non-NULL byte, otherwise false.
 */
bool_t ns_populated_variadic(ssize_t len, ...) {

	va_list list;
	chr_t *s = NULL;

	va_start(list, len);

	// Loop through the inputs, and immediately return true if any of the inputs is empty.
	for (ssize_t i = 0; i < len; i++) {
		s = va_arg(list, chr_t *);
		// Ensure the pointer is valid.
		if (!s) return false;

		// Ensure the buffer doesn't start with a NULL byte.
		if (!(*s)) return false;
	}

	va_end(list);

	// Return true if we made it this far, unless the list of strings was empty.
	return (len ? true : false);
}

/**
 * @brief	A checked cleanup function which can be used free a variable number of null-terminated strings.
 * @see		ns_free()
 *
 * @param	va_list	a list of null-terminated strings to be freed.
 *
 * @return	This function returns no value.
 */
void ns_cleanup_variadic(ssize_t len, ...) {

	va_list list;
	chr_t *s = NULL;

	va_start(list, len);

	for (ssize_t i = 0; i < len; i++) {
		s = va_arg(list, chr_t *);
		if (s) ns_free(s);
	}
	va_end(list);

	return;
}

/**
 * @brief	Duplicate a null-terminated string.
 * @param	s	the null-terminated string to be duplicated.
 * @return	NULL on failure, or a pointer to a copy of the input string.
 */
chr_t * ns_dupe(chr_t *s) {

	chr_t *result;
	size_t length;

	if (!s || (!(length = ns_length_get(s)))) {
		log_pedantic("Cannot duplicate NULL or zero-length string.");
		return NULL;
	}

	if (!(result = mm_alloc(length + 1))) {
		log_info("Was not able to allocate buffer for duplicate string.");
		return NULL;
	}

	mm_copy(result, s, length);
	return result;
}

/**
 * @brief	Get a block of memory as a null-terminated string.
 * @param	block	a pointer to the buffer containing the data to be copied.
 * @param	len		the length, in bytes, of the data buffer to be copied.
 * @return	NULL on failure, or a pointer to the newly allocated null-terminated string on success.
 */
chr_t * ns_import(void *block, size_t len) {

	chr_t *result;

	if (!(result = mm_alloc(len + 1))) {
		log_pedantic("Allocation of copied memory failed.");
		return NULL;
	}

	mm_copy(result, block, len);
	return result;
}

/**
 * @brief	Append one string to another and return the result.
 * @param	s		a pointer to a leading null-terminated string to to which the other string will be appended.
 * @param	append	a pointer to a null-terminated string to be appended to the leading string.
 * @return	NULL if a memory allocation failure occurred, or a pointer to the resulting string on success.
 */
chr_t * ns_append(chr_t *s, chr_t *append) {

	chr_t *output = NULL;
	size_t alen, slen = 0;

	if (!append || !(alen = ns_length_get(append))) {
		log_pedantic("The append string appears to be empty.");
		return s;
	}

	// Allocate a new string if the existing string pointer is NULL.
	if (!s) {
		s = ns_dupe(append);
	}
	else if (!(slen = ns_length_get(s))) {
		ns_free(s);
		s = ns_dupe(append);
	}
	// Otherwise check the amount of available space in the buffer and if necessary allocate more.
	else if ((output = ns_alloc(slen + alen + 1))) {
		mm_copy(output, s, slen);
		mm_copy(output + slen, append, alen);
		ns_free(s);
		s = output;
	}

	return s;
}

/**
 * @brief	Zero out a null-terminated string.
 * @param	s	the string to be wiped.
 * @param	len	the number of bytes to be wiped at the start of the string.
 * @return	This function returns no parameters.
 */
void ns_wipe(chr_t *s, size_t len) {

#ifdef MAGMA_PEDANTIC
	if (!s) log_pedantic("Attempting to wipe a NULL string pointer.");
#endif

	if (s && len) mm_set(s, 0, len);
	return;
}
