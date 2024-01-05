#include <stdio.h>
#include <ctype.h>
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
    while (*hex) { // while not at end of string
        // dereference cur position in hex string, increment pointer to next
        // char in the string --> convert to Int
        int digit = hexCharToInt(*(hex++));
        if (digit < 0) {
            return -1; // Invalid hexadecimal string
        }
        // shift and add the digit (hex = base 16)
        result = result * 16 + digit;
    }
    return result;
}

// helper function to check if a given character is ASCII
int isASCII(unsigned const char *input) {
    unsigned char c = (unsigned char)(*input);
    return c <= 127;
}

// helper function to retrieve information about a UTF8 character at a specified
// index in a string
int getUTF8CharInfo(unsigned const char *str, int index, int *currentChar, int *bytes) {
    *currentChar = 0;
    *bytes = 0;

    // Determine the number of bytes in the UTF-8 character
    if ((str[index] & 0x80) == 0) {// Single-byte character
        // checks if MSB = 0; if true, single byte encoding
        *currentChar = str[index];
        *bytes = 1;
    }
    else if ((str[index] & 0xE0) == 0xC0) { // two-byte character
        // checks if the first three bits of the first byte = 110
        // extract the lower five bytes of the first byte (excluding 110)
        *currentChar = str[index] & 0x1F;
        *bytes = 2;
    }
    else if ((str[index] & 0xF0) == 0xE0) { // three-byte character
        // check if first four bits are 1110
        // extract lower four bits of the first byte
        *currentChar = str[index] & 0x0F;
        *bytes = 3;
    }
    else if ((str[index] & 0xF8) == 0xF0) { // four-byte character
        // check if first five bits are 11110
        // extract lower three bits of the first byte
        *currentChar = str[index] & 0x07;
        *bytes = 4;
    }
    else {
        // Invalid UTF-8 sequence
        return -1;
    }

    // Read the remaining bytes of the UTF-8 sequence
    for (int j = 1; j < *bytes; ++j) {
        // check if the next byte in the sequence is a valid
        // continuation byte
        if ((str[index + j] & 0xC0) != 0x80) {
            // Malformed UTF-8 sequence
            return -1;
        }

        // shift the existing bit by 6 bits, making room for the 6
        // bit from the current byte to be appended.
        // isolate the lower 6 bits of the current byte (excluding the
        // leading '10' bits)
        // combine the shifted codepoint and the current byte by OR
        *currentChar = (*currentChar << 6) | (str[index + j] & 0x3F);
    }

    return 0; // Success
}

// Encoding a UTF8 string, taking as input an ASCII string,
// with UTF8 characters encoded using the Codepoint numbering
// scheme notation, and returns a UTF8 encoded string.
int my_utf8_encode(unsigned char *input, unsigned char *output) {
    if (input == NULL || output == NULL) {
        return -1; // Invalid
    }

    unsigned char *encoded = output; // pointer to output buffer

    while (*input) { // loop through each character in input string
        if ((*input == '\\' && *(input + 1) == 'u')) { // if unicode sequence
            input += 2; // Move past "\u"

            // Convert hexadecimal string to integer
            unsigned char hex[6] = {0}; // 5 hex digits + null terminator
            int i;
            for (i = 0; i < 5 && isxdigit(*input); ++i) {
                hex[i] = *(input++); // extract up to 5 hex digits from input
            }

            // if no valid hex digits were encountered after the \u
            if (i==0){
                *(encoded++) = '\\';
                *(encoded++) = 'u';
                continue; // go to next iteration of the loop
            }

            //  convert hex string to codepoint
            int codePoint = hexStringToInt(hex);

            // Encode the Unicode code point into UTF-8 using bit shifts
            if (codePoint <= 0x7F) { // one byte encoding
                // no shifting needed
                *(encoded++) = (char)codePoint;
            }
            else if (codePoint <= 0x7FF) { // two byte encoding
                // get higher-order bits and OR with 0xC0 (11000000) to extract
                // leading byte
                // lower-order bits used to create a trailing byte by
                // ORing with 0x80 (10000000)
                *(encoded++) = (char)(0xC0 | ((codePoint >> 6) & 0x1F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            }
            else if (codePoint <= 0xFFFF) { // three byte encoding
                // higher-order bits --> first byte (11100000 = 0xE0)
                // middle --> 2nd byte (10000000 = 0x80)
                // lower --> third byte (10000000 = 0x80)
                *(encoded++) = (char)(0xE0 | ((codePoint >> 12) & 0x0F));
                *(encoded++) = (char)(0x80 | ((codePoint >> 6) & 0x3F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            }
            else if (codePoint <= 0x10FFFF) { // four byte encoding
                // same approach as earlier bytes
                *(encoded++) = (char)(0xF0 | ((codePoint >> 18) & 0x07));
                *(encoded++) = (char)(0x80 | ((codePoint >> 12) & 0x3F));
                *(encoded++) = (char)(0x80 | ((codePoint >> 6) & 0x3F));
                *(encoded++) = (char)(0x80 | (codePoint & 0x3F));
            }
            else {
                return -1; // Invalid Unicode code point
            }
        }
        else { // must be regular ASCII
            // copy the ASCII to the output string as is
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
    while (*input) {
        if (isASCII(input)) {
            // if character is ASCII, copy as is to output
            *(output++) = *(input++);
        }
        else { // Non-ASCII character, handle UTF-8 decoding
            int codePoint = 0;
            int  numBytes = 0;

            // Check for errors in obtaining UTF-8 character information
            int info = getUTF8CharInfo(input, 0, &codePoint, &numBytes);
            if (info == -1) { // error
                return -1;
            }

            // Calculate the number of digits in the Unicode code point
            int digits = 1;
            int  temp = codePoint;
            while (temp >>= 4){
                ++digits;
            }

            // set the width for the sprintf function
            int width = (digits < 4) ? 4 : digits;

            // format and store the unicode code point as "\uXXXX"
            sprintf(output, "\\u%.*X", width, codePoint);

            output += width + 2; // Move the output pointer to the end of the codePoint
            input +=  numBytes; // Move the input pointer to the next character
        }
    }

    *output = '\0'; // Null-terminate the output string
    return 0;       // Success
}

// Validate that the input string is a valid UTF8 encoded string
int my_utf8_check(unsigned char *string) {
    while (*string) { // loop through string
        if ((*string & 0x80) == 0) {
            // single-byte character (ASCII)
            ++string; // move to next character
        }
        // check if current character = start of two-byte UTF-8 character
        else if ((*string & 0xE0) == 0xC0) {
            // validate second byte
            if ((string[1] & 0xC0) != 0x80 || // check if byte 2 = continuation byte
                (*string & 0x1F) == 0 || // check if first byte has a valid format
                (string[1] & 0x3F) == 0) { // check if second byte has valid format
                // Incorrect UTF-8 sequence
                return 0;
            }
            // move the pointer to next character after sequence
            string += 2;
        }
        // check if current character = start of three-byte UTF-8 character
        else if ((*string & 0xF0) == 0xE0) {
            // validate second and third bytes
            if ((string[1] & 0xC0) != 0x80 || // check if byte 2 = continuation byte
                (string[2] & 0xC0) != 0x80 || // check if byte 3 = continuation byte
                (*string & 0xF) == 0 || // check if first byte has a valid format
                (string[1] & 0x3F) == 0 || // check if second byte has a valid format
                (string[2] & 0x3F) == 0) { // check if third byte has a valid format
                // Incorrect UTF-8 sequence
                return 0;
            }
            // move the pointer to next character after sequence
            string += 3;
        }
        // check if current character = start of four-byte UTF-8 character
        else if ((*string & 0xF8) == 0xF0) {
            // validate second, third, and fourth bytes
            if ((string[1] & 0xC0) != 0x80 || // check if byte 2 = continuation byte
                (string[2] & 0xC0) != 0x80 || // check if byte 3 = continuation byte
                (string[3] & 0xC0) != 0x80 || // check if byte 4 = continuation byte
                (*string & 0x7) == 0 || // check if first byte has a valid format
                (string[1] & 0x3F) == 0 || // check if second byte has a valid format
                (string[2] & 0x3F) == 0 || // check if third byte has a valid format
                (string[3] & 0x3F) == 0) { // check if fourth byte has a valid format
                // incorrect UTF-8 sequence
                return 0;
            }
            // move the pointer to next character after sequence
            string += 4;
        }
        // if code makes it to else statement, current character is invalid
        else {
            // invalid UTF-8 sequence
            return 0;
        }
    }

    // if the code makes it here, all characters are valid
    return 1;
}

// Return the number of characters in a UTF8 encoded string
int my_utf8_strlen(unsigned char *string){
    int len = 0;

    while (*string){ // loop through string
        // extract first byte of current character
        int byte = (int)*string;

        if ((byte & 0x80) == 0){ // single-byte character
            len++;
            string++;
        }
        else if ((byte & 0xE0) == 0xC0){ // two-byte character
            len++;
            string+=2;
        }
        else if ((byte & 0xF0) == 0xE0){ // three-byte character
            len++;
            string += 3;
        }
        else if ((byte & 0xF8) == 0xF0){ // four-byte character
            len++;
            string+=4;
        }
        // if code reaches here, the current character is invalid
        else {
            // move to next character
            string++;
        }
    }
    return len;
}

// Returns the UTF8 encoded character at the location specified.
// If the input string is improperly encoded, this function should
// return NULL to indicate an error.
unsigned char *my_utf8_charat(unsigned const char *string, int index) {
    if (string == NULL || index < 0) {
        return NULL;  // Invalid input (string or index)
    }

    int i = 0;  // current position in string

    while (string[i]) { // loop through string
        if ((string[i] & 0x80) == 0) { // single-byte character
            // check if index matches current position
            if (index == 0) {
                // store result in static array and return
                static unsigned char result[2];
                result[0] = string[i];
                result[1] = '\0'; // null-terminate
                return result;
            }
            // keep track of remaining characters to skip before next potential match
            // (ensures that loop won't keep looking at the same character)
            index--;
        }
        else if ((string[i] & 0xE0) == 0xC0) { // two-byte character
            // check if index matches current position and following byte is
            // valid continuation byte
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80) {
                // store result in static array and return
                static unsigned char result[3];
                result[0] = string[i];
                result[1] = string[i+1];
                result[2] = '\0';
                return result;
            }
            index--;
            i++;
        }
        else if ((string[i] & 0xF0) == 0xE0) { // three-byte character
            // check if index matches current position and following bytes are
            // valid continuation bytes
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80) {
                // store result in static array and return
                static unsigned char result[4];
                result[0] = string[i];
                result[1] = string[i+1];
                result[2] = string[i+2];
                result[3] = '\0';
                return result;
            }
            index--;
            i += 2;
        }
        else if ((string[i] & 0xF8) == 0xF0) { // four-byte character
            // check if index matches current position and following bytes are
            // valid continuation bytes
            if (index == 0 && (string[i + 1] & 0xC0) == 0x80 && (string[i + 2] & 0xC0) == 0x80 && (string[i + 3] & 0xC0) == 0x80) {
                // store result in static array and return
                static unsigned char result[5];
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
        // if code reaches here, current character is invalid
        else {
            // Invalid UTF-8 character
            return NULL;
        }
        // move to next character in the string
        i++;
    }
    // Index out of bounds or invalid UTF-8 encoding
    return NULL;
}

// Returns whether the two strings are the same (similar result set to strcmp())
int my_utf8_strcmp(unsigned char *string1, unsigned char *string2) {
    while (*string1 && *string2) { // loop through both strings
        unsigned char char1 = *string1;
        unsigned char char2 = *string2;

        if (char1 < 128 && char2 < 128) {
            // Both characters are ASCII
            if (char1 != char2) {
                return char1 - char2;  // normal ASCII comparison
            }
        }
        else {
            // At least one character is non-ASCII (UTF-8)
            while ((*string1 & 0xC0) == 0x80) {
                string1++;  // Skip UTF-8 continuation bytes
            }
            while ((*string2 & 0xC0) == 0x80) {
                string2++; // Skip UTF-8 continuation bytes
            }

            // Compare UTF-8 characters
            int diff;
            while ((*string1 & 0xC0) == 0x80 && (*string2 & 0xC0) == 0x80) {
                // compare individual bytes of UTF-8 characters
                diff = *string1++ - *string2++;
                if (diff != 0) {
                    return diff; // difference found - not the same
                }
            }

            // If byte is continuation byte, it means that one of the
            // characters is part of a multibyte UTF-8 sequence.
            // If one string is shorter, return the difference in length
            if ((*string1 & 0xC0) == 0x80 || (*string2 & 0xC0) == 0x80) {
                return (*string1 & 0xC0) - (*string2 & 0xC0);
            }

            // Compare the first non-ASCII character
            diff = *string1 - *string2;
            if (diff != 0) {
                return diff; // difference found
            }
        }

        // Move to the next character in both strings
        string1++;
        string2++;
    }

    // Check if one string is shorter than the other
    if (*string1) {
        return 1; // string1 longer
    }
    else if (*string2) {
        return -1; // string2 longer
    }

    // Strings are equal
    return 0;
}

// EXTRA FUN FUNCTIONS:
// Function to remove whitespace from a UTF-8 encoded string
unsigned char* my_utf8_remove_whitespace(unsigned const char *input) {
    if (input == NULL) {
        return NULL;
    }

    // Allocate memory for the result string
    int inputLength = 0;
    while (input[inputLength]) {
        inputLength++; // calculate length of input string
    }

    // Allocate memory for the result string, considering the possibility of
    // removing characters
    unsigned char *result = (unsigned char*)malloc((inputLength + 1) * sizeof(char));
    if (result == NULL) {
        return NULL;
    }

    // Iterate through the UTF-8 string, copying non-whitespace characters to the result
    int resultIndex = 0;
    for (int i = 0; i < inputLength; ++i) {
        int currentChar = (int)(input[i]);
        // Check if the character is a whitespace character
        if (currentChar != ' ' && currentChar != '\t' && currentChar != '\n' &&
            currentChar != '\r' && currentChar != '\v' && currentChar != '\f') {
            result[resultIndex++] = input[i];
        }
    }

    result[resultIndex] = '\0'; // null-terminate

    return result;
}

// Function to check if two strings are anagrams (1 if they are, 0 if not)
int my_utf8_anagram_checker(unsigned char *str1, unsigned char *str2) {
    if (str1 == NULL || str2 == NULL) {
        printf("Invalid input.\n");
        return 0;
    }

    // Check the lengths of the strings
    int len1 = my_utf8_strlen(str1);
    int len2 = my_utf8_strlen(str2);

    if (len1 != len2) {
        return 0; // If lengths are different, strings cannot be anagrams
    }

    // Allocate memory for character count arrays
    // Assume a maximum of 2048 characters
    int *charCount1 = (int*)calloc(2048, sizeof(int));
    int *charCount2 = (int*)calloc(2048, sizeof(int));

    // check if memory allocation fails
    if (charCount1 == NULL || charCount2 == NULL) {
        free(charCount1);
        free(charCount2);
        return 0;
    }

    // Count the occurrence of each character in the first string
    int i = 0;
    while (i < len1) {
        int currentChar;
        int bytes;

        // Check for errors in obtaining UTF-8 character information
        if (getUTF8CharInfo(str1, i, &currentChar, &bytes) == -1) {
            free(charCount1);
            free(charCount2);
            return 0;
        }

        charCount1[currentChar]++;
        i += bytes;
    }

    // Count the occurrence of each character in the second string
    i = 0;
    while (i < len2) {
        int currentChar;
        int bytes;

        // Check for errors in obtaining UTF-8 character information
        if (getUTF8CharInfo(str2, i, &currentChar, &bytes) == -1) {
            free(charCount1);
            free(charCount2);
            return 0;
        }

        charCount2[currentChar]++;
        i += bytes;
    }

    // Compare character counts
    for (int j = 0; j < 2048; ++j) {
        if (charCount1[j] != charCount2[j]) {
            free(charCount1);
            free(charCount2);
            return 0; // If counts differ, strings are not anagrams
        }
    }

    // Free the allocated memory and return 1 (true)
    free(charCount1);
    free(charCount2);
    return 1;
}

// TESTING - helper functions
// manually compare two strings
int compare_strings(unsigned char *str1, unsigned char *str2){
    while (*str1 && *str2 && *str1 == *str2){
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

void test_utf8_remove_whitespace(const unsigned char *input, unsigned char *expected) {
    unsigned char *result = my_utf8_remove_whitespace(input);

    if (result == NULL) {
        printf("FAILED: Input=\"%s\", Expected=\"%s\", Result=NULL\n", input, expected);
        return;
    }

    if (compare_strings(result, expected) == 1) {
        printf("PASSED: Input=\"%s\", Expected=\"%s\", Result=\"%s\"\n", input, expected, result);
    }
    else {
        printf("FAILED: Input=\"%s\", Expected=\"%s\", Result=\"%s\"\n", input, expected, result);
    }

    free(result);
}

void test_utf8_anagram_checker(unsigned char* str1, unsigned char* str2, int expected){
    int res = my_utf8_anagram_checker(str1, str2);

    if (res == expected){
        printf("PASSED: String 1=\"%s\", String 2=\"%s\", Expected=%d, Result=%d\n", str1, str2, expected, res);
    }
    else {
        printf("FAILED: String 1=\"%s\", String 2=\"%s\", Expected=%d, Result=%d\n", str1, str2, expected, res);
    }
}

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
void test_all_utf8_remove_whitespace() {
    printf("\nTesting my_utf8_remove_whitespace:\n");
    test_utf8_remove_whitespace((unsigned char *) "Hello World", (unsigned char *) "HelloWorld");
    test_utf8_remove_whitespace((unsigned char *) "   Remove   \t  Whitespace\n", (unsigned char *) "RemoveWhitespace");
    test_utf8_remove_whitespace((unsigned char *) "NoWhitespaceHere", (unsigned char *) "NoWhitespaceHere");
    test_utf8_remove_whitespace((unsigned char *) "", (unsigned char *) "");
    test_utf8_remove_whitespace((unsigned char *) "     ", (unsigned char *) "");
    test_utf8_remove_whitespace((unsigned char *) "\n\t\r", (unsigned char *) "");
    test_utf8_remove_whitespace((unsigned char *) "Привет, \t\nмир    \n\r!", (unsigned char *) "Привет,мир!");
    test_utf8_remove_whitespace((unsigned char *) "\xcf\x87\xcf\x8e\t\xcf\x81\n\xce\xbf\xcf\x82",
                                (unsigned char *) "χώρος");

}

void test_all_utf8_anagram_checker(){
    printf("\nTesting my_utf8_anagram_checker():\n");
    test_utf8_anagram_checker((unsigned char*)"", (unsigned char*) "", 1);
    test_utf8_anagram_checker((unsigned char*)"A", (unsigned char*) "A", 1);
    test_utf8_anagram_checker((unsigned char*)"Amira", (unsigned char*) "Amira", 1);
    test_utf8_anagram_checker((unsigned char*)"是的中國", (unsigned char*) "國中是的", 1);
    test_utf8_anagram_checker((unsigned char*)"Amira", (unsigned char*) "aimar", 0);
    test_utf8_anagram_checker((unsigned char*)"Amira", (unsigned char*) "aisAr", 0);
    test_utf8_anagram_checker((unsigned char*)"Δοκιμές", (unsigned char*) "  Δοκιμές ", 0);
}

int main() {
    test_all_utf8_encode();
    test_all_utf8_decode();
    test_all_utf8_check();
    test_all_utf8_strlen();
    test_all_utf8_charat();
    test_all_utf8_strcmp();
    test_all_utf8_remove_whitespace();
    test_all_utf8_anagram_checker();

    return 0;
}