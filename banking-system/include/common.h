#ifndef COMMON_H
#define COMMON_H
#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#define REQUEST_SIZE 1024
#define RESPONSE_SIZE 32768
#define MONEY_SIZE 40
#define MAX_RUPEES "1000000000.00"
enum
{
    CUSTOMER = 1,
    EMPLOYEE = 2,
    MANAGER = 3,
    ADMIN = 4
};

/**
 * @brief Send every byte of a null-terminated string through a connected TCP socket.
 *
 * @param[in] fd (int) Connected socket file descriptor.
 * @param[in] text (const char *) Null-terminated text to send, excluding its terminating null
 *                 byte.
 *
 * @return (int) 1 when all bytes are sent; 0 if sending fails or the connection closes.
 *
 * @note Retries interrupted and partial sends. Does not append a newline or close the socket.
 */
int send_all(int fd, const char *text);

/**
 * @brief Read one newline-terminated ASCII protocol line from a TCP socket.
 *
 * @param[in] fd (int) Connected socket file descriptor.
 * @param[out] text (char *) Writable buffer of size bytes; receives the line without its
 *                  newline.
 * @param[in] size (size_t) Buffer capacity in bytes, including space for the terminating null
 *                 byte; must be at least 1.
 *
 * @return (int) 1 when a complete line is stored as a null-terminated string; 0 on disconnect,
 *         receive error, invalid input, or insufficient buffer space.
 *
 * @note Ignores carriage returns and retries interrupted reads. Only a successful call
 *       guarantees a complete output string.
 */
int receive_line(int fd, char *text, size_t size);

/**
 * @brief Split a pipe-delimited request into fields by modifying its buffer in place.
 *
 * @param[in,out] text (char *) Writable null-terminated request; separators are replaced with
 *                     null bytes.
 * @param[out] fields (char **) Array with room for maximum pointers; receives pointers into
 *                    text, including empty fields.
 * @param[in] maximum (int) Maximum number of field pointers that can be stored; must be at
 *                    least 1.
 *
 * @return (int) The field count from 1 through maximum, or -1 if the request has too many
 *         fields.
 *
 * @note Returned pointers alias text and must not be freed individually. The input may already
 *       be modified when -1 is returned.
 */
int split_fields(char *text, char **fields, int maximum);

/**
 * @brief Parse a decimal integer used for an identifier or menu choice.
 *
 * @param[in] text (const char *) Null-terminated integer text to parse.
 *
 * @return (int) An integer from 0 through 2000000000, or -1 for empty, invalid, negative, or
 *         out-of-range input.
 *
 * @note Zero is valid here; callers reject it when an operation requires a positive identifier.
 */
int parse_id(const char *text);

/**
 * @brief Check a text field against the allowed character set and length bounds.
 *
 * @param[in] text (const char *) Null-terminated text to validate.
 * @param[in] minimum (int) Inclusive minimum character count.
 * @param[in] maximum (int) Inclusive maximum character count.
 *
 * @return (int) 1 when the length and every character are allowed; otherwise 0.
 *
 * @note Accepts alphanumeric characters and spaces or _-.@!#$%+?. Rejects quotes, backslashes,
 *       newlines, and the pipe protocol separator.
 */
int valid_text(const char *text, int minimum, int maximum);

/**
 * @brief Validate a positive Indian rupee amount and normalize it to two decimal places.
 *
 * @param[in] text (const char *) Null-terminated rupee text, such as "1.2" or "0.2", with at
 *                 most two fractional digits.
 * @param[out] amount (char *) Writable buffer of at least MONEY_SIZE bytes; receives normalized
 *                    decimal text on success.
 *
 * @return (int) 1 for a positive amount at most MAX_RUPEES; 0 for invalid, zero, or excessive
 *         amounts.
 *
 * @note Removes leading zeros and pads fractional digits, for example "1.2" becomes "1.20".
 *       Uses decimal strings without currency conversion or floating-point arithmetic.
 */
int rupee_amount(const char *text, char *amount);

/**
 * @brief Get the lowercase name associated with a role constant.
 *
 * @param[in] role (int) CUSTOMER, EMPLOYEE, MANAGER, or ADMIN role value.
 *
 * @return (const char *) A borrowed constant string containing the role name, or an empty
 *         string for an unknown role.
 *
 * @note The returned string has static lifetime; do not modify or free it.
 */
const char *role_name(int role);

/**
 * @brief Convert a lowercase role name into its numeric role constant.
 *
 * @param[in] name (const char *) Null-terminated name: "customer", "employee", "manager", or
 *                 "admin".
 *
 * @return (int) The matching role constant from 1 through 4; 0 for an unrecognized name.
 */
int role_number(const char *name);
#endif
