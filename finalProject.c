#include <stdio.h>
#include <stdint.h>

int is_ascii(uint8_t byte){
    return byte < 0x80;
}

// function to encode a UTF-8 string with Unicode code points
// notation: "\uXXXX"
int my_utf8_encode(char *input, char *output) {
    int i = 0; // input string index
    int j = 0; // output string index

    // loop until the end of the string:
    while (input[i] != '\0') {
        if (input[i] == '\\' && input[i + 1] == 'u') {
            // Extract Unicode code point:
            unsigned int code_point;
            if (sscanf(&input[i + 2], "%4X", &code_point) != 1) {
                // Error: invalid \uXXXX notation
                return -1;
            }

            // Encode the Unicode code point in UTF-8
            if (code_point <= 0x7F) { // one byte encoding
                output[j++] = (char)code_point;
            } else if (code_point <= 0x7FF) { // two byte encoding
                // first byte: right-shift the code point by 6 - extracts upper 5
                // bits of original code point. mask the result with 0x1F
                // (00011111 in binary) to consider only the five lower bits.
                // combine with leading prefix: 110 (0xC0)
                output[j++] = (char)(0xC0 | ((code_point >> 6) & 0x1F));
                output[j++] = (char)(0x80 | (code_point & 0x3F));
            } else if (code_point <= 0xFFFF) { // three byte encoding
                output[j++] = (char)(0xE0 | ((code_point >> 12) & 0x0F));
                output[j++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
                output[j++] = (char)(0x80 | (code_point & 0x3F));
            } else if (code_point <= 0x10FFFF) { // four byte encoding
                output[j++] = (char)(0xF0 | ((code_point >> 18) & 0x07));
                output[j++] = (char)(0x80 | ((code_point >> 12) & 0x3F));
                output[j++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
                output[j++] = (char)(0x80 | (code_point & 0x3F));
            } else {
                // Error: invalid Unicode code point
                return -1;
            }

            // Move to the next character in the input (skipping
            i += 6; // Move to the character after \uXXXX
        } else {
            // Copy non-'\\uXXXX' characters without modifying
            output[j++] = input[i++];
        }
    }

    // Null-terminate the output string
    output[j] = '\0';

    return 0;
}

int my_utf8_decode(char *input, char *output) {
    if (input == NULL || output == NULL) {
        return -1; // Invalid input parameters
    }

    int outputIndex = 0;

    while (*input != '\0') {
        uint8_t currentByte = (uint8_t)(*input);

        // Determine the number of bytes in the UTF-8 character
        int numBytes = 0;

        // Decode the initial UTF-8 byte
        if ((currentByte & 0x80) == 0) {
            // Single-byte character
            numBytes = 1;
        } else if ((currentByte & 0xE0) == 0xC0) {
            // Two-byte character
            numBytes = 2;
        } else if ((currentByte & 0xF0) == 0xE0) {
            // Three-byte character
            numBytes = 3;
        } else if ((currentByte & 0xF8) == 0xF0) {
            // Four-byte character
            numBytes = 4;
        } else {
            return -1; // Invalid UTF-8 sequence
        }

        // Decode the UTF-8 character
        uint32_t codePoint = currentByte & (0xFF >> (numBytes + 1));
        for (int i = 1; i < numBytes; i++) {
            currentByte = (uint8_t)(*(++input));
            if ((currentByte & 0xC0) != 0x80) {
                return -1; // Invalid continuation byte
            }
            codePoint = (codePoint << 6) | (currentByte & 0x3F);
        }

        // Convert the code point to ASCII representation if possible
        if (is_ascii((uint8_t)codePoint)) {
            output[outputIndex++] = (char)codePoint;
        } else {
            // Use UTF-8 character representation for non-ASCII characters
            int numBytesWritten = snprintf(output + outputIndex, 7, "\\u%04X", codePoint);
            if (numBytesWritten <= 0 || numBytesWritten > 6) {
                return -1; // Error in formatting UTF-8 character representation
            }
            outputIndex += numBytesWritten;
        }

        // Move to the next byte in the input string
        input++;
    }

    // Null-terminate the output string
    output[outputIndex] = '\0';

    return 0; // Success
}


int main() {
    // ex input: "\\u05D0\\u05DE\\u05d9\\u05e8\\u05d4"
   // char input1[] = "\\u05D0\\u05DE\\u05d9\\u05e8\\u05d4";
    char output1[10];
    char input1[45];
    printf("Enter an ASCII string with UTF-8 characters: ");
    scanf("%s", input1);

    if (my_utf8_encode(input1, output1) == 0) {
        printf("Encoded UTF-8 string: %s\n", output1);
    } else {
        printf("Error: Invalid \\uXXXX notation or Unicode code point.\n");
    }

    char input2[45];
    printf("Enter a UTF8 string: ");
    scanf("%s", input2);

    char output2[100];

    int result = my_utf8_decode(input2,output2);
    if (result == 0){
        printf("Decoded string: %s", output2);
  }
    else {
        printf("Error decoding UTF-8 string.\n");
    }
    return 0;
}
