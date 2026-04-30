#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
typedef struct {
    int total_bits;
    int exp_bits;
    int mant_bits;
    int bias;
} FloatFormat;

FloatFormat get_format(int bits) {
    if (bits == 32) return (FloatFormat){32, 8, 23, 127};
    if (bits == 64) return (FloatFormat){64, 11, 52, 1023};
    return (FloatFormat){128, 15, 112, 16383};
}
void exact_fraction_to_machine(long long num, long long den, int bits, char* bit_string, long double* out_error) {
    FloatFormat fmt = get_format(bits);
    //  Знак
    int sign = (num < 0) ? 1 : 0;
    long long abs_num = llabs(num);
    // Обработка нуля
    if (abs_num == 0) {
        memset(bit_string, '0', bits + (bits/8));
        bit_string[bits] = '\0';
        *out_error = 0.0L;
        return;
    }
    //  Выделяем целую часть и остаток
    long long int_part = abs_num / den;
    long long rem = abs_num % den; //  хвост
    int exp = 0;
    char mantissa_bits[200] = {0}; //test1
    int m_len = 0;
    //  Нормализация (ищем самую старшую единицу)
    if (int_part > 0) {
        long long temp = int_part;
        int highest_bit = 0;
        while (temp > 1) { temp >>= 1; highest_bit++; }
        exp = highest_bit;
        // Записываем биты целой части в мантиссу
        for (int i = highest_bit - 1; i >= 0; i--) {
            if (m_len < fmt.mant_bits) {
                mantissa_bits[m_len++] = ((int_part >> i) & 1) ? '1' : '0';
            }
        }
    } else {
        // если число < 1 например, 0.49 двигаем запятую вправо
        exp = 0;
        while (rem < den) {
            rem *= 2; //test2, перед 1
            exp--;
            if (rem == 0) break;
        }
        // убираем ту самую единицу, которая уходит до запятой
        if (rem >= den) rem -= den;
    }
    // Генерируем мантиссу из остатка без потери точности
    while (m_len < fmt.mant_bits) {
        rem *= 2;
        if (rem >= den) {
            mantissa_bits[m_len++] = '1';
            rem -= den;
        } else {
            mantissa_bits[m_len++] = '0';
        }
    }

    //  РАСЧЕТ ОШИБКИ
    // rem / den - это  точный остаток, который мы не смогли вписать в биты
    // Умножаем его на в последнего сгенерированного бита, чтобы вернуть в масштаб числа
    *out_error = ((long double)rem / (long double)den) * powl(2.0L, exp - fmt.mant_bits);
// %.45Lf нужно, чтобы показать ошибку 128-бит она порядка 10^-35
    int p = 0;
    bit_string[p++] = sign ? '1' : '0';
    bit_string[p++] = ' ';
    int biased_exp = exp + fmt.bias;
    for (int i = fmt.exp_bits - 1; i >= 0; i--) {
        bit_string[p++] = ((biased_exp >> i) & 1) ? '1' : '0';
    }
    bit_string[p++] = ' ';

    for (int i = 0; i < fmt.mant_bits; i++) {
        bit_string[p++] = mantissa_bits[i];
    }
    bit_string[p] = '\0';
}
long double rand_range(long double a, long double b) {
    return a + ((long double)rand() / RAND_MAX) * (b - a);
}

int main() {
    srand((unsigned)time(NULL));
    int n, k, bits, p;
    long double a, b;
    FILE *in = fopen("input.txt", "r");
    if (!in || fscanf(in, "%d %d %d %Lf %Lf %d", &n, &k, &bits, &a, &b, &p) != 6) {
        printf("Ошибка файла настроек!\n"); return 1;
    }
    fclose(in);

    if (a > b) { long double t = a; a = b; b = t; }
    long long factor = (long long)round(pow(10.0, p));

    for (int v = 1; v <= n; v++) {
        char pt[256], pp[256];
        sprintf(pt, "Задания/variant_%d.md", v);
        sprintf(pp, "Проверка/variant_%d.md", v);

        FILE *ft = fopen(pt, "w"); FILE *fp = fopen(pp, "w");

        fprintf(ft, "## Вариант %d\n\n| № | Число |\n|--:|:---|\n", v);
        fprintf(fp, "## Ответы %d\n\n| № | Число | Машинный вид | Точность |\n|--:|:---|:---|:---|\n", v);

        for (int i = 1; i <= k; i++) {
            // Генерируем число,сразу переводим его в целые числитель и знаменатель
            long double raw_x = rand_range(a, b);
            long long num = (long long)round(raw_x * factor);
            long double exact_decimal = (long double)num / factor;
            char bit_string[300];
            long double error;
            exact_fraction_to_machine(num, factor, bits, bit_string, &error);
            fprintf(ft, "| %d | %.*Lf |\n", i, p, exact_decimal);
            // %.45Lf нужно, чтобы показать ошибку 128-бит (она порядка 10^-35)
            // Иначе printf просто округлит ее до 0.
            fprintf(fp, "| %d | %.*Lf | `%s` | %.45Lf |\n", i, p, exact_decimal, bit_string, error);
        }
        fclose(ft); fclose(fp);
        printf("Вариант %d готов!\n", v);
    }
    return 0;
}
