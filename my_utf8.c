#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
int isASCII(unsigned const char *input) {
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


            if (i==0){
                *(encoded++) = '\\';
                *(encoded++) = 'u';
                continue;
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
        if (isASCII(input)) {
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

            sprintf(output, "\\u%.*X", width, codePoint);

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
            if ((string[1] & 0b11000000) != 0b10000000 ||
                (*string & 0b00011111) == 0 ||
                (string[1] & 0b00111111) == 0) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 2;
        } 
        else if ((*string & 0b11110000) == 0b11100000) {
            // three byte char
            if ((string[1] & 0b11000000) != 0b10000000 ||
                (string[2] & 0b11000000) != 0b10000000 ||
                (*string & 0b00001111) == 0 ||
                (string[1] & 0b00111111) == 0 ||
                (string[2] & 0b00111111) == 0) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 3;
        } 
        else if ((*string & 0b11111000) == 0b11110000) {
            // four byte char
            if ((string[1] & 0b11000000) != 0b10000000 ||
                (string[2] & 0b11000000) != 0b10000000 ||
                (string[3] & 0b11000000) != 0b10000000 ||
                (*string & 0b00000111) == 0 ||
                (string[1] & 0b00111111) == 0 ||
                (string[2] & 0b00111111) == 0 ||
                (string[3] & 0b00111111) == 0) {
                // incorrect UTF-8 sequence
                return 0;
            }
            string += 4;
        } 
        else {
            // invalid UTF-8 sequence
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
//// return NULL to indicate an error.
//unsigned char *my_utf8_charat(unsigned const char *string, int index) {
//    if (string == NULL || index < 0) {
//        return NULL;  // Invalid input
//    }
//
//    int i = 0;
//    while (string[i] != '\0') {
//        if ((string[i] & 0x80) == 0) {
//            // Single-byte character
//            if (index == 0) {
//                static unsigned char result[2];
//                result[0] = string[i];
//                result[1] = '\0';
//                return result;
//            }
//            index--;
//        }
//        else if ((string[i] & 0xE0) == 0xC0) {
//            // Two-byte character
//            if (index == 0 && (string[i + 1] & 0xC0) == 0x80) {
//                static unsigned char result[3];
//                result[0] = string[i];
//                result[1] = string[i+1];
//                result[2] = '\0';
//                return result;
//            }
//            index--;
//            i++;
//        }
//        else if ((string[i] & 0xF0) == 0xE0) {
//            // Three-byte character
//            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80) {
//                static unsigned char result[4];
//                result[0] = string[i];
//                result[1] = string[i+1];
//                result[2] = string[i+2];
//                result[3] = '\0';
//                return result;
//            }
//            index--;
//            i += 2;
//        }
//        else if ((string[i] & 0xF8) == 0xF0) {
//            // Four-byte character
//            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80 && (string[i + 3] & 0xC0) == 0x80) {
//                static unsigned char result[5];
//                result[0] = string[i];
//                result[1] = string[i+1];
//                result[2] = string[i+2];
//                result[3] = string[i+3];
//                result[4] = '\0';
//                return result;
//            }
//            index--;
//            i += 3;
//        }
//        else {
//            // Invalid UTF-8 character
//            return NULL;
//        }
//        i++;
//    }
//
//    // Index out of bounds or invalid UTF-8 encoding
//    return NULL;
//}



unsigned char *my_utf8_charat(const unsigned char *string, int index) {
    if (string == NULL || index < 0) {
        return NULL;  // Invalid input
    }

    int i = 0;
    while (string[i] != '\0') {
        if ((string[i] & 0x80) == 0) {
            // Single-byte character
            if (index == 0) {
                unsigned char *result =(unsigned char *) malloc(2);
                if (result == NULL) {
                    return NULL; // Memory allocation error
                }
                result[0] = string[i];
                result[1] = '\0';
                return result;
            }

            index--;
        }
        else if ((string[i] & 0xE0) == 0xC0) {
            // Two-byte character
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80) {
                unsigned char *result = malloc(3);
                if (result == NULL) {
                    return NULL; // Memory allocation error
                }
                result[0] = string[i];
                result[1] = string[i+1];
                result[2] = '\0';
                return result;
            }
            index--;
            i++;
        }
        else if ((string[i] & 0xF0) == 0xE0) {
            // Three-byte character
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80) {
                unsigned char *result = malloc(4);
                if (result == NULL) {
                    return NULL; // Memory allocation error
                }
                result[0] = string[i];
                result[1] = string[i+1];
                result[2] = string[i+2];
                result[3] = '\0';
                return result;
            }
            index--;
            i += 2;
        }
        else if ((string[i] & 0xF8) == 0xF0) {
            // Four-byte character
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80 && (string[i + 3] & 0xC0) == 0x80) {
                unsigned char *result = malloc(5);
                if (result == NULL) {
                    return NULL; // Memory allocation error
                }
                result[0] = string[i];
                result[1] = string[i+1];
                result[2] = string[i+2];
                result[3] = string[i+3];
                result[4] = '\0';
                return result;
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
//int my_utf8_substring_search(unsigned char *haystack, unsigned char *needle) {
//    if (my_utf8_strlen(haystack) == 0 || my_utf8_strlen(needle)== 0) {
//        return -1; // Invalid input
//    }
//
//    int haystack_len = my_utf8_strlen(haystack);
//    int needle_len = my_utf8_strlen(needle);
//
//    if (needle_len > haystack_len) {
//        return -1; // Substring is longer than the string, no match possible
//    }
//
//    printf("test: %d\n", haystack_len - needle_len);
//    for (int i = 0; i <= haystack_len - needle_len; ++i) {
//        printf("i=%d\n", i);
//        int j = 0;
//
//        while (j < needle_len) {  // Change the condition to '<' instead of '<='
//            printf("j=%d\n", j);
//
//            unsigned char *haystack_char = my_utf8_charat(haystack, i + j);
//            unsigned char *needle_char = my_utf8_charat(needle, j);
//
//            printf("haystack=%s, needle=%s\n", haystack_char, needle_char);
//
//            if (haystack_char == NULL || needle_char == NULL) {
//                break;
//            }
//
//            if (my_utf8_strcmp(haystack_char, needle_char) != 0) {
//                break;
//            }
//
//            j += my_utf8_strlen(needle_char);
//        }
//
//        if (j == needle_len) {
//            printf("\ni=%d, j=%d, needle_len=%d\n", i, j, needle_len);
//            return i; // Substring found
//        }
//
//        // Increment i here so that it moves to the next position in the haystack
////        i++;
//    }
//
//
//
//
//
//
//    return -1; // Substring not found
//}





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

// TESTING - helper functions
// manually compare two strings
int compare_strings(unsigned char *str1, unsigned char *str2){
    while (*str1 != '\0' && *str2 != '\0' && *str1 == *str2){
        str1++;
        str2++;
    }

    return (*str1 == '\0' && *str2 == '\0');
}

// TESTING
void test_utf8_encode(unsigned char *input, unsigned char *expected){
    unsigned char output[100];

    int res = my_utf8_encode(input, output);

    if (res == 0 && compare_strings(output, expected)){
        printf("PASSED: Input=\"%s\", Expected=\"%s\", Output=\"%s\"\n", input, expected, output);
    }
    else {
        printf("FAILED: Input=\"%s\", Expected=\"%s\", Output=\"%s\"\n", input, expected, output);

    }
}

void test_utf8_decode(unsigned char *input, unsigned char *expected) {
    unsigned char output[100];

    int res = my_utf8_decode(input, output);

    if (res == 0 && compare_strings(output, expected)) {
        printf("PASSED: Input=\"%s\", Expected=\"%s\", Output=\"%s\"\n", input, expected, output);
    } 
    else {
        printf("FAILED: Input=\"%s\", Expected=\"%s\", Output=\"%s\"\n", input, expected, output);
    }
}

void test_utf8_check(unsigned char *input, int expected){
    int res = my_utf8_check(input);

    if (res == expected){
        printf("PASSED: Input=\"%s\", Expected=%d, Result=%d\n", input, expected, res);
    } 
    else {
        printf("FAILED: Input=\"%s\", Expected=%d, Result=%d\n", input, expected, res);
    }
}

void test_utf8_strlen(unsigned char *input, int expected){
    int res = my_utf8_strlen(input);

    if (res == expected){
        printf("PASSED: Input=\"%s\", Expected=%d, Result=%d\n", input, expected, res);
    }
    else {
        printf("FAILED: Input=\"%s\", Expected=%d, Result=%d\n", input, expected, res);
    }
}

void test_utf8_charat(unsigned char *input, int index, unsigned char *expected){
    unsigned char *res = my_utf8_charat(input, index);

    if (res != NULL){
        printf("PASSED: Input=\"%s\", Index=%d, Expected=%s, Result=%s\n", input, index, expected, res);
    }
    else {
        if (expected == NULL){
            printf("PASSED: Input=\"%s\", Index=%d, Expected=NULL, Result=NULL\n", input, index);
        }
        else {
            printf("FAILED: Input=\"%s\", Index=%d, Expected=%s, Result=%s\n", input, index, expected, res);
        }
    }
}

void test_utf8_strcmp(unsigned char *string1, unsigned char *string2, int expected){
    int res = my_utf8_strcmp(string1, string2);

    int sign = (res > 0) - (res < 0); // determine sign of result

    if (sign == expected){
        printf("PASSED: String 1=\"%s\", String 2=\"%s\", Expected=%d, Result=%d\n", string1, string2, expected, sign);
    }
    else {
        printf("FAILED: String 1=\"%s\", String 2=\"%s\", Expected=%d, Result=%d\n", string1, string2, expected, sign);
    }
}

void test_utf8_remove_whitespace(const unsigned char *input, const unsigned char *expected) {
    unsigned char *result = my_utf8_remove_whitespace(input);

    if (result == NULL) {
        printf("FAILED: Input=\"%s\", Expected=\"%s\", Result=NULL\n", input, expected);
        return;
    }

    if (strcmp((char *)result, (char *)expected) == 0) {
        printf("PASSED: Input=\"%s\", Expected=\"%s\", Result=\"%s\"\n", input, expected, result);
    } else {
        printf("FAILED: Input=\"%s\", Expected=\"%s\", Result=\"%s\"\n", input, expected, result);
    }

    free(result);
}

//
//void test_utf8_substring_search(unsigned char *haystack, unsigned char *needle, int expected_index) {
//    int res = my_utf8_substring_search(haystack, needle);
////    printf("\n%d\n", expected_index);
//
//    if (res == expected_index) {
//        printf("PASSED: String=\"%s\", Substring=\"%s\", Expected Index=%d, Actual Index=%d\n", haystack, needle, expected_index, res);
//    }
//    else {
//        printf("FAILED: String=\"%s\", Substring=\"%s\", Expected Index=%d, Actual Index=%d\n", haystack, needle, expected_index,  res);
//    }
//}




void test_all_utf8_encode(){
    printf("Testing my_utf8_encode:\n");
    test_utf8_encode((unsigned char*)"", (unsigned char*)"");
    test_utf8_encode((unsigned char*)"A", (unsigned char*)"A");
    test_utf8_encode((unsigned char*)"Hello", (unsigned char*)"Hello");
    test_utf8_encode((unsigned char*)"\\u1846", (unsigned char*)"ᡆ");
    test_utf8_encode((unsigned char*)"\\u10D2\\u10D0\\u10DB\\u10D0\\u10E0\\u10EF\\u10DD\\u10D1\\u10D0", (unsigned char*)"გამარჯობა");
    test_utf8_encode((unsigned char*)"\\u103a8", (unsigned char*)"𐎨");
    test_utf8_encode((unsigned char*)"\\uZZZZ", (unsigned char*)"\\uZZZZ");
    test_utf8_encode((unsigned char*)"\\u1f632\\u1f634", (unsigned char*)"😲😴");
    test_utf8_encode((unsigned char*)"Hello \\u05D0\\u05E8\\u05D9\\u05D4 \\u1F601", (unsigned char*)"Hello אריה 😁");
    test_utf8_encode((unsigned char*)"אמירה", (unsigned char*)"אמירה");
    test_utf8_encode((unsigned char*)"\xf0\x90\x84\xa1", (unsigned char*)"𐄡");
    test_utf8_encode((unsigned char*)"u\\05D0", (unsigned char*)"u\\05D0");
}

void test_all_utf8_decode(){
    printf("\nTesting my_utf8_decode:\n");
    test_utf8_decode((unsigned char*)"", (unsigned char*)"");
    test_utf8_decode((unsigned char*)"A", (unsigned char*)"A");
    test_utf8_decode((unsigned char*)"Hello", (unsigned char*)"Hello");
    test_utf8_decode((unsigned char*)"Ꮂ", (unsigned char*)"\\u13B2");
    test_utf8_decode((unsigned char*)"Խոսք", (unsigned char*)"\\u053D\\u0578\\u057D\\u0584");
    test_utf8_decode((unsigned char*)"🀜", (unsigned char*)"\\u1F01C");
    test_utf8_decode((unsigned char*)"🀤🀲", (unsigned char*)"\\u1F024\\u1F032");
    test_utf8_decode((unsigned char*)"Hello אריה 😁", (unsigned char*)"Hello \\u05D0\\u05E8\\u05D9\\u05D4 \\u1F601");
    test_utf8_decode((unsigned char*)"", (unsigned char*)"");
    test_utf8_decode((unsigned char*)"\xf0\x90\xa9\xa0", (unsigned char*)"\\u10A60");
    test_utf8_decode((unsigned char*)"\\u1846", (unsigned char*)"\\u1846");
    test_utf8_decode((unsigned char*)"\\uZZZZ", (unsigned char*)"\\uZZZZ");
}

void test_all_utf8_check(){
    printf("\nTesting my_utf8_check:\n");
    test_utf8_check((unsigned char*)"", 1);
    test_utf8_check((unsigned char*)"Hello", 1);
    test_utf8_check((unsigned char*)"한국어 문장", 1);
    test_utf8_check((unsigned char*)"This is አማርኛ and ខ្មែរ", 1);
    test_utf8_check((unsigned char*)"\\u00E9\\u00A7\\u00B8", 1);
    test_utf8_check((unsigned char*)"\xd7\x90", 1);
    test_utf8_check((unsigned char*)"\\uWXYZ", 1);
    
    // Invalid UTF-8 strings
    test_utf8_check((unsigned char*)"\xC0\x80", 0); // Overlong encoding
    test_utf8_check((unsigned char*)"\xED\xA0\x80", 0); // Surrogate pair
    test_utf8_check((unsigned char*)"\xFF\xFF\xFF\xFF", 0); // Surrogate pair
}

void test_all_utf8_strlen(){
    printf("\nTesting my_utf8_strlen:\n");
    test_utf8_strlen((unsigned char*)"", 0);
    test_utf8_strlen((unsigned char*)"R", 1);
    test_utf8_strlen((unsigned char*)"Hello", 5);
    test_utf8_strlen((unsigned char*)"😞😭", 2);
    test_utf8_strlen((unsigned char*)"Hello שלום", 10);
    test_utf8_strlen((unsigned char*)"\xd8\x9f\xd9\x81", 2);
    test_utf8_strlen((unsigned char*)"\\u00E9\\u00A7\\u00B8", 18);

    // Invalid UTF-8 strings
    test_utf8_strlen((unsigned char*)"\xC0\x80", 1); // Overlong encoding
    test_utf8_strlen((unsigned char*)"\xED\xA0\x80", 1); // Surrogate pair
    test_utf8_strlen((unsigned char*)"\xF4\x90\x80\x80\x80", 1); // Surrogate pair
}


void test_all_utf8_charat(){
    printf("\nTesting my_utf8_charat:\n");
    // valid index
    test_utf8_charat((unsigned char*)"", 0, NULL);
    test_utf8_charat((unsigned char*)"L", 0,(unsigned char*)"L");
    test_utf8_charat((unsigned char*)"Hello", 1, (unsigned char*)"e");
    test_utf8_charat((unsigned char*)"ສະບາຍດີ", 2, (unsigned char*)"ບ");
    test_utf8_charat((unsigned char*)"Language Язык", 9, (unsigned char*)"Я");
    test_utf8_charat((unsigned char*)"\xe2\x86\xaa\xe2\x86\xa1\xe2\x86\xa0", 2, (unsigned char*)"↠");
    test_utf8_charat((unsigned char*)"\xC0\x80", 0, (unsigned char*)"\xC0\x80"); // overlong
    test_utf8_charat((unsigned char*)"\xED\xA0\x80", 0, (unsigned char*)"\xED\xA0\x80"); // Surrogate pair
    // invalid index:
    test_utf8_charat((unsigned char*)"Amira", 5, NULL);
    test_utf8_charat((unsigned char*)"テスト", 7, NULL);
    test_utf8_charat((unsigned char*)"This is a Sợi dây", 30, NULL);
    test_utf8_charat((unsigned char*)"\xf0\x9e\xb8\x99\xf0\x9e\xb8\xbb", 4, NULL);
    test_utf8_charat((unsigned char*)"\xC0\x80", 4, NULL); // overlong
    test_utf8_charat((unsigned char*)"\xED\xA0\x80", 46, NULL); // Surrogate pair
}

void test_all_utf8_strcmp(){
    // should return 0 if string1 == string2,
    // -1 if string1 is before string2, 1 if string1 is after string2
    printf("\nTesting my_utf8_strcmp:\n");
    test_utf8_strcmp((unsigned char*)"",(unsigned char*)"", 0);
    test_utf8_strcmp((unsigned char*)"Amira",(unsigned char*)"", 1);
    test_utf8_strcmp((unsigned char*)"",(unsigned char*)"Amira", -1);
    test_utf8_strcmp((unsigned char*)"hello", (unsigned char*)"hello", 0);
    test_utf8_strcmp((unsigned char*)"abc", (unsigned char*)"def", -1);
    test_utf8_strcmp((unsigned char*)"def",(unsigned char*)"abc", 1);
    test_utf8_strcmp((unsigned char*)"চরিত্র",(unsigned char*)"চরিত্র", 0);
    test_utf8_strcmp((unsigned char*)"ලිපිය",(unsigned char*)"จดหมาย", -1);
    test_utf8_strcmp((unsigned char*)"ఉత్తరం",(unsigned char*)"מכתב", 1);
    test_utf8_strcmp((unsigned char*)"கடிதம்",(unsigned char*)"😎", -1);
    test_utf8_strcmp((unsigned char*)"ဈ",(unsigned char*)"\xe1\x80\x88", 0);
    test_utf8_strcmp((unsigned char*)"😁 Hello",(unsigned char*)"信 yes", 1);
    test_utf8_strcmp((unsigned char*)"\xC0\x80",(unsigned char*)"a", 1); // overlong
    test_utf8_strcmp((unsigned char*)"computer",(unsigned char*)"\xE0\x90\x88", -1); // overlong
    test_utf8_strcmp((unsigned char*)"\xED\xA0\x90",(unsigned char*)"ASCII", 1); // invalid surrogate pair
}
void test_all_utf8_remove_whitespace(){
    printf("\nTesting my_utf8_remove_whitespace():\n");

    test_utf8_remove_whitespace((unsigned char*)"Hello World", (unsigned char*)"HelloWorld");
    test_utf8_remove_whitespace((unsigned char*)"   Remove   \t  Whitespace\n", (unsigned char*)"RemoveWhitespace");
    test_utf8_remove_whitespace((unsigned char*)"NoWhitespaceHere", (unsigned char*)"NoWhitespaceHere");
    test_utf8_remove_whitespace((unsigned char*)"", (unsigned char*)"");
    test_utf8_remove_whitespace((unsigned char*)"     ", (unsigned char*)"");
    test_utf8_remove_whitespace((unsigned char*)"\n\t\r", (unsigned char*)"");
    test_utf8_remove_whitespace((unsigned char*)"Привет, \t\nмир    \n\r!", (unsigned char*)"Привет,мир!");
    test_utf8_remove_whitespace((unsigned char*)"Υπάρχει χώρος", (unsigned char*)"Привет,мир!");
    test_utf8_remove_whitespace((unsigned char*)"\xcf\x87\xcf\x8e\t\xcf\x81\n\xce\xbf\xcf\x82", (unsigned char*)"χώρος");

}// \u03C7\u03CE\u03C1\u03BF\u03C2


//void test_all_utf8_substring_search(){
//    printf("\nTesting my_utf8_substring_search:\n");
//    test_utf8_substring_search((unsigned char*)"中文句子", (unsigned char*)"子", 3);
////    test_utf8_substring_search((unsigned char*)"α a Greek letter? α", (unsigned char*)"",-1);
//    test_utf8_substring_search((unsigned char*)"Testing the end", (unsigned char*)"d", 14);
////    test_utf8_substring_search((unsigned char*)"Hello World", (unsigned char*)"World", 6);
////    test_utf8_substring_search((unsigned char*)"Is α a Greek letter? α", (unsigned char*)"α",3);
////    test_utf8_substring_search((unsigned char*)"α a Greek letter? α", (unsigned char*)"α",0);
////
////
////    test_utf8_substring_search((unsigned char*)"Is α a Greek letter? α", (unsigned char*)"p",-1);
//
//
//}


int main() {
    test_all_utf8_encode();
    test_all_utf8_decode();
    test_all_utf8_check();
    test_all_utf8_strlen();
    test_all_utf8_charat();
    test_all_utf8_strcmp();
    test_all_utf8_remove_whitespace();
//    test_all_utf8_substring_search();

    return 0;
}