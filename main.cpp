#include <cmath>
#include <cstring>
#include <ctime>
#include "txlib.h"

#define GROUP 8

const int WIDTH    = 800;
const int HEIGHT   = 600;
const int MAX_ITER = 256;
const float R_MAX  = 4.0f;

typedef struct
{
    float x[GROUP];
    float y[GROUP];
    int iter[GROUP];
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

    int r = (iter * 197) & 0xFF;
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
    COLORREF* colorBuffer = (COLORREF*)calloc(HEIGHT * WIDTH, sizeof(COLORREF));
    if (!colorBuffer) return;

    count_mandelbrot_grouped(colorBuffer);
    display(colorBuffer);

    free(colorBuffer);
}

void run_time()
{
    COLORREF* colorBuffer = (COLORREF*)calloc(HEIGHT * WIDTH, sizeof(COLORREF));
    if (!colorBuffer) return;

    clock_t start = clock();
    count_mandelbrot_grouped(colorBuffer);
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", time);

    free(colorBuffer);
}


void mandelbrot(PointGroup *pixel)
{
    float x[GROUP] = {0}, y[GROUP] = {0};
    bool active[GROUP] = {true, true, true, true, true, true, true, true};
    int iter = 0;
    unsigned int active_count = 0;

    for (; iter < MAX_ITER; iter++)
    {

        for (int i = 0; i < GROUP; i++)
        {
            if (!active[i]) continue;

            float x2 = x[i] * x[i];
            float y2 = y[i] * y[i];
            float xy = x[i] * y[i];

            if (x2 + y2 >= R_MAX)
            {
                active[i] = false;
                pixel->iter[i] = iter;
                continue;
            }


            x[i] = x2 - y2 + pixel->x[i];
            y[i] = 2 * xy + pixel->y[i];
            active_count++;
        }

        if (active_count == 0) break;
    }


    for (int i = 0; i < GROUP; i++)
    {
        if (active[i]) pixel->iter[i] = MAX_ITER;
    }

    return ;
}

void count_mandelbrot_grouped(COLORREF* colorBuffer)
{
    float xMin = -2.5f, xMax = 1.5f;
    float yMin = -1.5f, yMax = 1.5f;

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px += GROUP)
        {
            PointGroup pixel;


            for (int i = 0; i < GROUP; i++)
            {
                int curr_px = px + i;
                if (curr_px >= WIDTH) curr_px = WIDTH - 1;
                float x_step = (xMax - xMin) / WIDTH;
                float y_step = (yMax - yMin) / HEIGHT;
                pixel.x[i] = xMin +  (float)curr_px * x_step;
                pixel.y[i] = yMin + (float)py * y_step;
            }


            mandelbrot(&pixel);


            for (int i = 0; i < GROUP; i++)
            {
                int curr_px = px + i;
                if (curr_px >= WIDTH) continue;

                RGBQUAD quad = get_color(pixel.iter[i]);
                colorBuffer[py * WIDTH + curr_px] = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
            }
        }
    }
}
