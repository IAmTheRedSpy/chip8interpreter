#include <map>

class MyDisplay : public Fl_Window
{
private:
    #define Width 64
    #define Height 32
    // 64x32 pixels
    // std::array<uchar, Width*Height> pixels;
    std::array<std::array<bool, Width>, Height> pixels;
    Fl_Color white, black;
public:
    MyDisplay(int w, int h, const char *l = 0) : Fl_Window(w, h, l){}
    bool setPixel(int x, int y)
    {
        bool res = pixels[y][x];
        pixels[y][x] = true;
        return res&1;
    }
    bool unSetPixel(int x, int y)
    {
        bool res = pixels[y][x];
        pixels[y][x] = false;
        return res&1;
    }
    bool flipPixel(int x, int y)
    {
        bool res = pixels[y][x];
        pixels[y][x] = !res;
        return res&1;
    }
protected:
    void draw()
    {
        int scaleX = this->w()/Width;
        int scaleY = this->h()/Height;
        // fl_draw_image_mono(pixels.data(), 0, 0, Width, Height, 1);
        for (int row = 0; row < Height; row++)
        {
            for (int p = 0; p < Width; p++)
            {
                fl_rectf(p*scaleX, row*scaleY, scaleX, scaleY, pixels[row][p] ? FL_WHITE : FL_BLACK);
            }
            
        }
    }
};