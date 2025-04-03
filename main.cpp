#include <cmath>
#include <cstring>
#include <ctime>
#include "txlib.h"
#include <immintrin.h>  // Для AVX инструкций
#include <cassert>
#include <cfenv>
#define GROUP 8

const int WIDTH    = 800;
const int HEIGHT   = 600;
const int MAX_ITER = 450;
const float R_MAX  = 4.0f;

typedef struct
{
    alignas(32) float x[GROUP];
    alignas(32) float y[GROUP]; //demand aligned
    alignas(32) int iter[GROUP];
} PointGroup;

void draw_mandelbrot();
void count_mandelbrot_grouped(COLORREF* colorBuffer);
void display(COLORREF* colorBuffer);
void run_time();
void mandelbrot(PointGroup *pixel);
RGBQUAD get_color(int iter);

int main(const int argc, const char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "--graphics") == 0)
        draw_mandelbrot();
    else
        run_time();
    return 0;
}

RGBQUAD get_color(int iter)
{
    if (iter == MAX_ITER) return RGBQUAD{0, 0, 0};

    int r = (iter * 197) & 0xFF;//& 0xFF
    int g = (iter * 237) & 0xFF;
    int b = (iter * 255) & 0xFF;

    return RGBQUAD{(BYTE)b, (BYTE)g, (BYTE)r};
}



void display(COLORREF* colorBuffer)
{
    assert(colorBuffer);

    txCreateWindow(WIDTH, HEIGHT);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            txSetPixel(x, y, colorBuffer[y * WIDTH + x]);
        }
    }

    txTextCursor(false);
    txSleep(10000);
}

void draw_mandelbrot()
{
    COLORREF* colorBuffer = (COLORREF*)_mm_malloc(HEIGHT * WIDTH * sizeof(COLORREF), 32);
    if (!colorBuffer) return;

    count_mandelbrot_grouped(colorBuffer);
    display(colorBuffer);

    _mm_free(colorBuffer);
}

void run_time()
{
     COLORREF* colorBuffer = (COLORREF*)_mm_malloc(HEIGHT * WIDTH * sizeof(COLORREF), 32);
    if (!colorBuffer) return;

    clock_t start = clock();
    count_mandelbrot_grouped(colorBuffer);
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", time);

    _mm_free(colorBuffer);
}

void mandelbrot(PointGroup* p)
{

    assert((uintptr_t)(p->x) % 32 == 0);
    assert((uintptr_t)(p->y) % 32 == 0);

    __m256 x = _mm256_setzero_ps();
    __m256 y = _mm256_setzero_ps();
    const __m256 x0 = _mm256_load_ps(p->x);
    const __m256 y0 = _mm256_load_ps(p->y);
    const __m256 escape = _mm256_set1_ps(4.0f);

    __m256 active = _mm256_set1_ps(-1.0f);  // Все точки активны (0xFFFFFFFF)
    __m256i res = _mm256_setzero_si256();   // Счётчик итераций

    for (int iter = 0; iter < MAX_ITER; iter++)
    {

        __m256 x2 = _mm256_mul_ps(x, x);
        __m256 y2 = _mm256_mul_ps(y, y);


        __m256 r2 = _mm256_add_ps(x2, y2);
        __m256 mask = _mm256_cmp_ps(r2, escape, _CMP_LT_OS);  // 0xFFFFFFFF, если точка ещё в множестве


        active = _mm256_and_ps(active, mask);


        res = _mm256_sub_epi32(res, _mm256_castps_si256(mask)); // sub !!!


        __m256 xy = _mm256_mul_ps(x, y);
        x = _mm256_add_ps(_mm256_sub_ps(x2, y2), x0);
        y = _mm256_add_ps(_mm256_add_ps(xy, xy), y0);


        x = _mm256_mul_ps(x, active); // zeroing not active to not overflow float
        y = _mm256_mul_ps(y, active);


        if (_mm256_testz_ps(active, active)) break;
    }

    _mm256_store_si256((__m256i*)p->iter, res);
}

void count_mandelbrot_grouped(COLORREF* colorBuffer)
{

    const float xMin = -2.5f, xMax = 1.5f;
    const float yMin = -1.5f, yMax = 1.5f;
    const float xScale = (xMax - xMin) / WIDTH;
    const float yScale = (yMax - yMin) / HEIGHT;


    PointGroup* pixel = (PointGroup*)_mm_malloc(sizeof(PointGroup), 32);
    assert(pixel  && "Memory allocation failed");

    for (int py = 0; py < HEIGHT; py++) {
        const float y = yMin + (float)py * yScale;

        for (int px = 0; px < WIDTH; px += GROUP)
        {

            for (int i = 0; i < GROUP; i++)
            {
                int curr_px = px + i;
                if (curr_px >= WIDTH) continue;
                pixel->x[i] = xMin + (float)curr_px * xScale;
                pixel->y[i] = y;
            }


            mandelbrot(pixel);


            for (int i = 0; i < GROUP; i++)
            {

                const int curr_px = px + i;
                if (curr_px >= WIDTH) continue;

                const RGBQUAD quad = get_color(pixel->iter[i]);
                colorBuffer[py * WIDTH + curr_px] = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
            }

        }
    }

    _mm_free(pixel);
}
