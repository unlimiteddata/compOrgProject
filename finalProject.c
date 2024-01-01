#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <malloc.h>

// helper function to convert hex character to int
int hexCharToInt(char c) {
    if (isdigit(c)) { // if decimal digit
        return c - '0';
    } else if (isxdigit(c)) { // if hex digit
        return tolower(c) - 'a' + 10;
    } else {
        return -1; // Invalid hexadecimal character
    }
}

// helper function to convert hex string to int
int hexStringToInt(char *hex) {
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


int is_ascii(const char *input) {
    unsigned char c = (unsigned char)(*input);
    return c <= 127;
}

int my_utf8_encode(char *input, char *output) {
    if (input == NULL || output == NULL) {
        return -1; // Invalid
    }

    char *encoded = output;

    while (*input != '\0') {
        if ((*input == '\\' && *(input + 1) == 'u')) {
            input += 2; // Move past "\u"

            // Convert hexadecimal string to integer
            char hex[6] = {0}; // 5 hex digits + null terminator
            int i;
            for (i = 0; i < 5 && isxdigit(*input); ++i) {
                hex[i] = *(input++);
            }
            int codePoint = hexStringToInt(hex);

            // Encode the Unicode code point into UTF-8 using bit shifts
            if (codePoint <= 0x7F) { // one byte encoding
                *(encoded++) = (char)codePoint;
            } else if (codePoint <= 0x7FF) { // two byte encoding
                *(encoded++) = (char)(0xC0 | ((codePoint >> 6) & 0x1F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            } else if (codePoint <= 0xFFFF) { // three byte encoding
                *(encoded++) = (char)(0xE0 | ((codePoint >> 12) & 0x0F));
                *(encoded++) = (char)(0x80 | ((codePoint >> 6) & 0x3F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            } else if (codePoint <= 0x10FFFF) { // four byte encoding
                *(encoded++) = (char)(0xF0 | ((codePoint >> 18) & 0x07));
                *(encoded++) = (char)(0x80 | ((codePoint >> 12) & 0x3F));
                *(encoded++) = (char)(0x80 | ((codePoint >> 6) & 0x3F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            } else {
                return -1; // Invalid Unicode code point
            }
        } else {
            *(encoded++) = *(input++);
        }
    }

    *encoded = '\0'; // Null-terminate the encoded string
    return 0; // Success
}

int my_utf8_decode(char *input, char *output) {
    while (*input != '\0') {
        if (is_ascii(input)) {
            *output = *input;
            ++input;
            ++output;
        } else {
            // Non-ASCII character, handle UTF-8 decoding
            uint32_t codePoint = 0;
            int  numBytes = 0;

            // Determine the number of bytes in the UTF-8 sequence
            if ((*input & 0b10000000) == 0) {
                // Single-byte character
                codePoint = *input;
                numBytes = 1;
            } else if ((*input & 0b11100000) == 0b11000000) {
                // Two-byte character
                codePoint = *input & 0b00011111;
                numBytes = 2;
            } else if ((*input & 0b11110000) == 0b11100000) {
                // Three-byte character
                codePoint = *input & 0b00001111;
                numBytes = 3;
            } else if ((*input & 0b11111000) == 0b11110000) {
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
int my_utf8_check(char *string) {
    while (*string != '\0') {
        if ((*string & 0b10000000) == 0) {
            // single byte char
            ++string;
        } else if ((*string & 0b11100000) == 0b11000000) {
            // two byte char
            if ((string[1] & 0b11000000) != 0b10000000) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 2;
        } else if ((*string & 0b11110000) == 0b11100000) {
            // three byte char
            if ((string[1] & 0b11000000) != 0b10000000
                || (string[2] & 0b11000000) != 0b10000000) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 3;
        } else if ((*string & 0b11111000) == 0b11110000) {
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

int my_utf8_strlen(char *string){
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


char *my_utf8_charat(char *string, int index){
    if (index < 0){
        return NULL;
    }

    int i = 0;
    while (string[i] != '\0' && index > 0){
        if (my_utf8_check(&string[i])) {
            uint8_t byte = (uint8_t)string[i];
            index -= (byte & 0xC0) != 0x80 ? 1: 0;
        }
        else {
            return NULL;
        }

        i++;
    }

    if (index == 0){
        char *character = malloc(5);
        if (character != NULL){
            int j;
            for (j = 0; j < 4 && (string[i+j] & 0xC0) == 0x80; ++j){
                character[j] = string[i+j];
            }
            character[j] = '\0';
        }
        return character;
    }
    else {
        return NULL;
    }
}


int main() {
    char input[] = "Hello \\u05D0\\u05E8\\u05D9\\u05D4 \\u1F601"; // Hebrew characters in Codepoint notation
    char encoded[50] = {0};
    char decoded[50] = {0};

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

    printf("Input: %s - ", encoded);
    if (my_utf8_check(encoded)) {
        printf("Valid UTF-8\n");
    } else {
        printf("Invalid UTF-8\n");
    }

    char invalid[] = "\xc1\xa8\x81";
    printf("Input: %s - ", invalid);
    if (my_utf8_check(invalid)) {
        printf("Valid UTF-8\n");
    } else {
        printf("Invalid UTF-8\n");
    }

    // Test my_utf8_strlen
    printf("\nTesting my_utf8_strlen:");
    char lenInput[] = "Hello אריה";
    printf("\nLength of \"%s\":\n%d\n", lenInput, my_utf8_strlen(lenInput));

//    // Test my_utf8_charat
//    int index = 1; // Replace with the desired index
//    char *charAtIndex = my_utf8_charat(encoded, index);
//    if (charAtIndex != NULL) {
//        printf("\nCharacter at index %d of %s: %s\n", index, encoded, charAtIndex);
//        free(charAtIndex); // Free the allocated memory
//    } else {
//        printf("Invalid index or encoding\n");
//    }

    return 0;
}