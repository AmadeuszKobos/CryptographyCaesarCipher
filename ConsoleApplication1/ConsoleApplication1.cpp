#include <iostream>
#include <string>
#include <cctype>
#include <limits>

// Helper to shift alphabetic characters by a given offset in the alphabet.
// Non-alphabetic characters are returned unchanged.
char shiftChar(char c, int shift)
{
    if (std::isalpha(static_cast<unsigned char>(c))) {
        char base = std::islower(static_cast<unsigned char>(c)) ? 'a' : 'A';
        int offset = (c - base + shift) % 26;
        if (offset < 0)
            offset += 26;
        return static_cast<char>(base + offset);
    }
    return c;
}

// Encrypt text using Caesar shift followed by XOR with the keyword.
std::string encrypt(const std::string &text, int shift, const std::string &keyword)
{
    std::string result;
    result.reserve(text.size());
    size_t keyLen = keyword.size();
    for (size_t i = 0; i < text.size(); ++i) {
        char shifted = shiftChar(text[i], shift);
        char keyChar = keyword[i % keyLen];
        result.push_back(shifted ^ keyChar);
    }
    return result;
}

// Decrypt text that was encrypted with the above algorithm.
std::string decrypt(const std::string &cipher, int shift, const std::string &keyword)
{
    std::string result;
    result.reserve(cipher.size());
    size_t keyLen = keyword.size();
    for (size_t i = 0; i < cipher.size(); ++i) {
        char keyChar = keyword[i % keyLen];
        char xored = cipher[i] ^ keyChar;
        result.push_back(shiftChar(xored, -shift));
    }
    return result;
}

int main()
{
    std::cout << "Choose mode (e- encrypt, d- decrypt): ";
    char mode;
    std::cin >> mode;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter text: ";
    std::string text;
    std::getline(std::cin, text);

    std::cout << "Enter shift value: ";
    int shift;
    std::cin >> shift;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter keyword: ";
    std::string keyword;
    std::getline(std::cin, keyword);

    if (mode == 'e') {
        std::string encrypted = encrypt(text, shift, keyword);
        std::cout << "Encrypted text: " << encrypted << '\n';
    } else if (mode == 'd') {
        std::string decrypted = decrypt(text, shift, keyword);
        std::cout << "Decrypted text: " << decrypted << '\n';
    } else {
        std::cout << "Invalid mode" << '\n';
    }
    return 0;
}
