#include <arm_stdio.h>

#include "filter.h"
#include "pixels.h"

#define filterHeight 5
#define filterWidth 5

double filter[filterHeight][filterWidth] =
{
	{0, 0, -1, 0, 0},
	{0, 0, -1, 0, 0},
	{0, 0,  2, 0, 0},
	{0, 0,  0, 0, 0},
	{0, 0,  0, 0, 0},
};

double factor = 1.0;
double bias = 0.0;


int custom_min(int a, int b)
{
    if (a < b) {
        return a;
    }
    return b;
}

int custom_max(int a, int b)
{
    if (a < b) {
        return b;
    }
    return a;
}

void apply_filter()
{
    //load the image into the buffer
    int w = widthImage, h = heightImage;
    int result[w * h][3];

    //apply the filter
    int x, y;
    for(x = 0; x < w; x++) { \
        for(y = 0; y < h; y++)
    {
        double red = 0.0, green = 0.0, blue = 0.0;

        //multiply every value of the filter with corresponding image pixel
        int filterY, filterX;
        for(filterY = 0; filterY < filterHeight; filterY++) { \
            for(filterX = 0; filterX < filterWidth; filterX++)
        {
            int imageX = (x - filterWidth / 2 + filterX + w) % w;
            int imageY = (y - filterHeight / 2 + filterY + h) % h;
            red += pixels[imageY * w + imageX][0] * filter[filterY][filterX];
            green += pixels[imageY * w + imageX][1] * filter[filterY][filterX];
            blue += pixels[imageY * w + imageX][2] * filter[filterY][filterX];
        }}

        //truncate values smaller than zero and larger than 255
        result[y * w + x][0] = custom_min(custom_max((int)(factor * red + bias), 0), 255);
        result[y * w + x][1] = custom_min(custom_max((int)(factor * green + bias), 0), 255);
        result[y * w + x][2] = custom_min(custom_max((int)(factor * blue + bias), 0), 255);
    }}

    //print the pixels
    int z = 0;
    arm_puts("\n\nResult:\n");
    for(x = 0; x < w; x++) { 
        for(y = 0; y < h; y++) { 
            arm_puts(" p");
            for (z = 0; z < 3; z++) {
                arm_printf("%i ", (int) result[y * w + x][z]);
    }}}
}
