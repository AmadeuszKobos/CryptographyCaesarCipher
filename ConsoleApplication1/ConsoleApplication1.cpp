#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h> 

using namespace std;

// Sprawdza, czy znak jest małą literą łacińską
bool isLowerLatin(char c) {
    return (c >= 'a' && c <= 'z');
}

// Sprawdza, czy znak jest wielką literą łacińską
bool isUpperLatin(char c) {
    return (c >= 'A' && c <= 'Z');
}

// Oblicza wartość przesunięcia dla pojedynczego znaku kluczowego
int keywordShift(char k) {
    if (isLowerLatin(k))      return k - 'a';
    else if (isUpperLatin(k)) return k - 'A';
    else                      return 0; // jeśli znak nie jest literą, traktujemy jako 0
}

// Funkcja szyfrująca:
// Dla każdego znaku: sumujemy trzy składowe przesunięcia:
//   1) stały klucz (key)
//   2) indeks znaku w tekście (i)
//   3) przesunięcie wynikające z odpowiedniego znaku ze słowa kluczowego (keyword)
string encrypt(const string& plaintext, int key, const string& keyword) {
    string cipher = plaintext;
    size_t kwLen = keyword.size();
    for (size_t i = 0; i < plaintext.size(); ++i) {
        char c = plaintext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + static_cast<int>(i) + kwShift) % 26;

        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            cipher[i] = static_cast<char>(base + enc);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            cipher[i] = static_cast<char>(base + enc);
        }
        else {
            // Pozostałe znaki (spacje, cyfry, znaki diakrytyczne) zostają bez zmian
            cipher[i] = c;
        }
    }
    return cipher;
}

// Funkcja deszyfrująca:
// Dla każdego znaku: odejmujemy sumę trzech składowych przesunięcia:
//   1) stały klucz (key)
//   2) indeks znaku w tekście (i)
//   3) przesunięcie wynikające ze słowa kluczowego (keyword)
string decrypt(const string& ciphertext, int key, const string& keyword) {
    string plain = ciphertext;
    size_t kwLen = keyword.size();
    for (size_t i = 0; i < ciphertext.size(); ++i) {
        char c = ciphertext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + static_cast<int>(i) + kwShift) % 26;

        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            plain[i] = static_cast<char>(base + dec);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            plain[i] = static_cast<char>(base + dec);
        }
        else {
            plain[i] = c;
        }
    }
    return plain;
}

int main() {
    // Uzyskanie częstotliwości licznika wysokiej rozdzielczości
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {
        cerr << "QueryPerformanceFrequency nie powiodło się.\n";
        return 1;
    }

    cout << "=== Autorski szyfr: przesuniecie + slowo kluczowe (wersja sekwencyjna) ===\n\n";
    cout << "Wybierz tryb działania:\n";
    cout << "  1 - Szyfruj tekst\n";
    cout << "  2 - Deszyfruj tekst\n";
    cout << "Twoj wybor: ";
    int mode;
    cin >> mode;
    if (mode != 1 && mode != 2) {
        cerr << "Nieprawidlowy tryb. Program zakonczy dzialanie.\n";
        return 1;
    }

    cout << "Podaj klucz bazowy (liczba calkowita, np. 3): ";
    int key;
    cin >> key;
    key = key % 26;
    if (key < 0) key += 26;

    cout << "Podaj slowo kluczowe (tylko litery A-Z lub a-z): ";
    string keyword;
    cin >> keyword;
    if (keyword.empty()) {
        cerr << "Slowo kluczowe nie moze byc puste.\n";
        return 1;
    }

    cout << "Podaj nazwe pliku wejsciowego (tekst do przetworzenia): ";
    string input_filename;
    cin >> input_filename;
    if (input_filename.empty()) {
        cerr << "Nazwa pliku nie moze byc pusta.\n";
        return 1;
    }

    // Budowanie pełnych ścieżek plików
    string input_file_path = "text_files/" + input_filename + ".txt";
    string output_file_path = "text_files/" + input_filename + "_result.txt";

    ifstream infile(input_file_path, ios::in | ios::binary);
    if (!infile) {
        cerr << "Nie mozna otworzyc pliku wejsciowego: " << input_file_path << "\n";
        return 1;
    }
    string content;
    {
        ostringstream oss;
        oss << infile.rdbuf();
        content = oss.str();
    }
    infile.close();
    SetConsoleCP(1250);
    SetConsoleOutputCP(1250);
    // Pomiar czasu szyfrowania lub deszyfrowania
    LARGE_INTEGER start, end;
    string result;
    if (mode == 1) {
        QueryPerformanceCounter(&start);
        result = encrypt(content, key, keyword);
        QueryPerformanceCounter(&end);

        double elapsedMs = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
        cout << "\nCzas szyfrowania: " << elapsedMs << " ms\n";
        cout << "Tekst zostal zaszyfrowany. Zapis do: " << output_file_path << "\n";
    }
    else {
        QueryPerformanceCounter(&start);
        result = decrypt(content, key, keyword);
        QueryPerformanceCounter(&end);

        double elapsedMs = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
        cout << "\nCzas deszyfrowania: " << elapsedMs << " ms\n";
        cout << "Tekst zostal odszyfrowany. Zapis do: " << output_file_path << "\n";
    }

    ofstream outfile(output_file_path, ios::out | ios::binary);
    if (!outfile) {
        cerr << "Nie mozna otworzyc pliku wyjsciowego: " << output_file_path << "\n";
        return 1;
    }
    outfile << result;
    outfile.close();

    cout << "Gotowe.\n";
    return 0;
}
