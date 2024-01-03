#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

// helper function to convert hex character to int
int hexCharToInt(unsigned char c) {
    if (isdigit(c)) { // if decimal digit
        return c - '0';
    }
    else if (isxdigit(c)) { // if hex digit
        return tolower(c) - 'a' + 10;
    }
    else {
        return -1; // Invalid hexadecimal character
    }
}

// helper function to convert hex string to int
int hexStringToInt(unsigned char *hex) {
    int result = 0;
    while (*hex != '\0') {
        int digit = hexCharToInt(*(hex++));
        if (digit < 0) {
            return -1; // Invalid hexadecimal string
        }
        result = result * 16 + digit;
    }
    return result;
}

// helper function to check if a given character is ASCII
int is_ascii(unsigned const char *input) {
    unsigned char c = (unsigned char)(*input);
    return c <= 127;
}

// Encoding a UTF8 string, taking as input an ASCII string,
// with UTF8 characters encoded using the Codepoint numbering
// scheme notation, and returns a UTF8 encoded string.
int my_utf8_encode(unsigned char *input, unsigned char *output) {
    if (input == NULL || output == NULL) {
        return -1; // Invalid
    }

    unsigned char *encoded = output;

    while (*input != '\0') {
        if ((*input == '\\' && *(input + 1) == 'u')) {
            input += 2; // Move past "\u"

            // Convert hexadecimal string to integer
            unsigned char hex[6] = {0}; // 5 hex digits + null terminator
            int i;
            for (i = 0; i < 5 && isxdigit(*input); ++i) {
                hex[i] = *(input++);
            }
            int codePoint = hexStringToInt(hex);

            // Encode the Unicode code point into UTF-8 using bit shifts
            if (codePoint <= 0x7F) { // one byte encoding
                *(encoded++) = (char)codePoint;
            }
            else if (codePoint <= 0x7FF) { // two byte encoding
                *(encoded++) = (char)(0xC0 | ((codePoint >> 6) & 0x1F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            }
            else if (codePoint <= 0xFFFF) { // three byte encoding
                *(encoded++) = (char)(0xE0 | ((codePoint >> 12) & 0x0F));
                *(encoded++) = (char)(0x80 | ((codePoint >> 6) & 0x3F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            }
            else if (codePoint <= 0x10FFFF) { // four byte encoding
                *(encoded++) = (char)(0xF0 | ((codePoint >> 18) & 0x07));
                *(encoded++) = (char)(0x80 | ((codePoint >> 12) & 0x3F));
                *(encoded++) = (char)(0x80 | ((codePoint >> 6) & 0x3F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            }
            else {
                return -1; // Invalid Unicode code point
            }
        }
        else {
            *(encoded++) = *(input++);
        }
    }

    *encoded = '\0'; // Null-terminate the encoded string
    return 0; // Success
}

// Takes a UTF8 encoded string, and returns a string, with ASCII
// representation where possible, and UTF8 character representation
// for non-ASCII characters.
int my_utf8_decode(unsigned char *input, unsigned char *output) {
    while (*input != '\0') {
        if (is_ascii(input)) {
            *output = *input;
            ++input;
            ++output;
        }
        else {
            // Non-ASCII character, handle UTF-8 decoding
            uint32_t codePoint = 0;
            int  numBytes = 0;

            // Determine the number of bytes in the UTF-8 sequence
            if ((*input & 0b10000000) == 0) {
                // Single-byte character
                // Clang-Tidy: 'signed char' to 'uint32_t' (aka 'unsigned int') conversion; consider casting to 'unsigned char' first.
                codePoint = *input;
                numBytes = 1;
            }
            else if ((*input & 0b11100000) == 0b11000000) {
                // Two-byte character
                codePoint = *input & 0b00011111;
                numBytes = 2;
            }
            else if ((*input & 0b11110000) == 0b11100000) {
                // Three-byte character
                codePoint = *input & 0b00001111;
                numBytes = 3;
            }
            else if ((*input & 0b11111000) == 0b11110000) {
                // Four-byte character
                codePoint = *input & 0b00000111;
                numBytes = 4;
            }

            // Read the remaining bytes of the UTF-8 sequence
            for (int i = 1; i <  numBytes; ++i) {
                if ((input[i] & 0b11000000) != 0b10000000) {
                    // Malformed UTF-8 sequence
                    return -1;
                }
                codePoint = (codePoint << 6) | (input[i] & 0b00111111);
            }

            int digits = 1;
            uint32_t  temp = codePoint;
            while (temp >>= 4){
                ++digits;
            }

            int width = (digits < 4) ? 4 : digits;

            sprintf(output, "u\\%.*X", width, codePoint);

            output += width + 2; // Move the output pointer to the end of the codePoint
            input +=  numBytes; // Move the input pointer to the next character
        }
    }

    *output = '\0'; // Null-terminate the output string
    return 0;       // Success
}

// validate that the input string is a valid UTF8 encoded string
int my_utf8_check(unsigned char *string) {
    while (*string != '\0') {
        if ((*string & 0b10000000) == 0) {
            // single byte char
            ++string;
        }
        else if ((*string & 0b11100000) == 0b11000000) {
            // two byte char
            if ((string[1] & 0b11000000) != 0b10000000) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 2;
        }
        else if ((*string & 0b11110000) == 0b11100000) {
            // three byte char
            if ((string[1] & 0b11000000) != 0b10000000
                || (string[2] & 0b11000000) != 0b10000000) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 3;
        }
        else if ((*string & 0b11111000) == 0b11110000) {
            // four byte char
            if ((string[1] & 0b11000000) != 0b10000000
                || (string[2] & 0b11000000) != 0b10000000
                || (string[3] & 0b11000000) != 0b10000000) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 4;
        }
        else {
            return 0;
        }
    }
    return 1;
}

// return the number of characters in a UTF8 encoded string
int my_utf8_strlen(unsigned char *string){
    int len = 0;

    while (*string){
        uint8_t byte = (uint8_t)*string;

        if ((byte & 0x80) == 0){
            len++;
            string++;
        }
        else if ((byte & 0xE0) == 0xC0){
            len++;
            string+=2;
        }
        else if ((byte & 0xF0) == 0xE0){
            len++;
            string += 3;
        }
        else if ((byte & 0xF8) == 0xF0){
            len++;
            string+=4;
        }
        else {
            string++;
        }
    }
    return len;
}

// Returns the UTF8 encoded character at the location specified.
// If the input string is improperly encoded, this function should
// return NULL to indicate an error.
unsigned char *my_utf8_charat(unsigned char *string, int index) {
    if (string == NULL || index < 0) {
        return NULL;  // Invalid input
    }

    int i = 0;
    while (string[i] != '\0') {
        if ((string[i] & 0x80) == 0) {
            // Single-byte character
            if (index == 0) {
                return &string[i];
            }
            index--;
        }
        else if ((string[i] & 0xE0) == 0xC0) {
            // Two-byte character
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80) {
                return &string[i];
            }
            index--;
            i++;
        }
        else if ((string[i] & 0xF0) == 0xE0) {
            // Three-byte character
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80) {
                return &string[i];
            }
            index--;
            i += 2;
        }
        else if ((string[i] & 0xF8) == 0xF0) {
            // Four-byte character
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80 && (string[i + 3] & 0xC0) == 0x80) {
                return &string[i];
            }
            index--;
            i += 3;
        }
        else {
            // Invalid UTF-8 character
            return NULL;
        }
        i++;
    }

    // Index out of bounds or invalid UTF-8 encoding
    return NULL;
}

// Returns whether the two strings are the same (similar result set to strcmp())
int my_utf8_strcmp(unsigned char *string1, unsigned char *string2) {
    while (*string1 != '\0' && *string2 != '\0') {
        // Iterate over characters in both strings
        unsigned char char1 = *string1;
        unsigned char char2 = *string2;

        if (char1 < 128 && char2 < 128) {
            // Both characters are ASCII
            if (char1 != char2) {
                return char1 - char2;  // ASCII comparison
            }
        }
        else {
            // At least one character is non-ASCII (UTF-8)
            while ((*string1 & 0xC0) == 0x80) string1++;  // Skip UTF-8 continuation bytes
            while ((*string2 & 0xC0) == 0x80) string2++;

            // Compare UTF-8 characters
            int diff;
            while ((*string1 & 0xC0) == 0x80 && (*string2 & 0xC0) == 0x80) {
                diff = *string1++ - *string2++;
                if (diff != 0) {
                    return diff;
                }
            }

            // If one string is shorter, return the difference in length
            if ((*string1 & 0xC0) == 0x80 || (*string2 & 0xC0) == 0x80) {
                return (*string1 & 0xC0) - (*string2 & 0xC0);
            }

            // Compare the first non-ASCII character
            diff = *string1 - *string2;
            if (diff != 0) {
                return diff;
            }
        }

        // Move to the next character
        string1++;
        string2++;
    }

    // Check if one string is shorter than the other
    if (*string1 != '\0') {
        return 1;
    }
    else if (*string2 != '\0') {
        return -1;
    }

    // Strings are equal
    return 0;
}

// EXTRA FUNCTIONS:
int my_utf8_substring_search(unsigned char *haystack, unsigned char *needle, size_t *indices) {
    if (haystack == NULL || needle == NULL || indices == NULL) {
        return -1; // Invalid input
    }

    size_t haystack_len = my_utf8_strlen(haystack);
    size_t needle_len = my_utf8_strlen(needle);

    if (needle_len > haystack_len) {
        return 0; // Substring is longer than the string, no match possible
    }

    int count = 0; // Number of occurrences found

    // Iterate through the haystack
    for (size_t i = 0; i <= haystack_len - needle_len; ++i) {
        // Check if the substring matches at the current position
        size_t j;
        for (j = 0; j < needle_len; ++j) {
            if (haystack[i + j] != needle[j]) {
                break; // Substring doesn't match at this position
            }
        }

        // If the inner loop completed without breaking, the substring is found
        if (j == needle_len) {
            indices[count++] = i;
        }
    }

    return count;
}

// Function to remove whitespace from a UTF-8 encoded string
unsigned char* my_utf8_remove_whitespace(unsigned const char *input) {
    if (input == NULL) {
        return NULL;
    }

    // Allocate memory for the result string
    size_t inputLength = 0;
    while (input[inputLength] != '\0') {
        inputLength++;
    }

    unsigned char *result = (unsigned char*)malloc((inputLength + 1) * sizeof(char));
    if (result == NULL) {
        return NULL;
    }

    // Iterate through the UTF-8 string, copying non-whitespace characters to the result
    size_t resultIndex = 0;
    for (size_t i = 0; i < inputLength; ++i) {
        uint8_t currentChar = (uint8_t)(input[i]);

        // Check if the character is a whitespace character
        if (currentChar != ' ' && currentChar != '\t' && currentChar != '\n' && currentChar != '\r') {
            result[resultIndex++] = input[i];
        }
    }

    // Null-terminate the result string
    result[resultIndex] = '\0';

    return result;
}


int main() {
    unsigned char input[] = "Hello \\u05D0\\u05E8\\u05D9\\u05D4 \\u1F601";
    unsigned char encoded[50] = {0};
    unsigned char decoded[50] = {0};

    // Test my_utf8_encode
    printf("Testing my_utf8_encode:\n");
    printf("Input: %s\n", input);
    int encodeResult = my_utf8_encode(input, encoded);
    if (encodeResult == 0) {
        printf("Encoded: %s\n", encoded);
    } else {
        printf("Encoding failed\n");
        return 1;
    }

    // Test my_utf8_decode
    printf("\nTesting my_utf8_decode:\n");
    printf("Input: %s\n", encoded);
    int decodeResult = my_utf8_decode(encoded, decoded);
    if (decodeResult == 0) {
        printf("Decoded: %s\n", decoded);
    } else {
        printf("Decoding failed\n");
        return 1;
    }

    // Test my_utf8_check
    printf("\nTesting my_utf8_check:\n");
    printf("Input: %s - ", input);
    if (my_utf8_check(input)) {
        printf("Valid UTF-8\n");
    } else {
        printf("Invalid UTF-8\n");
    }


    // Test my_utf8_check
    unsigned char test[] = "\xd7\x90";
    unsigned char backslashTest[] = "\\xd7\\x90";
    printf("Input: %s- ", backslashTest);
    if (my_utf8_check(test)) {
        printf("Valid UTF-8\n");
    } else {
        printf("Invalid UTF-8\n");
    }

    printf("Input: %s - ", encoded);
    if (my_utf8_check(encoded)) {
        printf("Valid UTF-8\n");
    } else {
        printf("Invalid UTF-8\n");
    }

    unsigned char invalid[] = "\xc1\xa8\x81";
    printf("Input: %s - ", invalid);
    if (my_utf8_check(invalid)) {
        printf("Valid UTF-8\n");
    } else {
        printf("Invalid UTF-8\n");
    }

    // Test my_utf8_strlen
    printf("\nTesting my_utf8_strlen:");
    unsigned char lenInput1[] = "Hello אריה";
    printf("\nLength of \"%s\":\n%d\n", lenInput1, my_utf8_strlen(lenInput1));
    unsigned char lenInput2[] = "\xd7\x90\xd7\xaa";
    unsigned char lenInput2backslash[] = "\\xd7\\x90\\xd7\\xaa";
    printf("Length of \"%s\":\n%d\n", lenInput2backslash, my_utf8_strlen(lenInput2));

    // Test my_utf8_charat
    printf("\nTesting my_utf8_charat:\n");
    unsigned char charAtInput[] = "Hello אריה";
    for (int index = 0; charAtInput[index] != '\0'; ++index) {
        unsigned char *result = my_utf8_charat(charAtInput, index);
        if (result != NULL) {
            printf("Character at index %d: %.*s\n", index,
                   (int) (my_utf8_charat(charAtInput, index + 1) - my_utf8_charat(charAtInput, index)),
                   my_utf8_charat(charAtInput, index));
        } else {
            printf("Index %d: Invalid index or encoding\n", index);
            break;
        }
    }


    printf("\nTesting my_utf8_strcmp():\n");
    unsigned char utf8_string1[] = "Hello 世界!";
    unsigned char utf8_string2[] = "Hello 世界!";
    unsigned char utf8_string3[] = "No";
    unsigned char utf8_string4[] = "Yes";
    unsigned char utf8_chinese[] = "你好";
    unsigned char utf8_hebrew[] = "שלום";

    printf("Comparing %s and %s: ", utf8_string1, utf8_string2);
    int result1 = my_utf8_strcmp(utf8_string1, utf8_string2);

    if (result1 == 0) {
        printf("Strings are equal.\n");
    } else if (result1 < 0) {
        printf("String 1 is before String 2.\n");
    } else {
        printf("String 1 is after String 2.\n");
    }

    printf("Comparing %s and %s: ", utf8_string1, utf8_string3);
    int result2 = my_utf8_strcmp(utf8_string1, utf8_string3);
    if (result2 == 0) {
        printf("Strings are equal.\n");
    } else if (result2 < 0) {
        printf("String 1 is before String 2.\n");
    } else {
        printf("String 1 is after String 2.\n");
    }

    printf("Comparing %s and %s: ", utf8_string4, utf8_string3);
    int result3 = my_utf8_strcmp(utf8_string4, utf8_string3);
    if (result3 == 0) {
        printf("Strings are equal.\n");
    } else if (result3 < 0) {
        printf("String 1 is before String 2.\n");
    } else {
        printf("String 1 is after String 2.\n");
    }

    printf("Comparing %s and %s: ", utf8_chinese, utf8_hebrew);
    int result4 = my_utf8_strcmp(utf8_chinese, utf8_hebrew);
    if (result4 == 0) {
        printf("Strings are equal.\n");
    } else if (result4 < 0) {
        printf("String 1 is before String 2.\n");
    } else {
        printf("String 1 is after String 2.\n");
    }

    printf("\nTesting my_utf8_substring_search:\n");

    unsigned char haystack1[] = "Hello Amira";
    unsigned char haystack2[] = "Hello Amira Amira";
    unsigned char needle1[] = "Amira";
//    unsigned char needle2[] = "";

    size_t indices[10]; // Assuming a maximum of 10 occurrences
    int count = my_utf8_substring_search(haystack1, needle1, indices);
    printf("String: %s, substring: %s\n", haystack1, needle1);

    if (count == 1) {
        printf("Substring found at index: %zu\n", indices[0]);
    } else if (count > 0) {
        printf("Substring found at indices: ");
        for (int i = 0; i < count; ++i) {
            printf("%zu ", indices[i]);
        }
        printf("\n");
    } else {
        printf("Substring not found.\n");
    }

    count = my_utf8_substring_search(haystack2, needle1, indices);
    printf("String: %s, substring: %s\n", haystack2, needle1);

    if (count == 1) {
        printf("Substring found at index: %zu\n", indices[0]);
    } else if (count > 0) {
        printf("Substring found at indices: ");
        for (int i = 0; i < count; ++i) {
            printf("%zu ", indices[i]);
        }
        printf("\n");
    } else {
        printf("Substring not found.\n");
    }

    printf("\nTesting my_utf8_remove_whitespace:\n");
    unsigned char testString[] = " Hello, \t World!\nאמירה    \t\n 😂";

    // Remove whitespace from the test string
    unsigned char *result = my_utf8_remove_whitespace(testString);

    // Print the result
    if (result != NULL) {
        printf("Original String: \"%s\"\n", testString);
        printf("String without Whitespace: \"%s\"\n", result);

        // Free the allocated memory
        free(result);
    }

    return 0;
}