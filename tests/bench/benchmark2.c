#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE     8192
#define NUM_ITERATIONS 1000000

int c[ARRAY_SIZE] = {};

void func(int *a, int *m) {
#pragma nounroll
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (m[i] < 0) {
            a[i] = 128;
        } else {
            c[i] = 256;
        }
    }
}

int main() {
    srand(0);
    static int a[ARRAY_SIZE / 2] = {
        83, 86, 77, 15, 93, 35, 86, 92, 49, 21, 62, 27, 90, 59, 63, 26, 40, 26, 72, 36, 11, 68, 67,
        29, 82, 30, 62, 23, 67, 35, 29, 2,  22, 58, 69, 67, 93, 56, 11, 42, 29, 73, 21, 19, 84, 37,
        98, 24, 15, 70, 13, 26, 91, 80, 56, 73, 62, 70, 96, 81, 5,  25, 84, 27, 36, 5,  46, 29, 13,
        57, 24, 95, 82, 45, 14, 67, 34, 64, 43, 50, 87, 8,  76, 78, 88, 84, 3,  51, 54, 99, 32, 60,
        76, 68, 39, 12, 26, 86, 94, 39, 95, 70, 34, 78, 67, 1,  97, 2,  17, 92, 52, 56, 1,  80, 86,
        41, 65, 89, 44, 19, 40, 29, 31, 17, 97, 71, 81, 75, 9,  27, 67, 56, 97, 53, 86, 65, 6,  83,
        19, 24, 28, 71, 32, 29, 3,  19, 70, 68, 8,  15, 40, 49, 96, 23, 18, 45, 46, 51, 21, 55, 79,
        88, 64, 28, 41, 50, 93, 0,  34, 64, 24, 14, 87, 56, 43, 91, 27, 65, 59, 36, 32, 51, 37, 28,
        75, 7,  74, 21, 58, 95, 29, 37, 35, 93, 18, 28, 43, 11, 28, 29, 76, 4,  43, 63, 13, 38, 6,
        40, 4,  18, 28, 88, 69, 17, 17, 96, 24, 43, 70, 83, 90, 99, 72, 25, 44, 90, 5,  39, 54, 86,
        69, 82, 42, 64, 97, 7,  55, 4,  48, 11, 22, 28, 99, 43, 46, 68, 40, 22, 11, 10, 5,  1,  61,
        30, 78, 5,  20, 36, 44, 26, 22, 65, 8,  16, 82, 58, 24, 37, 62, 24, 0,  36, 52, 99, 79, 50,
        68, 71, 73, 31, 81, 30, 33, 94, 60, 63, 99, 81, 99, 96, 59, 73, 13, 68, 90, 95, 26, 66, 84,
        40, 90, 84, 76, 42, 36, 7,  45, 56, 79, 18, 87, 12, 48, 72, 59, 9,  36, 10, 42, 87, 6,  1,
        13, 72, 21, 55, 19, 99, 21, 4,  39, 11, 40, 67, 5,  28, 27, 50, 84, 58, 20, 24, 22, 69, 96,
        81, 30, 84, 92, 72, 72, 50, 25, 85, 22, 99, 40, 42, 98, 13, 98, 90, 24, 90, 9,  81, 19, 36,
        32, 55, 94, 4,  79, 69, 73, 76, 50, 55, 60, 42, 79, 84, 93, 5,  21, 67, 4,  13, 61, 54, 26,
        59, 44, 2,  2,  6,  84, 21, 42, 68, 28, 89, 72, 8,  58, 98, 36, 8,  53, 48, 3,  33, 33, 48,
        90, 54, 67, 46, 68, 29, 0,  46, 88, 97, 49, 90, 3,  33, 63, 97, 53, 92, 86, 25, 52, 96, 75,
        88, 57, 29, 36, 60, 14, 21, 60, 4,  28, 27, 50, 48, 56, 2,  94, 97, 99, 43, 39, 2,  28, 3,
        0,  81, 47, 38, 59, 51, 35, 34, 39, 92, 15, 27, 4,  29, 49, 64, 85, 29, 43, 35, 77, 0,  38,
        71, 49, 89, 67, 88, 92, 95, 43, 44, 29, 90, 82, 40, 41, 69, 26, 32, 61, 42, 60, 17, 23, 61,
        81, 9,  90, 25, 96, 67, 77, 34, 90, 26, 24, 57, 14, 68, 5,  58, 12, 86, 0,  46, 26, 94, 16,
        52, 78, 29, 46, 90, 47, 70, 51, 80, 31, 93, 57, 27, 12, 86, 14, 55, 12, 90, 12, 79, 10, 69,
        89, 74, 55, 41, 20, 33, 87, 88, 38, 66, 70, 84, 56, 17, 6,  60, 49, 37, 5,  59, 17, 18, 45,
        83, 73, 58, 73, 37, 89, 83, 7,  78, 57, 14, 71, 29, 0,  59, 18, 38, 25, 88, 74, 33, 57, 81,
        93, 58, 70, 99, 17, 39, 69, 63, 22, 94, 73, 47, 31, 62, 82, 90, 92, 91, 57, 15, 21, 57, 74,
        91, 47, 51, 31, 21, 37, 40, 54, 30, 98, 25, 81, 16, 16, 2,  31, 39, 96, 4,  38, 80, 18, 21,
        70, 62, 12, 79, 77, 85, 36, 4,  76, 83, 7,  59, 57, 44, 99, 11, 27, 50, 36, 60, 18, 5,  63,
        49, 44, 11, 5,  34, 91, 75, 55, 14, 89, 68, 93, 18, 5,  82, 22, 82, 17, 30, 93, 74, 26, 93,
        86, 53, 43, 74, 14, 13, 79, 77, 62, 75, 88, 19, 10, 32, 94, 17, 46, 35, 37, 91, 53, 43, 73,
        28, 25, 91, 10, 18, 17, 36, 63, 55, 90, 58, 30, 4,  71, 61, 33, 85, 89, 73, 4,  51, 5,  50,
        68, 3,  85, 6,  95, 39, 49, 20, 67, 26, 63, 77, 96, 81, 65, 60, 36, 55, 70, 18, 11, 42, 32,
        96, 79, 21, 70, 84, 72, 27, 34, 40, 83, 72, 98, 30, 63, 47, 50, 30, 73, 14, 59, 22, 47, 24,
        82, 35, 32, 4,  54, 43, 98, 86, 40, 78, 59, 62, 62, 83, 41, 48, 23, 24, 72, 22, 54, 35, 21,
        57, 65, 47, 71, 76, 69, 18, 1,  3,  53, 33, 7,  59, 28, 6,  97, 20, 84, 8,  34, 98, 91, 76,
        98, 15, 52, 71, 89, 59, 6,  10, 16, 24, 9,  39, 0,  78, 9,  53, 81, 14, 38, 89, 26, 67, 47,
        23, 87, 31, 32, 22, 81, 75, 50, 79, 90, 54, 50, 31, 13, 57, 94, 81, 81, 3,  20, 33, 82, 81,
        87, 15, 96, 25, 4,  22, 92, 51, 97, 32, 34, 81, 6,  15, 57, 8,  95, 99, 62, 97, 83, 76, 54,
        77, 9,  87, 32, 82, 21, 66, 63, 60, 82, 11, 85, 86, 85, 30, 90, 83, 14, 76, 16, 20, 92, 25,
        28, 39, 25, 90, 36, 60, 18, 43, 37, 28, 82, 21, 10, 55, 88, 25, 15, 70, 37, 53, 8,  22, 83,
        50, 57, 97, 27, 26, 69, 71, 51, 49, 10, 28, 39, 98, 88, 10, 93, 77, 90, 76, 99, 52, 31, 87,
        77, 99, 57, 66, 52, 17, 41, 35, 68, 98, 84, 95, 76, 5,  66, 28, 54, 28, 8,  93, 78, 97, 55,
        72, 74, 45, 0,  25, 97, 83, 12, 27, 82, 21, 93, 34, 39, 34, 21, 59, 85, 57, 54, 61, 62, 72,
        41, 16, 52, 50, 62, 82, 99, 17, 54, 73, 15, 6,  51, 64, 90, 63, 91, 72, 37, 37, 59, 28, 71,
        80, 87, 56, 90, 41, 70, 52, 65, 11, 69, 17, 61, 83, 51, 12, 0,  6,  38, 67, 64, 89, 32, 54,
        4,  75, 79, 41, 12, 38, 69, 36, 70, 56, 44, 60, 49, 14, 65, 14, 26, 86, 83, 39, 69, 35, 52,
        21, 93, 90, 89, 9,  31, 73, 64, 35, 48, 95, 77, 13, 33, 98, 49, 55, 55, 93, 68, 56, 60, 33,
        23, 86, 71, 58, 77, 40, 45, 81, 61, 90, 23, 50, 0,  54, 75, 64, 42, 24, 59, 19, 89, 44, 69,
        38, 51, 76, 83, 19, 33, 43, 4,  56, 81, 75, 66, 11, 67, 12, 92, 29, 2,  68, 31, 2,  74, 7,
        18, 16, 83, 77, 87, 72, 73, 57, 62, 25, 33, 97, 96, 18, 41, 53, 26, 74, 80, 93, 85, 48, 5,
        30, 29, 59, 98, 60, 62, 24, 19, 80, 41, 2,  10, 80, 26, 83, 89, 40, 8,  23, 38, 57, 93, 31,
        10, 20, 5,  90, 13, 91, 38, 70, 21, 67, 29, 71, 80, 43, 95, 99, 24, 88, 54, 86, 69, 32, 69,
        10, 73, 30, 33, 63, 87, 79, 94, 49, 99, 51, 39, 64, 42, 30, 86, 15, 49, 15, 86, 81, 11, 34,
        33, 87, 22, 87, 73, 43, 19, 42, 54, 44, 24, 39, 59, 63, 18, 53, 12, 69, 5,  4,  33, 99, 34,
        19, 15, 35, 87, 53, 69, 50, 87, 2,  37, 62, 89, 10, 5,  60, 4,  11, 57, 29, 3,  16, 92, 21,
        22, 5,  43, 79, 61, 28, 78, 47, 0,  45, 82, 87, 99, 51, 89, 86, 53, 26, 48, 94, 36, 6,  7,
        92, 17, 64, 21, 20, 80, 66, 94, 54, 23, 37, 33, 84, 17, 12, 31, 17, 9,  65, 56, 8,  69, 45,
        95, 22, 71, 95, 69, 59, 1,  76, 52, 71, 40, 25, 43, 72, 91, 89, 27, 66, 78, 12, 50, 96, 24,
        33, 13, 86, 99, 70, 94, 68, 15, 41, 42, 39, 37, 63, 98, 90, 39, 2,  13, 31, 28, 57, 4,  71,
        46, 83, 38, 25, 95, 40, 21, 72, 26, 34, 58, 25, 56, 52, 45, 72, 46, 39, 11, 83, 3,  61, 25,
        42, 16, 39, 74, 44, 96, 30, 67, 94, 13, 57, 19, 60, 50, 92, 32, 76, 79, 90, 53, 35, 95, 98,
        7,  41, 37, 70, 76, 40, 84, 1,  83, 0,  92, 9,  96, 40, 39, 63, 35, 4,  73, 6,  64, 23, 51,
        49, 51, 30, 39, 4,  65, 86, 2,  25, 79, 91, 47, 7,  84, 31, 61, 19, 31, 53, 28, 27, 94, 19,
        43, 81, 23, 68, 87, 39, 43, 38, 88, 94, 20, 80, 98, 86, 18, 52, 63, 98, 95, 10, 5,  79, 42,
        66, 98, 25, 72, 78, 53, 66, 97, 48, 47, 72, 16, 86, 12, 59, 77, 0,  53, 97, 32, 3,  35, 51,
        7,  98, 49, 54, 61, 6,  34, 3,  25, 84, 28, 97, 63, 33, 15, 60, 81, 14, 85, 97, 0,  97, 8,
        29, 49, 13, 79, 34, 16, 14, 85, 75, 65, 86, 30, 26, 92, 16, 29, 69, 52, 9,  66, 15, 95, 33,
        28, 76, 47, 13, 26, 0,  62, 86, 29, 63, 0,  8,  97, 16, 75, 82, 44, 40, 20, 26, 18, 65, 94,
        99, 34, 46, 8,  53, 62, 3,  86, 42, 32, 34, 7,  10, 34, 69, 96, 15, 84, 96, 24, 82, 65, 51,
        16, 61, 43, 37, 87, 61, 2,  81, 60, 88, 79, 20, 41, 93, 24, 28, 35, 8,  62, 42, 70, 96, 63,
        66, 11, 0,  15, 87, 34, 32, 90, 50, 93, 33, 39, 80, 94, 93, 13, 54, 82, 44, 75, 23, 38, 51,
        51, 73, 11, 65, 68, 81, 13, 83, 99, 77, 35, 14, 64, 69, 46, 7,  72, 91, 40, 11, 23, 87, 57,
        36, 93, 39, 81, 20, 14, 71, 71, 18, 96, 34, 83, 64, 67, 97, 0,  67, 74, 35, 33, 90, 57, 32,
        49, 29, 23, 90, 92, 47, 29, 49, 35, 22, 40, 68, 43, 55, 39, 66, 25, 36, 53, 60, 52, 20, 57,
        4,  87, 83, 40, 21, 26, 97, 5,  27, 78, 28, 69, 70, 27, 98, 20, 63, 21, 60, 83, 16, 67, 23,
        82, 92, 11, 35, 53, 63, 8,  62, 68, 95, 98, 60, 68, 76, 9,  73, 3,  87, 54, 73, 57, 81, 23,
        29, 96, 44, 42, 80, 12, 9,  55, 47, 54, 66, 34, 59, 81, 42, 73, 1,  38, 71, 13, 58, 99, 22,
        84, 55, 61, 90, 80, 71, 23, 3,  0,  20, 0,  42, 52, 12, 4,  7,  59, 58, 25, 94, 17, 58, 36,
        90, 60, 26, 14, 73, 37, 65, 48, 21, 20, 9,  63, 0,  32, 86, 4,  33, 58, 56, 27, 10, 68, 31,
        69, 28, 41, 46, 74, 58, 5,  10, 1,  65, 89, 67, 90, 26, 32, 38, 99, 5,  0,  62, 57, 32, 48,
        61, 17, 7,  69, 97, 69, 38, 28, 39, 18, 70, 85, 92, 80, 42, 54, 33, 7,  43, 0,  50, 21, 85,
        88, 20, 90, 40, 34, 47, 25, 83, 61, 94, 42, 30, 91, 63, 20, 20, 54, 38, 42, 92, 30, 22, 34,
        85, 8,  94, 80, 8,  44, 2,  45, 84, 22, 87, 25, 57, 35, 2,  92, 48, 96, 86, 30, 40, 49, 51,
        12, 56, 89, 54, 48, 72, 28, 82, 57, 88, 76, 37, 97, 20, 39, 94, 5,  14, 82, 82, 23, 69, 36,
        15, 17, 84, 53, 99, 24, 54, 50, 36, 10, 92, 42, 58, 64, 23, 41, 21, 11, 69, 10, 60, 90, 2,
        55, 47, 16, 89, 81, 39, 58, 17, 6,  75, 1,  11, 74, 78, 65, 77, 66, 76, 69, 61, 34, 33, 36,
        27, 6,  99, 97, 16, 60, 39, 70, 67, 86, 86, 8,  67, 77, 66, 36, 35, 93, 37, 46, 19, 67, 12,
        96, 34, 40, 17, 95, 74, 50, 31, 2,  8,  30, 51, 25, 42, 90, 47, 61, 76, 34, 69, 95, 63, 35,
        31, 51, 80, 20, 97, 0,  88, 61, 96, 74, 1,  14, 69, 28, 16, 52, 82, 25, 82, 33, 2,  77, 23,
        49, 90, 51, 35, 60, 46, 99, 47, 29, 2,  28, 49, 99, 28, 89, 13, 76, 63, 66, 90, 84, 94, 59,
        36, 76, 84, 71, 9,  38, 0,  84, 39, 90, 35, 75, 50, 81, 26, 50, 62, 28, 78, 64, 79, 58, 53,
        44, 34, 69, 11, 77, 5,  57, 36, 42, 34, 72, 65, 95, 10, 65, 32, 49, 7,  67, 76, 58, 1,  2,
        60, 63, 82, 38, 27, 14, 96, 33, 58, 30, 54, 69, 7,  59, 27, 95, 1,  13, 67, 18, 60, 29, 35,
        92, 31, 43, 12, 7,  53, 13, 62, 13, 28, 96, 51, 56, 10, 99, 41, 69, 81, 95, 90, 89, 54, 69,
        36, 8,  82, 4,  26, 95, 33, 14, 87, 64, 57, 51, 24, 10, 64, 38, 23, 93, 34, 26, 1,  45, 25,
        94, 66, 6,  89, 56, 47, 95, 26, 84, 3,  60, 40, 82, 55, 73, 48, 95, 90, 5,  46, 66, 67, 63,
        4,  90, 8,  90, 68, 61, 35, 93, 55, 1,  51, 44, 10, 51, 91, 88, 35, 47, 48, 75, 81, 56, 0,
        29, 51, 42, 34, 49, 60, 1,  64, 64, 43, 72, 55, 11, 33, 42, 56, 40, 96, 59, 36, 58, 10, 28,
        46, 97, 75, 94, 24, 56, 50, 25, 85, 53, 19, 71, 55, 80, 24, 19, 96, 67, 44, 3,  30, 29, 46,
        86, 70, 94, 45, 58, 52, 8,  86, 98, 5,  13, 44, 30, 21, 47, 7,  58, 0,  78, 29, 7,  58, 53,
        79, 7,  72, 75, 10, 2,  4,  8,  40, 26, 2,  86, 85, 6,  46, 23, 4,  51, 37, 1,  33, 10, 48,
        92, 69, 0,  71, 50, 8,  81, 56, 87, 88, 28, 62, 51, 83, 18, 59, 23, 45, 14, 61, 82, 72, 7,
        57, 29, 11, 94, 30, 44, 57, 78, 37, 26, 78, 60, 28, 86, 41, 84, 25, 82, 65, 39, 33, 48, 58,
        44, 23, 55, 10, 85, 89, 83, 44, 46, 12, 55, 93, 42, 0,  50, 20, 89, 28, 50, 49, 56, 37, 42,
        41, 14, 24, 58, 54, 9,  58, 64, 54, 81, 19, 16, 18, 60, 99, 63, 6,  63, 18, 99, 5,  70, 1,
        77, 11, 29, 28, 12, 86, 17, 55, 79, 83, 79, 89, 37, 89, 47, 53, 95, 80, 24, 11, 99, 84, 11,
        14, 43, 26, 84, 42, 32, 7,  96, 61, 18, 25, 41, 31, 63, 10, 86, 94, 94, 17, 35, 83, 58, 82,
        37, 53, 15, 13, 17, 14, 98, 80, 80, 41, 6,  64, 35, 38, 23, 31, 52, 42, 9,  45, 25, 24, 56,
        63, 71, 2,  80, 6,  85, 91, 89, 74, 44, 56, 88, 61, 70, 38, 41, 2,  79, 48, 18, 66, 38, 42,
        50, 42, 84, 59, 88, 61, 35, 96, 24, 6,  98, 56, 13, 35, 47, 54, 62, 44, 10, 2,  57, 32, 40,
        51, 86, 71, 99, 4,  89, 89, 46, 39, 84, 82, 50, 72, 43, 86, 68, 19, 92, 18, 76, 57, 53, 75,
        63, 15, 19, 25, 69, 77, 9,  9,  80, 95, 32, 31, 0,  22, 20, 46, 13, 56, 29, 64, 28, 24, 50,
        48, 44, 94, 18, 72, 4,  72, 47, 67, 39, 19, 93, 9,  48, 54, 70, 28, 50, 55, 59, 50, 29, 31,
        48, 42, 88, 77, 6,  68, 54, 8,  69, 98, 3,  87, 70, 7,  11, 69, 26, 3,  40, 71, 64, 88, 26,
        86, 16, 76, 41, 27, 78, 22, 59, 26, 65, 99, 4,  23, 67, 58, 32, 88, 8,  87, 76, 30, 94, 39,
        51, 72, 42, 92, 44, 6,  80, 22, 93, 49, 98, 86, 28, 76, 9,  87, 2,  26, 38, 58, 49, 58, 68,
        81, 46, 28, 20, 74, 58, 14, 14, 62, 39, 56, 54, 35, 15, 34, 57, 60, 35, 7,  46, 64, 83, 55,
        3,  37, 33, 42, 48, 83, 0,  68, 16, 46, 49, 37, 21, 59, 3,  87, 21, 42, 43, 75, 77, 10, 62,
        86, 70, 97, 93, 17, 61, 28, 24, 17, 66, 58, 59, 66, 93, 59, 86, 9,  57, 35, 46, 30, 95, 2,
        17, 16, 96, 13, 92, 74, 23, 6,  60, 94, 3,  6,  63, 17, 34, 39, 34, 52, 49, 93, 18, 42, 4,
        5,  4,  13, 92, 2,  44, 87, 4,  61, 56, 1,  26, 48, 27, 50, 54, 39, 96, 9,  45, 11, 26, 80,
        2,  12, 32, 52, 57, 3,  46, 61, 60, 50, 27, 52, 53, 71, 40, 57, 84, 48, 10, 11, 48, 37, 13,
        54, 77, 61, 63, 74, 24, 42, 54, 26, 54, 39, 78, 64, 42, 25, 77, 2,  27, 4,  54, 80, 75, 46,
        90, 60, 94, 0,  23, 42, 90, 88, 96, 19, 49, 12, 93, 25, 54, 48, 51, 60, 87, 82, 24, 29, 59,
        2,  83, 86, 6,  37, 67, 34, 36, 9,  46, 30, 9,  69, 25, 51, 57, 73, 70, 58, 85, 64, 83, 39,
        64, 86, 52, 3,  68, 76, 32, 27, 30, 67, 66, 37, 4,  33, 71, 92, 94, 17, 75, 55, 38, 0,  7,
        47, 73, 77, 57, 59, 93, 92, 50, 57, 78, 2,  60, 47, 31, 44, 26, 61, 11, 92, 98, 68, 77, 21,
        60, 71, 90, 87, 79, 80, 87, 86, 79, 13, 63, 36, 24, 9,  28, 74, 66, 59, 29, 27, 58, 60, 71,
        84, 73, 35, 29, 24, 3,  58, 45, 15, 82, 88, 3,  61, 68, 42, 47, 0,  55, 62, 36, 31, 71, 17,
        58, 38, 76, 87, 17, 34, 99, 40, 70, 72, 27, 51, 96, 82, 10, 94, 98, 92, 82, 53, 53, 2,  47,
        52, 54, 55, 14, 91, 86, 38, 60, 44, 28, 88, 83, 45, 22, 34, 37, 44, 7,  65, 96, 55, 47, 6,
        49, 97, 50, 83, 2,  3,  38, 50, 55, 92, 5,  21, 35, 91, 11, 95, 88, 39, 83, 23, 36, 57, 58,
        26, 2,  65, 91, 50, 20, 90, 8,  22, 88, 58, 57, 90, 61, 95, 40, 68, 40, 97, 41, 75, 41, 53,
        23, 29, 44, 58, 52, 33, 16, 10, 59, 70, 27, 50, 72, 0,  40, 80, 74, 80, 38, 31, 71, 99, 79,
        63, 19, 19, 61, 12, 46, 54, 17, 69, 83, 62, 28, 87, 95, 96, 50, 54, 66, 29, 56, 38, 29, 96,
        18, 55, 29, 8,  87, 0,  7,  18, 15, 78, 37, 28, 90, 35, 82, 8,  5,  17, 22, 85, 57, 69, 81,
        7,  23, 99, 36, 79, 89, 18, 27, 7,  73, 56, 15, 12, 8,  74, 30, 76, 4,  19, 56, 46, 55, 39,
        6,  12, 8,  28, 97, 65, 97, 30, 72, 72, 29, 61, 3,  18, 31, 83, 77, 56, 91, 44, 69, 52, 70,
        51, 28, 26, 23, 84, 72, 30, 75, 79, 94, 84, 59, 91, 49, 9,  73, 74, 81, 2,  35, 37, 72, 66,
        20, 1,  74, 11, 45, 43, 15, 67, 47, 43, 93, 22, 80, 17, 4,  7,  96, 98, 91, 8,  89, 93, 17,
        62, 67, 50, 16, 54, 87, 40, 72, 59, 41, 46, 23, 38, 42, 38, 5,  89, 82, 50, 63, 14, 67, 67,
        21, 16, 65, 65, 24, 6,  58, 41, 20, 25, 91, 88, 79, 31, 28, 51, 90, 69, 97, 13, 59, 91, 52,
        16, 32, 86, 66, 95, 0,  85, 14, 73, 1,  79, 38, 77, 37, 96, 18, 57, 73, 62, 45, 4,  93, 25,
        7,  35, 46, 5,  49, 5,  48, 53, 73, 81, 39, 39, 76, 91, 77, 91, 64, 30, 22, 55, 8,  60, 51,
        26, 69, 77, 40, 15, 81, 33, 92, 89, 69, 39, 46, 70, 96, 94, 23, 70, 75, 14, 61, 4,  5,  38,
        95, 21, 69, 69, 76, 77, 29, 80, 55, 99, 57, 96, 66, 38, 81, 58, 79, 2,  49, 25, 24, 46, 72,
        47, 16, 47, 61, 77, 51, 18, 16, 98, 40, 37, 68, 16, 66, 49, 96, 21, 48, 5,  69, 66, 96, 51,
        77, 75, 5,  26, 53, 30, 72, 25, 29, 88, 72, 43, 18, 76, 61, 86, 26, 1,  23, 94, 70, 89, 96,
        18, 62, 96, 24, 32, 63, 20, 35, 40, 47, 92, 18, 0,  22, 43, 25, 4,  83, 50, 47, 1,  78, 8,
        87, 4,  62, 62, 51, 32, 51, 47, 2,  66, 43, 26, 50, 58, 98, 37, 50, 98, 29, 69, 98, 4,  12,
        76, 8,  47, 78, 55, 49, 8,  63, 36, 64, 77, 51, 15, 9,  54, 14, 12, 20, 10, 90, 70, 68, 41,
        7,  71, 39, 89, 40, 89, 93, 4,  17, 53, 51, 47, 8,  52, 55, 23, 41, 72, 1,  92, 39, 62, 46,
        54, 26, 19, 16, 69, 89, 84, 10, 49, 55, 1,  90, 47, 42, 83, 3,  12, 88, 7,  11, 96, 11, 19,
        71, 52, 91, 24, 96, 30, 87, 43, 36, 65, 62, 52, 34, 3,  89, 96, 4,  96, 97, 94, 96, 92, 29,
        99, 56, 17, 58, 67, 65, 22, 86, 37, 74, 77, 61, 71, 8,  0,  66, 96, 66, 80, 49, 0,  35, 38,
        97, 40, 34, 46, 86, 30, 90, 16, 82, 46, 85, 92, 14, 51, 14, 52, 40, 41, 30, 53, 64, 90, 54,
        30, 86, 72, 62, 87, 72, 97, 25, 21, 89, 60, 20, 76, 42, 10, 92, 24, 9,  29, 69, 75, 80, 83,
        27, 20, 24, 9,  74, 88, 99, 80, 70, 38, 52, 32, 25, 76, 82, 51, 98, 71, 63, 70, 47, 57, 32,
        91, 34, 41, 21, 3,  16, 53, 38, 96, 26, 63, 5,  52, 3,  57, 32, 26, 95, 36, 58, 20, 12, 92,
        23, 62, 64, 38, 32, 63, 48, 65, 7,  82, 58, 28, 37, 27, 33, 75, 23, 59, 90, 28, 11, 46, 85,
        95, 72, 32, 31, 82, 5,  96, 75, 28, 58, 39, 19, 43, 54, 67, 60, 61, 1,  70, 41, 38, 97, 75,
        13, 20, 86, 56, 1,  98, 2,  38, 45, 74, 23, 29, 56, 28, 25, 31, 8,  35, 22, 79, 30, 77, 46,
        90, 90, 47, 13, 32, 37, 10, 7,  51, 83, 93, 7,  36, 43, 9,  26, 89, 35, 49, 18, 91, 77, 95,
        23, 38, 30, 97, 17, 13, 74, 16, 55, 65, 15, 68, 49, 53, 79, 56, 56, 14, 1,  15, 50, 97, 76,
        76, 38, 11, 26, 8,  2,  55, 3,  77, 93, 85, 75, 11, 98, 1,  27, 6,  18, 42, 74, 67, 47, 53,
        23, 3,  67, 77, 18, 69, 26, 94, 46, 64, 5,  24, 72, 60, 79, 27, 37, 25, 64, 64, 36, 15, 66,
        15, 21, 84, 9,  95, 52, 57, 1,  27, 60, 20, 56, 31, 90, 82, 25, 88, 46, 83, 12, 70, 43, 91,
        49, 32, 16, 66, 97, 4,  81, 63, 71, 54, 99, 81, 1,  51, 90, 2,  31, 2,  75, 87, 33, 65, 70,
        11, 5,  68, 46, 17, 91, 89, 60, 92, 73, 29, 58, 70, 33, 91, 85, 57, 97, 37, 90, 99, 88, 80,
        53, 71, 34, 28, 59, 20, 45, 81, 83, 50, 1,  29, 19, 92, 70, 80, 37, 43, 9,  47, 66, 94, 39,
        3,  3,  88, 40, 93, 87, 81, 25, 41, 52, 12, 21, 63, 32, 67, 44, 15, 69, 46, 44, 89, 90, 14,
        21, 27, 9,  30, 75, 75, 76, 66, 31, 80, 54, 71, 25, 42, 4,  3,  35, 57, 15, 56, 72, 47, 75,
        17, 14, 45, 63, 58, 86, 53, 24, 7,  81, 33, 89, 8,  61, 17, 74, 92, 49, 28, 15, 75, 22, 72,
        30, 57, 81, 45, 66, 53, 92, 41, 70, 58, 38, 85, 16, 24, 39, 92, 83, 72, 77, 72, 32, 38, 42,
        6,  82, 91, 86, 50, 18, 9,  22, 48, 18, 55, 45, 84, 8,  89, 78, 79, 47, 16, 64, 15, 93, 55,
        7,  76, 79, 85, 1,  11, 75, 95, 69, 10, 86, 56, 12, 57, 17, 86, 57, 35, 41, 3,  72, 49, 92,
        50, 80, 40, 18, 97, 55, 11, 4,  15, 40, 84, 0,  93, 47, 27, 88, 17, 37, 74, 73, 1,  83, 90,
        87, 41};
    static int m[ARRAY_SIZE] = {};

    for (int i = 0; i < ARRAY_SIZE / 2; ++i) {
        m[i] = rand() % 2 - 1;
    }
    for (int j = 0; j < NUM_ITERATIONS; j++) {
        func(a, m);
    }

    for (int i = 0; i < ARRAY_SIZE / 2; i++) {
        printf("%d %d\n", a[i], c[i]);
    }
    for (int i = ARRAY_SIZE / 2; i < ARRAY_SIZE; i++) {
        printf("%d\n", c[i]);
    }

    return 0;
}
