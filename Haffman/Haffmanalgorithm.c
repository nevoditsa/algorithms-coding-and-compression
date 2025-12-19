#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

// cтруктура узла дерева Хаффмана
typedef struct HuffmanNode {
    int frequency;               // частота встречаемости символа
    unsigned char symbol;        // символ
    struct HuffmanNode* left;    // левый потомок
    struct HuffmanNode* right;   // правый потомок
} HuffmanNode;

// структура для хранения кода Хаффмана
typedef struct HuffmanCode {
    unsigned char bits[256];     // массив битов
    int length;                  // длина кода
} HuffmanCode;

// таблица встречаемости символов и таблица кодов символов
int symbolFrequency[256];      
HuffmanCode codeTable[256]; 

// функция для создания нового узла дерева Хаффмана: создаёт и инициализирует новый узел в
// динамической памяти и возвращает указатель на него
HuffmanNode* createNode(unsigned char symbol, int frequency, HuffmanNode* left, HuffmanNode* right) {
    HuffmanNode* node = malloc(sizeof(HuffmanNode));
    if (!node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = left;
    node->right = right;
    return node;
}

// функция строит дерево Хаффмана, в котором: коды символов, которые встречаются чаще, короче, 
// а встречающихся реже - длиннее
HuffmanNode* HuffmanTreebuilding() {
    HuffmanNode* nodes[256];
    int nodeCount = 0;
    for (int i = 0; i < 256; i++) {
        if (symbolFrequency[i] > 0) {
            nodes[nodeCount++] = createNode((unsigned char)i, symbolFrequency[i], NULL, NULL);
        }
    }
    // проверка случаев, когда файл пустой и когда в файле только один уникальный символ
    if (nodeCount == 0) return NULL; 
    if (nodeCount == 1) {
        HuffmanNode* notreal = createNode(0, 0, NULL, NULL);
        return createNode(0, nodes[0]->frequency, nodes[0], notreal);
    }
    // объединение двух узлов с наименьшей частотой встречаемости
    while (nodeCount > 1) {
        int min1 = -1, min2 = -1;
        for (int i = 0; i < nodeCount; i++) {
            if (min1 == -1 || nodes[i]->frequency < nodes[min1]->frequency)
                min1 = i;
        }
        for (int i = 0; i < nodeCount; i++) {
            if (i == min1) continue;
            if (min2 == -1 || nodes[i]->frequency < nodes[min2]->frequency)
                min2 = i;
        }
        // создаем родителя и заменяем им два узла
        HuffmanNode* parent = createNode(
            0,
            nodes[min1]->frequency + nodes[min2]->frequency,
            nodes[min1],
            nodes[min2]
        );
        nodes[min1] = parent;
        nodes[min2] = nodes[--nodeCount];
    }
    return nodes[0];
}

// заполнение таблицы кодов рекурсивно: path — массив для записи пути, len — текущая длина пути
void СodeTablebuilding(HuffmanNode* root, unsigned char* path, int len) {
    if (!root->left && !root->right) {
        // когда один уникальный символ в файле
        if (len == 0) {
            path[0] = 0;
            len = 1;
        }
        codeTable[root->symbol].length = len;
        memcpy(codeTable[root->symbol].bits, path, len);
        return;
    }
    // проходит по левой ветке (добавляет 0)
    if (root->left) {
        path[len] = 0;
        СodeTablebuilding(root->left, path, len + 1);
    }
    // проходит по правой ветке (добавляет 1)
    if (root->right) {
        path[len] = 1;
        СodeTablebuilding(root->right, path, len + 1);
    }
}

// функция для освобождения памяти 
void freeHuffmanTree(HuffmanNode* root) {
    if (!root) return;
    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    free(root);
}

// функция для записи размера исходного файла и таблицы встрчаемости в результирующий
void writeFrequencyTable(FILE* out, int originalSize) {
    fwrite(&originalSize, sizeof(int), 1, out);             
    fwrite(symbolFrequency, sizeof(int), 256, out);    
}

// функция для восстановления дерева по таблице: сначала очищает массив частот от лишних данных,
// потом записывает размер исходного файла и таблицу встречаемости
// в конце строит дерево Хаффмана по полученным данным
HuffmanNode* readFrequencyTable(FILE* in, int* originalSize) {
    memset(symbolFrequency, 0, sizeof(symbolFrequency));
    fread(originalSize, sizeof(int), 1, in);
    fread(symbolFrequency, sizeof(int), 256, in);
    return HuffmanTreebuilding();
}

// функция для кодирования файла
void encode(int originalSize) {
    FILE* input = fopen("text.txt", "rb");
    FILE* output = fopen("encoded.txt", "wb");
    if (!input || !output) {
        printf("Ошибка открытия файлов\n");
        if (input) fclose(input);
        if (output) fclose(output);
        exit(EXIT_FAILURE);
    }
    writeFrequencyTable(output, originalSize);
    unsigned char byteBuffer = 0;  
    int bitCount = 0;
    int ch;
    // кодирование символ за символом
    while ((ch = fgetc(input)) != EOF) {
        HuffmanCode code = codeTable[ch];
        for (int i = 0; i < code.length; i++) {
            byteBuffer = (byteBuffer << 1) | code.bits[i];
            bitCount++;
            if (bitCount == 8) {
                fputc(byteBuffer, output);
                byteBuffer = 0;
                bitCount = 0;
            }
        }
    }
    if (bitCount > 0) {
        byteBuffer <<= (8 - bitCount); 
        fputc(byteBuffer, output);
    }
    fclose(input);
    fclose(output);
}

// функция для декодирования файла
void decode() {
    FILE* input = fopen("encoded.txt", "rb");
    FILE* output = fopen("decoded.txt", "wb");
    if (!input || !output) {
        printf("Ошибка открытия файла\n");
        if (input) fclose(input);
        if (output) fclose(output);
        return;
    }
    // восстановление дерева и размера исходного файла из encoded.txt
    int originalSize;
    HuffmanNode* root = readFrequencyTable(input, &originalSize);
    if (!root) {
        printf("Пустой файл\n");
        fclose(input);
        fclose(output);
        return;
    }
    HuffmanNode* current = root;
    int writtenBytes = 0;
    int byte;
    // побитовое чтение сжатого файла
    while (writtenBytes < originalSize && (byte = fgetc(input)) != EOF) {
        for (int i = 7; i >= 0 && writtenBytes < originalSize; i--) {
            int bit = (byte >> i) & 1;
            if (!current) {
                break;
            }
            if (bit) {
                current = current->right;
            }
            else {
                current = current->left;
            }
            if (current && current->left == NULL && current->right == NULL) {
                fputc(current->symbol, output);
                writtenBytes++;
                current = root;
            }
        }
    }
    freeHuffmanTree(root);
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

void main() {
    setlocale(LC_ALL, "Russian");
    memset(symbolFrequency, 0, sizeof(symbolFrequency));
    memset(codeTable, 0, sizeof(codeTable));
    int mode;
    printf("Введите режим: \n");
    printf("1 - Кодирование\n2 - Декодирование\n");
    scanf_s("%d", &mode);
    if (mode == 1) {
        FILE* input = fopen("text.txt", "rb");
        if (!input) {
            printf("Ошибка открытия файла\n");
            return 1;
        }
        int ch;
        long fileSize = 0;
        while ((ch = fgetc(input)) != EOF) {
            symbolFrequency[ch]++;
            fileSize++;
        }
        fclose(input);
        HuffmanNode* root = HuffmanTreebuilding();
        if (!root) {
            printf("Пустой входной файл\n");
            return 1;
        }
        unsigned char path[256];
        СodeTablebuilding(root, path, 0);
        encode(fileSize);
        freeHuffmanTree(root);
        printf("Кодирование завершено\n");
    }
    else if (mode == 2) {
        decode();
        printf("Декодирование завершено\n");
        if (compareFiles("text.txt", "decoded.txt"))
            printf("Файлы совпадают\n");
        else
            printf("Файлы различаются\n");
    }
    else {
        printf("Режим неправильный\n");
    }

}
