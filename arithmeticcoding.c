#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

// структура для диапазона символа
typedef struct SymbolRange {
    int frequency;          // частота встречаемости символа
    unsigned char symbol;   // символ
    int low;                // нижняя граница
    int high;               // верхняя граница
} SymbolRange;

// структура для битового потока
typedef struct BitStream {
    int bits_to_follow;     // счетчик отложенных битов
    unsigned char buffer;   // буфер для накопления битов
    int bits_in_buffer;     // количество битов в буфере
    int byte_curr;          // текущий читаемый байт
    int bit_pos;            // позиция текущего бита в байте
} BitStream;

// таблица встречаемости символов
int symbolFrequency[256];

// функция для подсчета частот встречаемости символов в файле
void countFrequencies(char* fname, int* fsize) {
    memset(symbolFrequency, 0, sizeof(symbolFrequency));
    FILE* file = fopen(fname, "rb");
    if (!file) {
        printf("Ошибка открытия файла");
        exit(1);
    }
    *fsize = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        symbolFrequency[ch]++;
        (*fsize)++;
    }
    fclose(file);
}

// функция для инициализации битового потока
void BitStreaminitialization(BitStream* bs) {
    bs->bits_to_follow = 0;    
    bs->buffer = 0;            
    bs->bits_in_buffer = 0;    
    bs->byte_curr = 0;      
    bs->bit_pos = 0;    
}

// функция для записи бита в битовый поток
void BitStreamwriting(BitStream* bs, int bit, FILE* output) {
    bs->buffer = (bs->buffer << 1) | bit;
    bs->bits_in_buffer++;
    if (bs->bits_in_buffer == 8) {
        fputc(bs->buffer, output);    
        bs->buffer = 0;             
        bs->bits_in_buffer = 0;   
    }
    while (bs->bits_to_follow > 0) {
        bs->buffer = (bs->buffer << 1) | (!bit);
        bs->bits_in_buffer++;
        if (bs->bits_in_buffer == 8) {
            fputc(bs->buffer, output);
            bs->buffer = 0;
            bs->bits_in_buffer = 0;
        }
        bs->bits_to_follow--;
    }
}

// функция для записи оставшихся битов в битовый поток
void BitStreamending(BitStream* bs, FILE* output) {
    if (bs->bits_in_buffer > 0) {
        bs->buffer <<= (8 - bs->bits_in_buffer);
        fputc(bs->buffer, output);
        bs->buffer = 0;
        bs->bits_in_buffer = 0;
    }
}

// функция для чтения бита из битового потока
int readBit(BitStream* bs, FILE* input) {
    if (bs->bit_pos == 0) {
        bs->byte_curr = fgetc(input);
        if (bs->byte_curr == EOF) bs->byte_curr = 0;
        bs->bit_pos = 8;
    }
    int bit = (bs->byte_curr >> (bs->bit_pos - 1)) & 1;
    bs->bit_pos--;
    return bit;
}

// функция для записи таблицы частот в файл
void writeFrequencyTable(FILE* output, int fsize) {
    fwrite(&fsize, sizeof(int), 1, output);
    fwrite(symbolFrequency, sizeof(int), 256, output);
}

// функция для чтения таблицы частот из файла
void readFrequencyTable(FILE* input, int* fsize) {
    fread(fsize, sizeof(int), 1, input);
    fread(symbolFrequency, sizeof(int), 256, input);
}

// функция для кодирования файла
void encode(int fsize) {
    FILE* input = fopen("text.txt", "rb");
    FILE* output = fopen("encoded.txt", "wb");  
    if (!input || !output) {
        printf("Ошибка открытия файлов\n");
        if (input) {
            fclose(input);
        }
        if (output) {
            fclose(output);
        }
        exit(1);
    }
    writeFrequencyTable(output, fsize);
    int freq[256 + 1] = { 0 };
    int total_symbols = 0;
    for (int i = 0; i < 256; i++) {
        freq[i + 1] = freq[i] + symbolFrequency[i];
        total_symbols += symbolFrequency[i];
    }
    if (total_symbols == 0) {
        printf("Файл пустой\n");
        fclose(input);
        fclose(output);
        return;
    }
    int low = 0; int high = 65535;
    int first_qtr = (high + 1) / 4;    
    int half = first_qtr * 2;         
    int third_qtr = first_qtr * 3;    
    BitStream bstream;
    BitStreaminitialization(&bstream);
    int ch;
    while ((ch = fgetc(input)) != EOF) {
        int symbol_low = freq[ch];
        int symbol_high = freq[ch + 1];
        int count = (high - low) + 1;
        int new_low = low + ((count * symbol_low) / total_symbols);
        int new_high = low + ((count * symbol_high) / total_symbols) - 1;
        low = new_low;
        high = new_high;
        // нормализация
        while (1) {
            if (high < half) {
                BitStreamwriting(&bstream, 0, output);
            }
            else if (low >= half) {
                BitStreamwriting(&bstream, 1, output);
                low -= half;
                high -= half;
            }
            else if (low >= first_qtr && high < third_qtr) {
                bstream.bits_to_follow++;
                low -= first_qtr;
                high -= first_qtr;
            }
            else {
                break;
            }
            low <<= 1;
            high = (high << 1) | 1;
        }
    }
    // завершение кодирования
    bstream.bits_to_follow++;
    if (low < first_qtr) {
        BitStreamwriting(&bstream, 0, output);
    }
    else {
        BitStreamwriting(&bstream, 1, output);
    }
    BitStreamending(&bstream, output);
    fclose(input);
    fclose(output);
}

// функция для декодирования файла
void decode() {
    FILE* input = fopen("encoded.txt", "rb"); 
    FILE* output = fopen("decoded.txt", "wb");
    if (!input || !output) {
        printf("Ошибка открытия файлов\n");
        if (input) {
            fclose(input);
        }
        if (output) {
            fclose(output);
        }
        return;
    }
    int fsize;
    readFrequencyTable(input, &fsize);
    int freq[257] = { 0 };
    int total = 0;
    for (int i = 0; i < 256; i++) {
        freq[i + 1] = freq[i] + symbolFrequency[i];
        total += symbolFrequency[i];
    }
    if (total == 0) {
        printf("Сжатый файл пустой\n");
        fclose(input);
        fclose(output);
        return;
    }
    int low = 0; int high = 65535; 
    int first_qtr = (high + 1) / 4;
    int half = first_qtr * 2;
    int third_qtr = first_qtr * 3;
    BitStream bstream;
    BitStreaminitialization(&bstream);
    int num = 0;
    for (int i = 0; i < 16; i++) {
        num = (num << 1) | readBit(&bstream, input);
    }
    // поиск символа по значению
    for (int i = 0; i < fsize; i++) {
        int range = (high - low) + 1;
        int range_size = ((num - low + 1) * total - 1) / range;
        int symbol = 0;
        for (symbol = 0; symbol < 256; symbol++) {
            if (range_size < freq[symbol + 1]) {
                break;
            }
        }
        // записываем декодированный символ
        fputc(symbol, output);
        int symbol_low = freq[symbol];
        int symbol_high = freq[symbol + 1];
        int new_low = low + ((range * symbol_low) / total);
        int new_high = low + ((range * symbol_high) / total) - 1;
        low = new_low;
        high = new_high;
        while (1) {
            if (high < half) {}
            else if (low >= half) {
                num -= half;
                low -= half;
                high -= half;
            }
            else if (low >= first_qtr && high < third_qtr) {
                num -= first_qtr;
                low -= first_qtr;
                high -= first_qtr;
            }
            else {
                break;
            }
            low <<= 1;
            high = (high << 1) | 1;
            num = (num << 1) | readBit(&bstream, input);
        }
    }
    fclose(input);
    fclose(output);
}

// функция для сравнения файлов: нужна для проверки на то, что исходных файл и декодированный одинаковы
int compareFiles(const char* fileA, const char* fileB) {
    FILE* f1 = fopen(fileA, "rb");
    FILE* f2 = fopen(fileB, "rb");
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        printf("Ошибка открытия файлов\n");
        return 0;
    }
    int a, b;
    while (1) {
        a = fgetc(f1);
        b = fgetc(f2);
        if (a == EOF && b == EOF) break;
        if (a != b) {
            fclose(f1);
            fclose(f2);
            return 0;
        }
    }
    fclose(f1);
    fclose(f2);
    return 1;
}

// функция для вычисления коэффициента сжатия
double compression() {
    FILE* f1 = fopen("text.txt", "rb");
    FILE* f2 = fopen("encoded.txt", "rb"); 
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        printf("Ошибка открытия файлов\n");
        return;
    }
    fseek(f1, 0, SEEK_END);
    fseek(f2, 0, SEEK_END);
    int size_of_text = ftell(f1);
    int size_of_decoded = ftell(f2);
    fclose(f1);
    fclose(f2);
    double compression_ratio = (double)size_of_decoded / size_of_text;
    return compression_ratio;
}

void main() {
    setlocale(LC_ALL, "Russian");
    memset(symbolFrequency, 0, sizeof(symbolFrequency));
    int mode;
    printf("Выберите режим:\n");
    printf("1 - Кодирование\n2 - Декодирование\n");
    scanf_s("%d", &mode);
    clock_t start_time, end_time;
    double result_time;
    if (mode == 1) {
        int fileSize = 0;
        countFrequencies("text.txt", &fileSize);
        if (fileSize == 0) {
            printf("Пустой входной файл\n");
        }
        start_time = clock();
        encode(fileSize);
        end_time = clock();
        result_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Кодирование завершено\n");
        printf("Время кодирования: %.3f сек\n", result_time);
        printf("Коэффициент сжатия: %.3f\n", compression());
    }
    else if (mode == 2) {
        start_time = clock();
        decode();
        end_time = clock();
        result_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Декодирование завершено\n");
        printf("Время декодирования: %.3f сек\n", result_time);
        if (compareFiles("text.txt", "decoded.txt")) {
            printf("Файлы совпадают\n");
        }
        else {
            printf("Файлы различаются\n");
        }
    }
    else {
        printf("Режим неправильный\n");
    }
}