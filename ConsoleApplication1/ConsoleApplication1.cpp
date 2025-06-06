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

    // Lista plików testowych (bez rozszerzenia .txt)
    vector<string> files = {
        "text_test_10000",
        "text_test_100000",
        "text_test_1000000"
    };

    // Lista słów kluczowych o długości ~10, ~20, ~30
    vector<string> keywords = {
        "abcdefghij",                   // 10 znaków
        "abcdefghijklmnopqrst",         // 20 znaków
        "abcdefghijklmnopqrstuvwxyzabcd" // 30 znaków
    };

    // Lista liczby wątków do przetestowania
    vector<int> threadCounts = { 1, 3, 6, 12, 18, 24, 32 };

    // Utworzenie folderów wynikowych (jeśli nie istnieją)
    _mkdir("text_files");
    _mkdir("text_files/result_csv");

    // Plik CSV z nagłówkiem
    ofstream csv("text_files/result_csv/results.csv", ios::out | ios::binary);
    csv << "file_name,file_size,keyword_len,threads,time_seq_ms,time_thread_ms,time_openmp_ms\n";

    // Przeiteruj po każdym pliku
    for (const auto& fname : files) {
        string input_path = "text_files/" + fname + ".txt";
        // Wczytaj cały plik do stringa
        ifstream infile(input_path, ios::in | ios::binary);
        if (!infile) {
            cerr << "Nie mozna otworzyc pliku wejsciowego: " << input_path << "\n";
            continue;
        }
        string content;
        {
            ostringstream oss;
            oss << infile.rdbuf();
            content = oss.str();
        }
        infile.close();
        size_t fileSize = content.size();

        // Przetestuj każde słowo kluczowe
        for (const auto& keyw : keywords) {
            int keyLen = static_cast<int>(keyw.size());

            // Dla każdej liczby wątków
            for (int tcount : threadCounts) {
                double time_seq = 0.0, time_thread = 0.0, time_openmp = 0.0;
                string out; // bufor wyjściowy

                // 1) Sekwencyjnie - tylko gdy tcount == 1
                if (tcount == 1) {
                    LARGE_INTEGER start, end;
                    QueryPerformanceCounter(&start);
                    encrypt_seq(content, out, /*key=*/3, keyw);
                    QueryPerformanceCounter(&end);
                    time_seq = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
                }

                // 2) std::thread
                {
                    LARGE_INTEGER start, end;
                    QueryPerformanceCounter(&start);
                    encrypt_threaded(content, out, /*key=*/3, keyw, static_cast<unsigned int>(tcount));
                    QueryPerformanceCounter(&end);
                    time_thread = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
                }

                // 3) OpenMP
                {
                    LARGE_INTEGER start, end;
                    QueryPerformanceCounter(&start);
                    encrypt_openmp(content, out, /*key=*/3, keyw, tcount);
                    QueryPerformanceCounter(&end);
                    time_openmp = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
                }

                // Zapisz wiersz do CSV. Jeśli tcount > 1, pole time_seq pozostaw puste.
                csv << fname << ","
                    << fileSize << ","
                    << keyLen << ","
                    << tcount << ",";
                if (tcount == 1) {
                    csv << time_seq;
                }
                csv << ",";
                csv << time_thread << ",";
                csv << time_openmp << "\n";
            }
        }
    }

    csv.close();
    cout << "Wyniki zapisane do text_files/result_csv/results.csv\n";
    return 0;
}
