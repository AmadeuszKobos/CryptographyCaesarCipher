#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h>    // QueryPerformanceCounter, SetConsoleCP, SetConsoleOutputCP, _mkdir
#include <thread>
#include <vector>
#include <direct.h>     // _mkdir
#include <omp.h>        // OpenMP

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

// ---------------- Sekwencyjne wersje ----------------

// Sekwencyjna wersja encrypt
void encrypt_seq(const string& plaintext, string& out, int key, const string& keyword) {
    size_t kwLen = keyword.size();
    size_t N = plaintext.size();
    out.resize(N);
    for (size_t i = 0; i < N; ++i) {
        char c = plaintext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + static_cast<int>(i) + kwShift) % 26;
        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            out[i] = static_cast<char>(base + enc);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            out[i] = static_cast<char>(base + enc);
        }
        else {
            out[i] = c;
        }
    }
}

// Sekwencyjna wersja decrypt
void decrypt_seq(const string& ciphertext, string& out, int key, const string& keyword) {
    size_t kwLen = keyword.size();
    size_t N = ciphertext.size();
    out.resize(N);
    for (size_t i = 0; i < N; ++i) {
        char c = ciphertext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + static_cast<int>(i) + kwShift) % 26;
        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            out[i] = static_cast<char>(base + dec);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            out[i] = static_cast<char>(base + dec);
        }
        else {
            out[i] = c;
        }
    }
}

// ---------------- Wątkowe wersje z parametrem ilości wątków ----------------

// Część pracy dla pojedynczego wątku (encrypt)
void encrypt_part(const string& plaintext, string& out, int key, const string& keyword, size_t start, size_t end) {
    size_t kwLen = keyword.size();
    for (size_t i = start; i < end; ++i) {
        char c = plaintext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + static_cast<int>(i) + kwShift) % 26;
        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            out[i] = static_cast<char>(base + enc);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            out[i] = static_cast<char>(base + enc);
        }
        else {
            out[i] = c;
        }
    }
}

// Część pracy dla pojedynczego wątku (decrypt)
void decrypt_part(const string& ciphertext, string& out, int key, const string& keyword, size_t start, size_t end) {
    size_t kwLen = keyword.size();
    for (size_t i = start; i < end; ++i) {
        char c = ciphertext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + static_cast<int>(i) + kwShift) % 26;
        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            out[i] = static_cast<char>(base + dec);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            out[i] = static_cast<char>(base + dec);
        }
        else {
            out[i] = c;
        }
    }
}

// Wątkowa wersja encrypt z ustawioną liczbą wątków
void encrypt_threaded(const string& plaintext, string& out, int key, const string& keyword, unsigned int num_threads) {
    size_t N = plaintext.size();
    out.resize(N);
    if (num_threads == 0) num_threads = 1;
    vector<thread> threads;
    size_t chunk = N / num_threads;
    for (unsigned int t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end = (t == num_threads - 1) ? N : (start + chunk);
        threads.emplace_back(encrypt_part, cref(plaintext), ref(out), key, cref(keyword), start, end);
    }
    for (auto& th : threads) {
        th.join();
    }
}

// Wątkowa wersja decrypt z ustawioną liczbą wątków
void decrypt_threaded(const string& ciphertext, string& out, int key, const string& keyword, unsigned int num_threads) {
    size_t N = ciphertext.size();
    out.resize(N);
    if (num_threads == 0) num_threads = 1;
    vector<thread> threads;
    size_t chunk = N / num_threads;
    for (unsigned int t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end = (t == num_threads - 1) ? N : (start + chunk);
        threads.emplace_back(decrypt_part, cref(ciphertext), ref(out), key, cref(keyword), start, end);
    }
    for (auto& th : threads) {
        th.join();
    }
}

// ---------------- OpenMP wersje ----------------

// OpenMP wersja encrypt (ustawiana liczba wątków)
void encrypt_openmp(const string& plaintext, string& out, int key, const string& keyword, int omp_threads) {
    omp_set_num_threads(omp_threads);
    size_t kwLen = keyword.size();
    size_t N = plaintext.size();
    out.resize(N);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < static_cast<int>(N); ++i) {
        char c = plaintext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + i + kwShift) % 26;
        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            out[i] = static_cast<char>(base + enc);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int enc = (orig + totalShift) % 26;
            out[i] = static_cast<char>(base + enc);
        }
        else {
            out[i] = c;
        }
    }
}

// OpenMP wersja decrypt (ustawiana liczba wątków)
void decrypt_openmp(const string& ciphertext, string& out, int key, const string& keyword, int omp_threads) {
    omp_set_num_threads(omp_threads);
    size_t kwLen = keyword.size();
    size_t N = ciphertext.size();
    out.resize(N);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < static_cast<int>(N); ++i) {
        char c = ciphertext[i];
        int kwShift = keywordShift(keyword[i % kwLen]);
        int totalShift = (key + i + kwShift) % 26;
        if (isLowerLatin(c)) {
            int base = 'a';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            out[i] = static_cast<char>(base + dec);
        }
        else if (isUpperLatin(c)) {
            int base = 'A';
            int orig = c - base;
            int dec = (orig - totalShift + 26) % 26;
            out[i] = static_cast<char>(base + dec);
        }
        else {
            out[i] = c;
        }
    }
}

int main() {
    // Ustawienie polskiej strony kodowej w konsoli
    SetConsoleCP(1250);
    SetConsoleOutputCP(1250);

    // Uzyskanie częstotliwości licznika wysokiej rozdzielczości
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {
        cerr << "QueryPerformanceFrequency nie powiodło się.\n";
        return 1;
    }

    cout << "=== Autorski szyfr: przesuniecie + slowo kluczowe (zrównoleglenie) ===\n\n";
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

    cout << "Podaj nazwe pliku wejsciowego (tekst do przetworzenia, bez rozszerzenia): ";
    string input_filename;
    cin >> input_filename;
    if (input_filename.empty()) {
        cerr << "Nazwa pliku nie moze byc pusta.\n";
        return 1;
    }

    // Zapytanie o liczbę wątków dla wersji std::thread i OpenMP
    cout << "Podaj liczbe watkow dla wersji std::thread (np. 4): ";
    unsigned int num_threads;
    cin >> num_threads;
    if (num_threads == 0) {
        cerr << "Liczba watkow musi byc wieksza od 0.\n";
        return 1;
    }

    cout << "Podaj liczbe watkow dla wersji OpenMP (np. 4): ";
    int omp_threads;
    cin >> omp_threads;
    if (omp_threads <= 0) {
        cerr << "Liczba watkow (OpenMP) musi byc wieksza od 0.\n";
        return 1;
    }

    // Ścieżki do folderów wynikowych (tworzymy, jeśli ich nie ma)
    _mkdir("text_files/result_seq");
    _mkdir("text_files/result_thread");
    _mkdir("text_files/result_openmp");

    // Ścieżka pliku wejściowego
    string input_file_path = "text_files/" + input_filename + ".txt";

    // Wczytanie pliku wejściowego
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

    // Bufory wyjściowe
    string out_seq, out_thread, out_openmp;

    // ---------------------- SEKWENCYJNE ----------------------
    LARGE_INTEGER start, end;
    if (mode == 1) {
        QueryPerformanceCounter(&start);
        encrypt_seq(content, out_seq, key, keyword);
        QueryPerformanceCounter(&end);
    }
    else {
        QueryPerformanceCounter(&start);
        decrypt_seq(content, out_seq, key, keyword);
        QueryPerformanceCounter(&end);
    }
    double time_seq = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    cout << "\nCzas (sekwencyjnie): " << time_seq << " ms\n";

    // Zapis sekwencyjny
    string out_seq_path = "text_files/result_seq/" + input_filename + "_seq.txt";
    ofstream ofs_seq(out_seq_path, ios::out | ios::binary);
    if (!ofs_seq) {
        cerr << "Nie mozna otworzyc pliku wyjsciowego: " << out_seq_path << "\n";
        return 1;
    }
    ofs_seq << out_seq;
    ofs_seq.close();

    // ---------------------- STD::THREAD ----------------------
    if (mode == 1) {
        QueryPerformanceCounter(&start);
        encrypt_threaded(content, out_thread, key, keyword, num_threads);
        QueryPerformanceCounter(&end);
    }
    else {
        QueryPerformanceCounter(&start);
        decrypt_threaded(content, out_thread, key, keyword, num_threads);
        QueryPerformanceCounter(&end);
    }
    double time_thread = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    cout << "Czas (std::thread, wątki=" << num_threads << "): " << time_thread << " ms\n";

    // Zapis threaded
    string out_thread_path = "text_files/result_thread/" + input_filename + "_thread.txt";
    ofstream ofs_thread(out_thread_path, ios::out | ios::binary);
    if (!ofs_thread) {
        cerr << "Nie mozna otworzyc pliku wyjsciowego: " << out_thread_path << "\n";
        return 1;
    }
    ofs_thread << out_thread;
    ofs_thread.close();

    // ---------------------- OpenMP ----------------------
    if (mode == 1) {
        QueryPerformanceCounter(&start);
        encrypt_openmp(content, out_openmp, key, keyword, omp_threads);
        QueryPerformanceCounter(&end);
    }
    else {
        QueryPerformanceCounter(&start);
        decrypt_openmp(content, out_openmp, key, keyword, omp_threads);
        QueryPerformanceCounter(&end);
    }
    double time_openmp = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
    cout << "Czas (OpenMP, wątki=" << omp_threads << "): " << time_openmp << " ms\n\n";

    // Zapis OpenMP
    string out_openmp_path = "text_files/result_openmp/" + input_filename + "_openmp.txt";
    ofstream ofs_openmp(out_openmp_path, ios::out | ios::binary);
    if (!ofs_openmp) {
        cerr << "Nie mozna otworzyc pliku wyjsciowego: " << out_openmp_path << "\n";
        return 1;
    }
    ofs_openmp << out_openmp;
    ofs_openmp.close();

    cout << "Wyniki zapisane w podfolderach result_seq, result_thread, result_openmp.\n";
    cout << "Gotowe.\n";
    return 0;
}
