#include <vector>
#include <stack>
#include <array>
#include <random>
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <cassert>
#include <thread>
#include <chrono>

// Requires FLTK 1.3
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Image_Surface.H>

#include "MyDisplay.cpp"

// #define DEBUG 1

#ifdef _WIN32
#include <Windows.h>
void beep() {
    Beep(700, 200);
}
#elif __linux__
void beep() {
    system("(speaker-test -t sine -f 700)& pid=$!; sleep 0.2s; kill -9 $pid");
}
#else
void beep() {
    std::cout << "(Beep: unknown operating system)" << std::endl;
}
#endif

using namespace std::chrono_literals;

auto clockDuration = 1s/60.0;

void decrementTimer(void* t) {
    uint8_t* timer = static_cast<uint8_t*>(t);
    (*timer) = std::min(*timer, (uint8_t)((*timer)-1));
    Fl::repeat_timeout(1.0/60.0, decrementTimer, t);
}

int main(int argc, char* argv[])
{
    std::vector<uint8_t> memory(0x1000);

    std::string filename;
    if (argc > 1)
    {
        filename = argv[1];
    }
    else
    {
        filename = "tombstontipp.ch8";
    }
    
    std::cout << "Use the left side of the keyboard to control the game (1 through 4, q through r, a through f, and z through v)" << std::endl;

    // Window setup
    Fl::visual(FL_RGB);
    MyDisplay window(640, 320);
    window.color(FL_WHITE);
    window.resizable(window);
    window.end();
    window.show();    

    // Key state setup
    std::array<char, 16> keymap = {'x','1','2','3','q','w','e','a','s','d','z','c','4','r','f','v'};
    std::array<bool, 16> keysPressed = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // Store Font Data
    std::array<uint8_t, 0x10> fonts = {0, 5, 10, 15, 20, 25, 30, 45, 50, 55, 60, 65, 70, 75};
    {
        int tmparray[80] = {
            0xf0, 0x90, 0x90, 0x90, 0xf0, //0
            0x20, 0x60, 0x20, 0x20, 0x70,
            0xf0, 0x10, 0xf0, 0x80, 0xf0,
            0xf0, 0x10, 0xf0, 0x10, 0xf0,
            0x90, 0x90, 0xf0, 0x10, 0x10,
            0xf0, 0x80, 0xf0, 0x10, 0xf0,
            0xf0, 0x80, 0xf0, 0x90, 0xf0,
            0xf0, 0x10, 0x20, 0x40, 0x40,
            0xf0, 0x90, 0xf0, 0x90, 0xf0,
            0xf0, 0x90, 0xf0, 0x10, 0xf0,
            0xf0, 0x90, 0xf0, 0x90, 0x90,
            0xe0, 0x90, 0xe0, 0x90, 0xe0,
            0xf0, 0x80, 0x80, 0x80, 0xf0,
            0xe0, 0x90, 0x90, 0x90, 0xe0,
            0xf0, 0x80, 0xf0, 0x80, 0xf0,
            0xf0, 0x80, 0xf0, 0x80, 0x80  //F
        };
        // std::copy(tmparray, tmparray+80, memory);
        memory.insert(memory.begin(), tmparray, tmparray+80);
    }
    

    // Load program into memory
    {
        std::ifstream executable;
        unsigned char tmp;
        bool isLowerByte = false;
        executable.open(filename, std::ios::binary|std::ios::in);
        size_t loc = 0x200;
        while(executable >> std::noskipws >> tmp)
        {
            memory[loc] = (uint8_t)tmp;
            // std::cout << "read `" << std::hex << (unsigned int)tmp << "` // ";
            loc++;
        }
    }

    // print instructions
    // for (auto &&i : memory) {
    //     std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)i << " ";
    // }
    // std::cout << std::endl;
    // std::cout << memory.size() << std::endl;

    //Interpreter
    uint16_t inst = 0x0000;
    uint32_t addrptr = 0x200;
    uint16_t memptr = 0;
    uint16_t cons;
    std::array<uint8_t, 16> regs {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    std::stack<uint16_t> stack;
    uint8_t vx, vy, key, delay, sound, scratch;
    bool beep = false;
    // RNG
    std::random_device r;
    std::mt19937 randGen(r());
    std::uniform_int_distribution<uint8_t> randomInt(0, 0xff);

    bool done = false;
    #ifdef DEBUG
    std::ostringstream s;
    s.setf(s.hex, s.basefield);
    #endif
    int i = 0;

    // Setup timers
    std::chrono::steady_clock::time_point lastTimerCheck = std::chrono::steady_clock::now();
    Fl::add_timeout(1.0/60.0, decrementTimer, &delay);
    Fl::add_timeout(1.0/60.0, decrementTimer, &sound);
    while (!done && addrptr < memory.size())
    {
        beep = (sound == 1);
        // Window was closed
        if (Fl::check() == 0)
        {
            break;
        }
        // Make beep if timer reached 0
        if (beep && (sound == 0))
        {
            //beep
        }
        
        
        inst = ((uint16_t)memory[addrptr] << 8) | ((uint16_t)memory[addrptr+1]);
        #ifdef DEBUG
        s.str("");
        std::cout << std::setfill('0') << std::setw(4) << std::hex << addrptr << ":" << std::setw(4) << inst << " ";
        #endif

        switch (inst & 0xf000)
        {
        case 0x0000:
            switch (inst)
            {
            // 0NNN machine language subroutine is not supported
            // Clear the screen
            case 0x00e0:
                #ifdef DEBUG
                s << "clear";
                #endif
                for (int y = 0; y < 64; y++)
                {
                    for (int x = 0; x < 128; x++)
                    {
                        window.unSetPixel(x,y);
                    }
                }
                window.redraw();
                break;

            // Return
            case 0x00ee:
                if (stack.empty())
                {
                    done = true;
                }
                else
                {
                    addrptr = stack.top();
                    stack.pop();
                }
                #ifdef DEBUG
                s << "return to 0x" << addrptr;
                #endif
                break;
            
            // // Switch to 64x32
            // case 0x00fe:
            //     // window.setSize(64, 32);
            //     // width = 64;
            //     // height = 32;
            //     #ifdef DEBUG
            //     s << "high-res mode off";
            //     #endif
            //     break;

            // // NOT IMPLEMENTED Switch to 128x64
            // case 0x00ff:
            //     // window.setSize(128, 64);
            //     // width = 128;
            //     // height = 64;
            //     #ifdef DEBUG
            //     s << "NOT IMPLEMENTED high-res mode on";
            //     #endif
            //     break;

            // Undefined
            default:
                #ifdef DEBUG
                s << "undefined instruction";
                #endif
                break;
            }
            break;
        
        // Jump
        case 0x1000:
            cons = inst & 0x0fff;
            addrptr = cons;
            #ifdef DEBUG
            s << "jump to 0x" << cons;
            std::cout << s.str() << std::endl;
            #endif
            continue;
            break;

        // Call
        case 0x2000:
            stack.push(addrptr);
            cons = inst & 0x0fff;
            addrptr = cons;
            #ifdef DEBUG
            s << "call to 0x" << cons;
            std::cout << s.str() << std::endl;
            #endif
            continue;
            break;

        // Skip if unary eq
        case 0x3000:
            vx = (inst & 0x0f00) >> 8;
            cons = inst & 0x00ff;
            #ifdef DEBUG
            s << "if r" << (unsigned int)vx << "(0x" << (unsigned int)regs[vx]
                << ") == 0x" << cons << " then skip";
            #endif
            if (regs[vx] == cons)
            {
                addrptr += 2;
            }
            break;
        
        // Skip if unary neq
        case 0x4000:
            vx = (inst & 0x0f00) >> 8;
            cons = inst & 0x00ff;
            #ifdef DEBUG
            s << "if r" << (unsigned int)vx << "(0x" << (unsigned int)regs[vx]
                << ") != 0x" << cons << " then skip";
            #endif
            if (regs[vx] != cons)
            {
                addrptr += 2;
            }
            break;

        // Skip if binary eq
        case 0x5000:
            if ((inst & 0x000f) == 0)
            {
                vx = (inst & 0x0f00) >> 8;
                vy = (inst & 0x00f0) >> 4;
                #ifdef DEBUG
                s << "if r" << (unsigned int)vx << "(0x" << (unsigned int)regs[vx] << ") == r"
                    << (unsigned int)vy << "(0x" << (unsigned int)regs[vy] << ") then skip";
                #endif
                if (regs[vx] == regs[vy])
                {
                    addrptr += 2;
                }
            }
            // Undefined
            else
            {
                #ifdef DEBUG
                s << "undefined instruction";
                #endif
            }
            break;
            
        
        // Unary Assignment
        case 0x6000:
            vx = (inst & 0x0f00) >> 8;
            cons = inst & 0x00ff;
            regs[vx] = cons;
            #ifdef DEBUG
            s << "assign r" << (unsigned int)vx << " the value 0x" << cons;
            #endif
            break;
        
        // Increment (don't change carry flag)
        case 0x7000:
            vx = (inst & 0x0f00) >> 8;
            cons = inst & 0x00ff;
            #ifdef DEBUG
            s << "increment r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") by ";
            #endif
            regs[vx] += cons;
            #ifdef DEBUG
            s << cons << " = " << (unsigned int)regs[vx];
            #endif
            break;
        
        // Varies
        case 0x8000:
            vx = (inst & 0x0f00) >> 8;
            vy = (inst & 0x00f0) >> 4;
            // operation varies by 0x000f
            switch (inst & 0x000f)
            {
            // Binary assignment
            case 0:
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") = r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ")";
                #endif
                regs[vx] = regs[vy];
                break;

            // Binary or
            case 1:
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") |= r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ")";
                #endif
                regs[vx] |= regs[vy];
                break;
            
            // Binary and
            case 2:
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") &= r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ")";
                #endif
                regs[vx] &= regs[vy];
                break;
            
            // Binary xor
            case 3:
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") ^= r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ")";
                #endif
                regs[vx] ^= regs[vy];
                break;
            
            // Binary increment
            case 4:
                // x+y > 255 => carry happened
                if ((uint8_t)(regs[vx] + regs[vy]) < regs[vx] || (uint8_t)(regs[vx] + regs[vy]) < regs[vy])
                {
                    scratch = 1;
                }
                else
                {
                    scratch = 0;
                }
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") += r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ")";
                s << " carry: rf=" << (unsigned int)scratch; 
                #endif
                regs[vx] += regs[vy];
                regs[0xf] = scratch;
                break;
            
            // Binary decrement
            case 5:
                // result would be negative => borrow happened
                if (regs[vx] < regs[vy])
                {
                    scratch = 0;
                }
                else
                {
                    scratch = 1;
                }
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") -= r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ")";
                s << " borrow: rf=" << (unsigned int)scratch; 
                #endif
                regs[vx] -= regs[vy];
                regs[0xf] = scratch;
                break;
            
            // Binary shift right
            case 6:
                scratch = regs[vy] & 0x01;
                regs[vx] = regs[vy] >> 1;
                regs[0xf] = scratch;
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") = r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ") >> 1";
                #endif
                break;
            
            // Binary sub&store
            case 7:
                // result would be negative => borrow happened
                if (regs[vy] < regs[vx])
                {
                    scratch = 0;
                }
                else
                {
                    scratch = 1;
                }
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << " = r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ") - r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ")";
                s << " borrow: rf=" << (unsigned int)scratch; 
                #endif
                regs[vx] = regs[vy] - regs[vx];
                regs[0xf] = scratch;
                break;
            
            // Binary shift left
            case 0xe:
                scratch = regs[vy] >> 7;
                regs[vx] = regs[vy] << 1;
                regs[0xf] = scratch;
                #ifdef DEBUG
                s << "r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ") = r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ") << 1";
                #endif
                break;
            
            // Undefined
            default:
                #ifdef DEBUG
                s << "undefined instruction";
                #endif
                break;
            }
            #ifdef DEBUG
            s << " (r" << (unsigned int)vx << " = " << (unsigned int) regs[vx] << ")";
            #endif
            break;

        // Skip if binary neq
        case 0x9000:
            if ((inst & 0x000f) == 0)
            {
                vx = (inst & 0x0f00) >> 8;
                vy = (inst & 0x00f0) >> 4;
                #ifdef DEBUG
                s << "if r" << (unsigned int)vx << "(0x" << (unsigned int)regs[vx] << ") != r"
                    << (unsigned int)vy << "(0x" << (unsigned int)regs[vy] << ") then skip";
                #endif
                if (regs[vx] != regs[vy])
                {
                    addrptr += 2;
                }
            }
            else
            {
                #ifdef DEBUG
                s << "undefined instruction";
                #endif
            }
            break;

        // Set memptr
        case 0xa000:
            memptr = inst & 0x0fff;
            #ifdef DEBUG
            s << "set memptr to 0x" << memptr;
            #endif
            break;
        
        // Jump offset
        case 0xb000:
            cons = inst & 0x0fff;
            addrptr = (cons + regs[0]) & 0x0fff; // simulate 12-bit overflow
            #ifdef DEBUG
            s << "jump to 0x" << cons << " + " << (unsigned int)regs[0];
            s << " = 0x" << addrptr;
            std::cout << s.str() << std::endl;
            #endif
            continue;
            break;
        
        // Random & mask
        case 0xc000:
            vx = (inst & 0x0f00) >> 8;
            scratch = randomInt(randGen);
            cons = inst & 0x00ff;
            regs[vx] = scratch & cons;
            #ifdef DEBUG
            s << "r" << (unsigned int)vx << " = random(" << (unsigned int)scratch << ") & mask(" << cons << ")";
            #endif
            break;
        
        // Draw sprite, if flipped from set to unset then vf=1 (carry flag)
        case 0xd000:
            vx = (inst & 0x0f00) >> 8;
            vy = (inst & 0x00f0) >> 4;
            cons = inst & 0x000f;
            regs[0xf] = 0;
            for (uint8_t i = 0; i < cons; i++)
            {
                scratch = memory[memptr+i];
                for (int j = 7; j >= 0; j--)
                {
                    // set to values in memory at memptr (bitstring)
                    if (scratch & 1)
                    {
                        regs[0xf] |= window.flipPixel((regs[vx]+j) % 64, (regs[vy]+i) % 32);
                    }
                    scratch = scratch >> 1;
                    
                }
                
            }
            window.redraw();
            
            #ifdef DEBUG
            s << "draw 8x" << cons << " sprite at r" << (unsigned int)vx << "(" << (unsigned int)regs[vx];
            s << "),r" << (unsigned int)vy << "(" << (unsigned int)regs[vy] << ") I=";
            s << memptr << " - collision:" << (unsigned int)regs[0xf];
            #endif
            break;
        
        // Is key pressed or not
        case 0xe000:
            vx = (inst & 0x0f00) >> 8;
            switch (inst & 0x00ff)
            {
            // Skip if key vx pressed
            case 0x9e:
                #ifdef DEBUG
                s << "if key r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ")  is held then skip";
                #endif
                if (Fl::event_key(keymap[regs[vx]]))
                {
                    addrptr += 2;
                }
                break;

            // Skip if key vx not pressed
            case 0xa1:
                #ifdef DEBUG
                s << "if key r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ")=" << keymap[regs[vx]] << " is not held then skip";
                #endif
                if (!Fl::event_key(keymap[regs[vx]]))
                {
                    addrptr += 2;
                }
                break;

            // Undefined
            default:
                #ifdef DEBUG
                s << "undefined instruction";
                #endif
                break;
            }
            break;
        
        // Varies
        case 0xf000:
            switch (inst & 0x00ff)
            {
            // Set vx to the value of the delay timer
            case 0x07:
                vx = (inst & 0x0f00) >> 8;
                regs[vx] = delay;
                #ifdef DEBUG
                s << "Set r" << (unsigned int)vx << " to delay(" << (unsigned int)delay << ")";
                #endif

                // Wait if busywait is detected
                if (delay != 0 && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastTimerCheck) <= clockDuration)
                {
                    // Busywait detected if last checked delay within one clock cycle
                    // Wait one clock cycle (16 miliseconds)
                    #ifdef DEBUG
                    s << " (waiting for " << clockDuration.count() << ")";
                    #endif
                    std::this_thread::sleep_for(clockDuration);
                }
                lastTimerCheck = std::chrono::steady_clock::now();
                break;

            // Wait for keypress and store in vx
            case 0x0a:
                vx = (inst & 0x0f00) >> 8;
                #ifdef DEBUG
                s << "wait for keypress/";
                std::cout << s.str();
                s.str("");
                #endif
                // Check which keys are held down
                for (uint8_t i = 0; i < 16; i++)
                {
                    keysPressed[i] = Fl::event_key(keymap[i]);
                }
                key = 255;
                while (key == 255)
                {
                    // Wait for new input
                    if (Fl::wait() == 0)
                    {
                        done = true;
                        break;
                    }

                    // Check if key changed from not pressed to pressed
                    for (uint8_t i = 0; i < 16; i++)
                    {
                        // Key was newly pressed, finish early
                        if (!keysPressed[i] && Fl::event_key(keymap[i]))
                        {
                            key = i;
                            break;
                        }
                        // Update key and continue
                        keysPressed[i] = Fl::event_key(keymap[i]);
                    }
                }
                #ifdef DEBUG
                s << "key " << (unsigned int) key << " was pressed, store in r" << (unsigned int)vx;
                #endif
                regs[vx] = key;
                break;

            // Set delay timer to the value of vx
            case 0x15:
                vx = (inst & 0x0f00) >> 8;
                delay = regs[vx];
                #ifdef DEBUG
                s << "Set delay to r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ")";
                #endif
                break;

            // Set sound timer to the value of vx
            case 0x18:
                vx = (inst & 0x0f00) >> 8;
                sound = regs[vx];
                #ifdef DEBUG
                s << "Set sound to r" << (unsigned int)vx << "(" << (unsigned int)regs[vx] << ")";
                #endif
                break;

            // Add vx to I
            case 0x1e:
                vx = (inst & 0x0f00) >> 8;
                #ifdef DEBUG
                s << "Increment I(" << memptr << ") by r" << (unsigned int)vx;
                s << "(" << (unsigned int)regs[vx] << ") = " << memptr+regs[vx];
                #endif
                memptr += regs[vx];
                break;
            
            // Set I to the location of the hex character stored in vx
            case 0x29:
                vx = (inst & 0x0f00) >> 8;
                memptr = fonts[regs[vx]];
                // memptr = regs[vx]*5;
                #ifdef DEBUG
                s << "Set memptr to char r" << (unsigned int)vx << "(" << (unsigned int)vx << ")";
                #endif
                break;
            
            // // NOT IMPLEMENTED (large font) Set I to the location of the hex character stored in vx
            // case 0x30:
            //     vx = (inst & 0x0f00) >> 8;
            //     memptr = fonts[regs[vx]];
            //     // memptr = regs[vx]*5;
            //     #ifdef DEBUG
            //     s << "Set memptr to large char r" << (unsigned int)vx << "(" << (unsigned int)vx << ")";
            //     #endif
            //     break;

            // Store binary-coded decimal equivalent at I, I+1, I+2
            case 0x33:
                vx = (inst & 0x0f00) >> 8;
                memory[memptr] = regs[vx]/100;
                memory[memptr+1] = (regs[vx]%100)/10;
                memory[memptr+2] = regs[vx]%10;
                #ifdef DEBUG
                s << "Store BCD of r" << (unsigned int)vx << " starting at " << memptr
                    << " (" << (unsigned int)memory[memptr] << "," << (unsigned int)memory[memptr+1] 
                    << "," << (unsigned int)memory[memptr+2] << ")";
                #endif
                break;

            // Store registers v0 to vX (inclusive) in memory starting at memptr. Increment memptr by X+1
            // NOTE: allows self-modifying code
            case 0x55:
                vx = (inst & 0x0f00) >> 8;
                for (uint8_t r = 0; r <= vx; r++)
                {
                    memory[memptr] = regs[r];
                    memptr++;
                }
                #ifdef DEBUG
                s << "Store r0 to r" << (unsigned int)vx << " starting at " << memptr-vx-1;
                #endif
                break;

            // Fill registers v0 to vX (inclusive) from memory starting at memptr. Increment memptr by X+1
            case 0x65:
                vx = (inst & 0x0f00) >> 8;
                for (uint8_t r = 0; r <= vx; r++)
                {
                    regs[r] = memory[memptr];
                    memptr++;
                }
                #ifdef DEBUG
                s << "Load r0 to r" << (unsigned int)vx << " starting at " << memptr-vx-1;
                #endif
                break;
            default:
                break;
            }

            break;

        default:
            #ifdef DEBUG
            s << "undefined instruction";
            #endif
            break;
        }
    #ifdef DEBUG
    std::cout << s.str() << std::endl;
    #endif
    addrptr += 2;
    i++;
    }
    
}

