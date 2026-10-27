/* Small helpers shared by client and server; no database is used in this file. */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>

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
int send_all(int fd, const char *text)
{
    size_t left = strlen(text);
    while (left)
    {
        ssize_t n = send(fd, text, left, MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return 0;
        text += n;
        left -= (size_t)n;
    }
    return 1;
}

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
int receive_line(int fd, char *text, size_t size)
{
    size_t used = 0;
    for (;;)
    {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return 0;
        if (c == '\n')
        {
            text[used] = 0;
            return 1;
        }
        if (c == '\r')
            continue;
        if ((unsigned char)c < 32 || (unsigned char)c > 126 || used + 1 >= size)
            return 0;
        text[used++] = c;
    }
}

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
int split_fields(char *text, char **fields, int maximum)
{
    int count = 1;
    fields[0] = text;
    for (char *p = text; *p; p++)
    {
        if (*p == '|')
        {
            if (count == maximum)
                return -1;
            *p = 0;
            fields[count++] = p + 1;
        }
    }
    return count;
}

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
int parse_id(const char *text)
{
    char *end;
    errno = 0;
    long id = strtol(text, &end, 10);
    if (errno || !*text || *end || id < 0 || id > 2000000000)
        return -1;
    return (int)id;
}

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
int valid_text(const char *text, int minimum, int maximum)
{
    size_t size = strlen(text);
    if (size < (size_t)minimum || size > (size_t)maximum)
        return 0;
    for (size_t i = 0; i < size; i++)
        if (!isalnum((unsigned char)text[i]) && !strchr(" _-.@!#$%+?", text[i]))
            return 0;
    return 1;
}

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
int rupee_amount(const char *text, char *amount)
{
    const char *whole = text, *p = text;
    if (!isdigit((unsigned char)*p))
        return 0;
    while (isdigit((unsigned char)*p))
        p++;
    const char *end = p;
    while (end - whole > 1 && *whole == '0')
        whole++;
    size_t digits = (size_t)(end - whole);
    char fraction[2] = {'0', '0'};
    if (*p == '.')
    {
        p++;
        int places = 0;
        while (isdigit((unsigned char)*p) && places < 2)
            fraction[places++] = *p++;
        if (!places)
            return 0;
    }
    if (*p || digits > 10)
        return 0;
    snprintf(amount, MONEY_SIZE, "%.*s.%c%c", (int)digits, whole, fraction[0], fraction[1]);
    return strcmp(amount, "0.00") && !(digits == 10 && strcmp(amount, MAX_RUPEES) > 0);
}

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
const char *role_name(int role)
{
    switch (role)
    {
    case CUSTOMER:
        return "customer";
    case EMPLOYEE:
        return "employee";
    case MANAGER:
        return "manager";
    case ADMIN:
        return "admin";
    default:
        return "";
    }
}

/**
 * @brief Convert a lowercase role name into its numeric role constant.
 *
 * @param[in] name (const char *) Null-terminated name: "customer", "employee", "manager", or
 *                 "admin".
 *
 * @return (int) The matching role constant from 1 through 4; 0 for an unrecognized name.
 */
int role_number(const char *name)
{
    for (int role = 1; role <= 4; role++)
        if (!strcmp(name, role_name(role)))
            return role;
    return 0;
}
