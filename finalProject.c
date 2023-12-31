#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

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
    if (input == NULL || output == NULL) { // empty string input
        return -1; // Invalid 
    }

    char *encoded = output; // store result 

    while (*input != '\0') {
        if (*input == 'U' && *(input + 1) == '+') {
            input += 2; // Move past "U+"

            // Convert hexadecimal string to integer
            char hex[7] = {0}; // 6 hex digits + null terminator
            int i;
            for (i = 0; i < 6 && isxdigit(*input); ++i) {
                hex[i] = *(input++); 
            } 
            int codePoint = hexStringToInt(hex);

            // Encode the Unicode code point into UTF-8 using bit shifts 
            if (codePoint <= 0x7F) { // one byte encoding
                *(encoded++) = (char)codePoint;
            } else if (codePoint <= 0x7FF) { // two byte encoding
                // first byte: right-shift code point by 6 - gets upper 5
                // bits of code point. mask result with 0x1F (0001111 in binary) 
                // to consider only five lower bits; combine with leading 
                // prefix 110 (0xC0) 
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

            sprintf(output, "U+%.*X", width, codePoint);

            output += width + 2; // Move the output pointer to the end of the codePoint
            input +=  numBytes; // Move the input pointer to the next character
        }
    }

    *output = '\0'; // Null-terminate the output string
    return 0;       // Success
}


int main() {
    char input1[] = "Hello U+0048U+0065U+006CU+006CU+006FU+0021 U+1F601 U+5D0";
    printf("Input: %s\n", input1);
    char output1[100];

    if (my_utf8_encode(input1, output1) == 0) {
        printf("Encoded string: %s\n", output1);
    } else {
        printf("Encoding failed.\n");
    }


    char input2[] = "😁 Hello, אמירה";
    printf("Input: %s\n", input2);
    char output2[10000];

    if (my_utf8_decode(input2, output2) == 0) {
        printf("Decoded string: %s\n", output2);
    } else {
        printf("Decoding failed.\n");
    }
    return 0;

}
