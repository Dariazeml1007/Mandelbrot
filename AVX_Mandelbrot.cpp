#include <cmath>
#include <cstring>
#include <ctime>
#include "txlib.h"
#include <immintrin.h>
#define SIZE_GROUP_AVX 8

const int WIDTH    = 800;
const int HEIGHT   = 600;
const int MAX_ITER = 256;
const float R_MAX  = 4.0f;

typedef struct
{
    alignas(32) float x[SIZE_GROUP_AVX];
    alignas(32) float y[SIZE_GROUP_AVX]; //demand aligned
    alignas(32) int iter[SIZE_GROUP_AVX];
} PointGroup;


struct ViewParams
{
    float xMin = -2.5f;
    float xMax = 1.5f;
    float yMin = -1.5f;
    float yMax = 1.5f;
    bool needRedraw = true;

    void reset()
    {
        xMin = -2.5f;
        xMax = 1.5f;
        yMin = -1.5f;
        yMax = 1.5f;
    }

    void zoom(float factor)
    {
        float centerX = (xMin + xMax) / 2;
        float centerY = (yMin + yMax) / 2;

        float width = (xMax - xMin) / factor;
        float height = (yMax - yMin) / factor;

        xMin = centerX - width / 2;
        xMax = centerX + width / 2;
        yMin = centerY - height / 2;
        yMax = centerY + height / 2;
    }

    void move(float dx, float dy)
    {
        float width = xMax - xMin;
        float height = yMax - yMin;

        xMin += dx * width;
        xMax += dx * width;
        yMin += dy * height;
        yMax += dy * height;
    }
};

RGBQUAD get_color(int iter);
void mandelbrot(PointGroup *pixel);
void run_time(ViewParams *viewParams);
void draw_mandelbrot(ViewParams *viewParams);
void count_mandelbrot_grouped(COLORREF* colorBuffer, ViewParams *viewParams);
void display(COLORREF* colorBuffer, ViewParams *viewParams);
void handleInput(ViewParams* viewParams);



int main(const int argc, const char* argv[])
{
    ViewParams viewParams;

    if (argc > 1 && strcmp(argv[1], "--graphics") == 0)
    {
        viewParams.needRedraw = true;
        txCreateWindow(WIDTH, HEIGHT);
        txTextCursor(false);
        while (!GetAsyncKeyState(VK_ESCAPE))
        {
            handleInput(&viewParams);

            if (viewParams.needRedraw )
            {
                draw_mandelbrot(&viewParams);
                viewParams.needRedraw  = false;
            }

            txSleep(10);
        }
    }
    else
    {
        run_time(&viewParams);
    }
    return 0;
}

void handleInput(ViewParams *viewParams)
{
    assert(viewParams);

    const float moveStep = 0.2f;
    const float zoomFactor = 1.5f;

    if (GetAsyncKeyState(VK_LEFT))
    {
        viewParams->move(-moveStep, 0);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_RIGHT))
    {
        viewParams->move(moveStep, 0);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_UP))
    {
        viewParams->move(0, -moveStep);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_DOWN)) {
        viewParams->move(0, moveStep);
        viewParams->needRedraw =  true;
    }

    if (GetAsyncKeyState(VK_ADD) )
    {
        viewParams->zoom(zoomFactor);
        viewParams->needRedraw =  true;
    }
    if (GetAsyncKeyState(VK_SUBTRACT) )
    {
        viewParams->zoom(1/zoomFactor);
        viewParams->needRedraw =  true;
    }

    if (GetAsyncKeyState(VK_SPACE))
    {
        viewParams->reset();
        viewParams->needRedraw = true;
    }

    return;
}
//O0 intr
RGBQUAD get_color(int iter)
{
    if (iter == MAX_ITER) return RGBQUAD{0, 0, 0};

    int r = (iter * 197) & 0xFF;// % 256
    int g = (iter * 237) & 0xFF;
    int b = (iter * 255) & 0xFF;

    return RGBQUAD{(BYTE)b, (BYTE)g, (BYTE)r};
}


void display(COLORREF* colorBuffer, ViewParams *viewParams)
{
    assert(colorBuffer);
    assert(viewParams);

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px++)
        {
             txSetPixel(px, py, colorBuffer[py * WIDTH + px]);
        }
    }
    char info[256];
    sprintf(info, "X: [%.5f, %.5f] Y: [%.5f, %.5f]",
            viewParams->xMin, viewParams->xMax, viewParams->yMin, viewParams->yMax);
    txSetColor(TX_WHITE);
    txTextOut(10, 10, info);
}


void draw_mandelbrot(ViewParams *viewParams)
{
    COLORREF* colorBuffer = (COLORREF*)_mm_malloc(HEIGHT * WIDTH * sizeof(COLORREF), 32);
    if (!colorBuffer) return;

    count_mandelbrot_grouped(colorBuffer, viewParams);
    display(colorBuffer, viewParams);

    _mm_free(colorBuffer);
}


void run_time(ViewParams *viewParams)
{
    COLORREF* colorBuffer = (COLORREF*)_mm_malloc(HEIGHT * WIDTH * sizeof(COLORREF), 32);
    if (!colorBuffer) return;

    clock_t start = clock();
    count_mandelbrot_grouped(colorBuffer, viewParams);
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", time);

    _mm_free(colorBuffer);
}





void mandelbrot(PointGroup* group)
{
    assert((uintptr_t)(group->x) % 32 == 0);
    assert((uintptr_t)(group->y) % 32 == 0);

    __m256 x = _mm256_setzero_ps();
    __m256 y = _mm256_setzero_ps();
    const __m256 x0 = _mm256_load_ps(group->x);
    const __m256 y0 = _mm256_load_ps(group->y);
    const __m256 escape = _mm256_set1_ps(R_MAX);

    __m256 active = _mm256_set1_ps(-1.0f);                       // All points are active (0xFFFFFFFF)
    __m256i res = _mm256_setzero_si256();                        // matrix of iters

    for (int iter = 0; iter < MAX_ITER; iter++)
    {

        __m256 x2 = _mm256_mul_ps(x, x);
        __m256 y2 = _mm256_mul_ps(y, y);


        __m256 r2 = _mm256_add_ps(x2, y2);
        __m256 mask = _mm256_cmp_ps(r2, escape, _CMP_LT_OS);        // 0xFFFFFFFF, if point still active (in Mandelbrot set); 0x00000000 - if not


        active = _mm256_and_ps(active, mask);                        // byte and to add new "non-active" points to active table


        res = _mm256_sub_epi32(res, _mm256_castps_si256(mask));      // update res


        __m256 xy = _mm256_mul_ps(x, y);
        x = _mm256_add_ps(_mm256_sub_ps(x2, y2), x0);
        y = _mm256_add_ps(_mm256_add_ps(xy, xy), y0);


        x = _mm256_mul_ps(x, active);                               // zeroing not active to not overflow float
        y = _mm256_mul_ps(y, active);


        if (_mm256_testz_ps(active, active)) break;                 // if all points - "inactive"
    }

    _mm256_store_si256((__m256i*)group->iter, res);                 // res to group->iter
}

void count_mandelbrot_grouped(COLORREF* colorBuffer, ViewParams *viewParams)
{

    const float xScale = (viewParams->xMax - viewParams->xMin) / WIDTH;
    const float yScale = (viewParams->yMax - viewParams->yMin) / HEIGHT;


    PointGroup* group = (PointGroup*)_mm_malloc(sizeof(PointGroup), 32);
    assert(group  && "Memory allocation failed");

    for (int py = 0; py < HEIGHT; py++)
    {
        const float y = viewParams->yMin + (float)py * yScale;

        for (int px = 0; px < WIDTH; px += SIZE_GROUP_AVX)
        {

            for (int i = 0; i < SIZE_GROUP_AVX; i++)
            {
                int curr_px = px + i;
                if (curr_px >= WIDTH) continue;
                group->x[i] = viewParams->xMin + (float)curr_px * xScale;
                group->y[i] = y;
            }

            mandelbrot(group);

            for (int i = 0; i < SIZE_GROUP_AVX; i++)
            {
                const int curr_px = px + i;
                if (curr_px >= WIDTH) continue;

                const RGBQUAD quad = get_color(group->iter[i]);
                colorBuffer[py * WIDTH + curr_px] = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
            }

        }
    }

    _mm_free(group);
}
