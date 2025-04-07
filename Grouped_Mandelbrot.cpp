#include <cmath>
#include <cstring>
#include <ctime>
#include "txlib.h"

#define SIZE_GROUP 8

const int WIDTH    = 800;
const int HEIGHT   = 600;
const int MAX_ITER = 256;
const float R_MAX  = 4.0f;

typedef struct
{
    float x[SIZE_GROUP];
    float y[SIZE_GROUP];
    int iter[SIZE_GROUP];
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

RGBQUAD get_color(int iter)
{
    if (iter == MAX_ITER) return RGBQUAD{0, 0, 0};

    int r = (iter * 197) & 0xFF;
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
    assert(viewParams);

    COLORREF* colorBuffer = (COLORREF*)calloc(HEIGHT * WIDTH, sizeof(COLORREF));
    if (!colorBuffer) return;

    count_mandelbrot_grouped(colorBuffer, viewParams);
    display(colorBuffer, viewParams);

    free(colorBuffer);
}

void run_time(ViewParams *viewParams)
{
    assert(viewParams);

    COLORREF* colorBuffer = (COLORREF*)calloc(HEIGHT * WIDTH, sizeof(COLORREF));
    if (!colorBuffer) return;

    clock_t start = clock();
    count_mandelbrot_grouped(colorBuffer, viewParams);
    double time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", time);

    free(colorBuffer);
}





void mandelbrot(PointGroup *pixel)
{
    float x[SIZE_GROUP] = {0}, y[SIZE_GROUP] = {0};
    bool active[SIZE_GROUP] = {true, true, true, true, true, true, true, true};
    int iter = 0;

    unsigned int active_count = 0;

    for (; iter < MAX_ITER; iter++)
    {

        for (int i = 0; i < SIZE_GROUP; i++)
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


    for (int i = 0; i < SIZE_GROUP; i++)
    {
        if (active[i]) pixel->iter[i] = MAX_ITER;
    }

    return ;
}

void count_mandelbrot_grouped(COLORREF* colorBuffer, ViewParams *viewParams)
{

    for (int py = 0; py < HEIGHT; py++)
    {
        for (int px = 0; px < WIDTH; px += SIZE_GROUP)
        {
            PointGroup pixel;


            for (int i = 0; i < SIZE_GROUP; i++)
            {
                int curr_px = px + i;
                if (curr_px >= WIDTH) curr_px = WIDTH - 1;
                float x_step = (viewParams->xMax - viewParams->xMin) / WIDTH;
                float y_step = (viewParams->yMax - viewParams->yMin) / HEIGHT;
                pixel.x[i] = viewParams->xMin +  (float)curr_px * x_step;
                pixel.y[i] = viewParams->yMin + (float)py * y_step;
            }


            mandelbrot(&pixel);


            for (int i = 0; i < SIZE_GROUP; i++)
            {
                int curr_px = px + i;
                if (curr_px >= WIDTH) continue;

                RGBQUAD quad = get_color(pixel.iter[i]);
                colorBuffer[py * WIDTH + curr_px] = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
            }
        }
    }
}
