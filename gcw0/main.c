#ifdef __WIN32__
#include <windows.h>
#else
#define MessageBox(owner, text, caption, type) printf("%s: %s\n", caption, text)
#endif

#include <SDL.h>
#include <SDL_thread.h>

#include "shared.h"
#include "sms_ntsc.h"
#include "md_ntsc.h"
#include "utils.h"

#ifdef GCWZERO
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <time.h>

static int sy1, sy2, sy3 = 0;
static int dy1, dy2, dy3 = 0;
static int  h1,  h2,  h3 = 0;
static short unsigned int old_srect_y;
static short unsigned int old_drect_y;
int    gcwzero_cycles = 3420;
uint8  do_once        = 1;
uint32 gcw0_w         = 320;
uint32 gcw0_h         = 240;
uint8  gotomenu       = 0;
uint8  show_lightgun  = 0;
uint8  virtua_racing  = 0;
uint   post           = 0;
uint   then           = 0;
uint   frametime      = 0;
static uint skipval   = 0;
uint8  frameskip      = 0;
time_t current_time;
char rom_filename[256];
#ifdef SDL2
SDL_Window   *window;
SDL_Renderer *renderer;
SDL_Texture  *texture;
#endif
const char *cursor[4]=
{
    "./CLASSIC_01_RED.png", //doesn't flash (for epileptics it's default)
    "./CLASSIC_02.png",     //square flashing red and white
    "./CLASSIC_01.png",
    "./SQUARE_02.png",
};

#endif

#define SOUND_FREQUENCY 44100
#define SOUND_SAMPLES_SIZE 2048
#define VIDEO_WIDTH  320
#define VIDEO_HEIGHT 240

int  joynum     = 0;
int  log_error  = 0;
int  debug_on   = 0;
uint turbo_mode = 0;
uint use_sound  = 1;
uint fullscreen = 1; /* SDL_FULLSCREEN */

/***************************************************************
 * This function enables various speed hacks.                  *
 * If you don't want them remove references to this function.  *
 * This function is only called for Virtua racing and MCD roms.* 
 ***************************************************************/
int calc_framerate(int optimisations)
{
    if (!gotomenu)
    {
	uint now  = SDL_GetTicks();
        frametime = now - then;
        then      = now;

        static sint8 frameskipcount = 5;
        //printf("\nframerate = %d", frametime);
if(optimisations && drawn_from_line > 1)
{
        if (virtua_racing)
        {
            if (config.optimisations == 1) //Balanced performance improvements
            {
                if (frametime > (vdp_pal ? 50 : 60))
                {
                    turbo_mode = 1;
                    if (SVP_cycles >= 600)      SVP_cycles -= 100;
                    gcwzero_cycles = (vdp_pal ? 2859 : 2873);
                    frameskipcount = 0;
                    if (frameskip < 2)
                        frameskip ++;
                }
                else
                {
                    turbo_mode = 0;
                  if (SVP_cycles <= 780)      SVP_cycles += 20;
                    gcwzero_cycles = (vdp_pal ? 3010 : 3192);
                    if (++frameskipcount > 24)
                    {
                        if (frameskip)
                            frameskip--;
                        frameskipcount = 0;
                    }
                }
            }
            else //Full speed ahead!
            {
                if (frametime > (vdp_pal ? 55 : 66))
                {
                    if (SVP_cycles >= 300)      SVP_cycles -= 160;
                    turbo_mode     = 1;
                    gcwzero_cycles = (vdp_pal ? 2700 : 2720);
                    frameskipcount = 0;
                    if (frameskip < 3)
                        frameskip += 2;
                }
                else
                if (frametime > (vdp_pal ? 45 : 54))
                {
                    if (SVP_cycles >= 500)      SVP_cycles -= 80;
                    gcwzero_cycles = (vdp_pal ? 2700 : 2720);
                }
                else
                {
                    if (SVP_cycles <= 760)      SVP_cycles += 20;
                    gcwzero_cycles = (vdp_pal ? 2859 : 2873);
                    if (++frameskipcount > 35)
                    {
                        if (frameskip)
                            frameskip--;
                        frameskipcount = 0;
                    }
                    if (frameskip < 1)
                        turbo_mode = 0;
                }

            }
        } //VR optimisations
        else //MCD optimisations
        {
            if (config.optimisations == 1) //Balanced optimisations
            {
                if (frametime > (vdp_pal ? 55 : 66))
                {
                    turbo_mode     = 1;
                    frameskipcount = 0;
                    if (!frameskip)
                        frameskip++;
                }
                else
                if (frametime > (vdp_pal ? 50 : 60))
                {
                    frameskipcount++;
                }
                else
                {
                    turbo_mode = 0;
                    frameskipcount++;
                    if (frameskipcount > 10)
                    {
                        if (frameskip)
                            --frameskip;
                        frameskipcount = 0;
                    }
                }
            } //balanced optimisations
            else //Speed optimisations
            {
                if (frametime > (vdp_pal ? 54 : 65))
                {
                    turbo_mode     = 1;
                    frameskipcount = 0;
                    if (frameskip < 3)
                        frameskip++;
                }
                else
                if (frametime > (vdp_pal ? 50 : 60))
                {
                    frameskipcount = 0;
                   if (frameskip < 2)
                        frameskip++;
                }
                else
                {
                    if (++frameskipcount > 34)
                    {
                        if (frameskip)
                            --frameskip;
                        frameskipcount = 0;
                    }
                    if(frameskip < 1)
                        turbo_mode = 0;

                }
            } //Speed optimisations
        } //MCD optimisations
} else return frametime;
    }
return 1;
}

/* sound */
struct
{
    char* current_pos;
    char* buffer;
    int current_emulated_samples;
} sdl_sound;


static uint8 brm_format[0x40] =
{
    0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x00,0x00,0x00,0x00,0x40,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x53,0x45,0x47,0x41,0x5f,0x43,0x44,0x5f,0x52,0x4f,0x4d,0x00,0x01,0x00,0x00,0x00,
    0x52,0x41,0x4d,0x5f,0x43,0x41,0x52,0x54,0x52,0x49,0x44,0x47,0x45,0x5f,0x5f,0x5f
};

static short soundframe[SOUND_SAMPLES_SIZE];

static void sdl_sound_callback(void *userdata, Uint8 *stream, int len)
{
    if (sdl_sound.current_emulated_samples < len)
        memset(stream, 0, len);
    else
    {
        memcpy(stream, sdl_sound.buffer, len);
        /* loop to compensate desync */
        uint len_times_two = 2 * len;
        do
        {
            sdl_sound.current_emulated_samples -= len;
        }
        while(sdl_sound.current_emulated_samples > len_times_two);
        memcpy(sdl_sound.buffer,
               sdl_sound.current_pos - sdl_sound.current_emulated_samples,
               sdl_sound.current_emulated_samples);
        sdl_sound.current_pos = sdl_sound.buffer + sdl_sound.current_emulated_samples;
    }
}

static int sdl_sound_init()
{
    int n;
    SDL_AudioSpec as_desired, as_obtained;

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        MessageBox(NULL, "SDL Audio initialization failed", "Error", 0);
        return 0;
    }

    if (strstr(rominfo.international,"Virtua Racing"))
        as_desired.freq     = SOUND_FREQUENCY / 4; //speed hack, assumes SOUND_FREQUENCY = 44100kHz
    else
        as_desired.freq     = SOUND_FREQUENCY;
    as_desired.format   = AUDIO_S16LSB;
    as_desired.channels = 2;
    as_desired.samples  = SOUND_SAMPLES_SIZE;
    as_desired.callback = sdl_sound_callback;

    if (SDL_OpenAudio(&as_desired, &as_obtained) == -1)
    {
        MessageBox(NULL, "SDL Audio open failed", "Error", 0);
        return 0;
    }
 
    if (as_desired.samples != as_obtained.samples)
    {
        MessageBox(NULL, "SDL Audio wrong setup", "Error", 0);
        return 0;
    }
 
    sdl_sound.current_emulated_samples = 0;
    n = SOUND_SAMPLES_SIZE * 2 * sizeof(short) * 20;
    sdl_sound.buffer = (char*)malloc(n);
    if (!sdl_sound.buffer)
    {
        MessageBox(NULL, "Can't allocate audio buffer", "Error", 0);
        return 0;
    }
    memset(sdl_sound.buffer, 0, n);
    sdl_sound.current_pos = sdl_sound.buffer;
    return 1;
}
 
static void sdl_sound_update(int enabled)
{
    int size = audio_update(soundframe) * 2;
 
    if (enabled)
    {
        unsigned int i;
        short *out;
 
        SDL_LockAudio();
        out = (short*)sdl_sound.current_pos;
        for(i = 0; i < size; i+=2)
        {
            *out++ = soundframe[i];
            *out++ = soundframe[i+1];
        }
        sdl_sound.current_pos = (char*)out;
        sdl_sound.current_emulated_samples += size * sizeof(short);
        SDL_UnlockAudio();
    }
}
 
static void sdl_sound_close()
{
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    if (sdl_sound.buffer)
        free(sdl_sound.buffer);
}

#ifdef GCWZERO //A-stick support
static void sdl_joystick_init()
{
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
        MessageBox(NULL, "SDL Joystick initialization failed", "Error", 0);
    else
        MessageBox(NULL, "SDL Joystick initialisation successful", "Success", 0);
    return;
}
#endif
 
/* video */
md_ntsc_t *md_ntsc;
sms_ntsc_t *sms_ntsc;
 
struct
{
    SDL_Surface* surf_screen;
    SDL_Surface* surf_bitmap;
    SDL_Rect srect;
    SDL_Rect drect;
    SDL_Rect my_srect; //for blitting small portions of the screen (custom blitter)
    SDL_Rect my_drect; //
    Uint32 frames_rendered;
} sdl_video;
 
static int sdl_video_init()
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        MessageBox(NULL, "SDL Video initialization failed", "Error", 0);
        return 0;
    }
#ifdef SDL2
    window                = SDL_CreateWindow("genplus", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240,  SDL_WINDOW_FULLSCREEN_DESKTOP);
    renderer              = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    sdl_video.surf_bitmap = SDL_CreateRGBSurface(0, 320, 240, 16, 0, 0, 0, 0);    /* You will need to change the pixelformat if using 32bits etc */
    texture               = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 320, 240);
#else
    if     (config.renderer == 0) //Triple buffering
        sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, 16, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_TRIPLEBUF);
    else if(config.renderer == 1) //Double buffering
        sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, 16, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF);
    else if(config.renderer == 2) //Software rendering
        sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, 16, SDL_SWSURFACE | SDL_FULLSCREEN                );

    sdl_video.surf_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0, 0, 0, 0);
    SDL_ShowCursor(0);
#endif //SDL2
    sdl_video.frames_rendered = 0;
    return 1;
}
 
static void sdl_video_update()
{
    if (system_hw == SYSTEM_MCD)
    {
#ifdef GCWZERO
        if ((skipval >= frameskip) && (skipval < 10))
        {
            do_not_blit = 1; //default is do not blit, we will change this later if a pixel changes on the screen.
            system_frame_scd(0); //render frame
            skipval = 10;
        } else {
            system_frame_scd(frameskip); //skip frame render
            ++skipval;
        }
#else
        system_frame_scd(0);
#endif
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
#ifdef GCWZERO
    {
        if ((skipval >= frameskip) && (skipval < 10)) 
        {
            do_not_blit = 1; //default is do not blit, we will change this later if a pixel changes on the screen.
            system_frame_gen(0);
            skipval = 10;
        } else {
            system_frame_gen(frameskip);
            ++skipval;
        }
#else
        system_frame_gen(0);
#endif
    }
    else
    {
#ifdef GCWZERO
        if (skipval >= frameskip) 
        {
            do_not_blit = 1; //default is do not blit, we will change this later if a pixel changes on the screen.
            system_frame_sms(0);
            skipval = 10;
        } else {
            system_frame_sms(1);
            ++skipval;
        }
#else
        system_frame_sms(0);
#endif
    }
 
    /* viewport size changed */
    if (bitmap.viewport.changed & 1)
    {
        bitmap.viewport.changed &= ~1;
 
        /* source bitmap */
      //remove left bar bug with SMS roms
        if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
        {
            if (config.smsmaskleftbar) sdl_video.srect.x = 8;
            else                       sdl_video.srect.x = 0;
        }
//        else   sdl_video.srect.x = 0;
        sdl_video.srect.y = 0;
        sdl_video.srect.w = bitmap.viewport.w + (2 * bitmap.viewport.x);
        sdl_video.srect.h = bitmap.viewport.h + (2 * bitmap.viewport.y);
        if (sdl_video.srect.w > VIDEO_WIDTH)
        {
//            sdl_video.srect.x = (sdl_video.srect.w - VIDEO_WIDTH) / 2;
            sdl_video.srect.w = VIDEO_WIDTH;
            if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
                if (config.smsmaskleftbar)
                    sdl_video.srect.x += 8;
        }
        if (sdl_video.srect.h > VIDEO_HEIGHT)
        {
            sdl_video.srect.y = (sdl_video.srect.h - VIDEO_HEIGHT) / 2;
            sdl_video.srect.h = VIDEO_HEIGHT;
        }
 
        /* destination bitmap */
        sdl_video.drect.w = sdl_video.srect.w;
        sdl_video.drect.h = sdl_video.srect.h;

//        sdl_video.drect.x = (VIDEO_WIDTH  - sdl_video.drect.w) / 2;
        sdl_video.drect.x = 0; //testing

        sdl_video.drect.y = (VIDEO_HEIGHT - sdl_video.drect.h) / 2;
 
        /* clear destination surface */
      //We're triple buffering so do some blank screen flips to stop any flickering
#ifdef SDL2
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
#else
        int i;
        for(i=0;i<3;i++)
        {
            SDL_FillRect(sdl_video.surf_screen, 0, 0);
            if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
            else                    SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
        }
#endif
        old_srect_y = sdl_video.srect.y;
        old_drect_y = sdl_video.drect.y;
        drawn_from_line = 0;
        sy1 = sy2 = sy3 = sdl_video.srect.y;
        dy1 = dy2 = dy3 = sdl_video.drect.y;
        h1  =  h2 =  h3 = sdl_video.srect.h;
        sdl_video.my_srect.x = sdl_video.srect.x;
        sdl_video.my_srect.y = sdl_video.srect.y;
        sdl_video.my_srect.w = sdl_video.srect.w;
        sdl_video.my_srect.h = sdl_video.srect.h;
        sdl_video.my_drect.x = sdl_video.drect.x;
        sdl_video.my_drect.y = sdl_video.drect.y;
        sdl_video.my_drect.w = sdl_video.drect.w;
        sdl_video.my_drect.h = sdl_video.drect.h;
    }

//DK IPU scaling for gg/sms roms
#ifndef SDL2
    if (config.gcw0_fullscreen)
    {
        if ( (gcw0_w != sdl_video.drect.w) || (gcw0_h != sdl_video.drect.h) )
        {
            if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
            {
                if (config.smsmaskleftbar)
                {
                    sdl_video.srect.w = sdl_video.srect.w - 8;
                    sdl_video.drect.w = sdl_video.srect.w;
                    sdl_video.drect.x = 4;
                }
                else
                {
                    sdl_video.srect.w = sdl_video.srect.w ;
                    sdl_video.drect.w = sdl_video.srect.w;
                    sdl_video.drect.x = 0;
                }

            }
            else
            {
                sdl_video.drect.x = 0;
                sdl_video.drect.w = sdl_video.srect.w;
            }

/* 
//this does not play nice with the custom blitter, off screen pixel changes mess everything up.
            if (strstr(rominfo.international,"Virtua Racing"))
            {
                sdl_video.srect.y = (sdl_video.srect.h - VIDEO_HEIGHT) / 2 + 24;
                sdl_video.drect.h = sdl_video.srect.h = 192;
                sdl_video.drect.y = 0;
            }
            else
            {
*/
                sdl_video.drect.h = sdl_video.srect.h;
                sdl_video.drect.y = 0;
/*
            }
*/
            gcw0_w = sdl_video.drect.w;
            gcw0_h = sdl_video.drect.h;

            old_srect_y = sdl_video.srect.y;
            old_drect_y = sdl_video.drect.y;
            drawn_from_line = 0;
            sy1 = sy2 = sy3 = sdl_video.srect.y;
            dy1 = dy2 = dy3 = sdl_video.drect.y;
            h1  =  h2 =  h3 = sdl_video.srect.h;
            sdl_video.my_srect.x = sdl_video.srect.x;
            sdl_video.my_srect.y = sdl_video.srect.y;
            sdl_video.my_srect.w = sdl_video.srect.w;
            sdl_video.my_srect.h = sdl_video.srect.h;
            sdl_video.my_drect.x = sdl_video.drect.x;
            sdl_video.my_drect.y = sdl_video.drect.y;
            sdl_video.my_drect.w = sdl_video.drect.w;
            sdl_video.my_drect.h = sdl_video.drect.h;

            if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
            {
#ifdef SDL2
#else
                if      (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
                else if (config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
                else if (config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, 16, SDL_SWSURFACE                );
#endif //SDL2
            }
            else
            { 
#ifdef SDL2
#else
                if           (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
                else if      (config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
                else if      (config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, 16, SDL_SWSURFACE                );
#endif //SDL2
            } 
        }
    }

    if (show_lightgun && !config.gcw0_fullscreen)  //hack to remove cursor corruption if over game screen edge
        SDL_FillRect(sdl_video.surf_screen, 0, 0);

    if (!frameskip || (skipval == 10) )
    {
      //Custom Blitter, see core/vdp_render.c
      if(!do_not_blit) //at least one pixel has changed so we need to blit and flip.
      {
        if(system_hw == SYSTEM_MCD || (system_hw & SYSTEM_PBC) == SYSTEM_MD)
        {
          // Define the new srect and drect value of y
          sdl_video.my_srect.y = old_srect_y + drawn_from_line - 1;
          sdl_video.my_drect.y = old_drect_y + drawn_from_line - 1;
          //Reduce h if we're stopping blitting early
          sdl_video.my_srect.h = sdl_video.my_drect.h = (drawn_to_line - drawn_from_line + 2);
          if(config.renderer <  2) //we're using hardware rendering so we need to prevent flicker
          {
            /************************************************************************************************
             * We first need to find out what is the minimum value of y to blit, then the max value of y+h. *
             * We use the previous three values as we are triple buffering.                                 *
             ************************************************************************************************/
            static int rot = 0; //rotate between settings
            rot++;
            if (rot == 1) {          sy1 = sdl_video.my_srect.y; dy1 = sdl_video.my_drect.y; h1 =  sdl_video.my_srect.h; } else
            if (rot == 2) {          sy2 = sdl_video.my_srect.y; dy2 = sdl_video.my_drect.y; h2 =  sdl_video.my_srect.h; } else
                          { rot = 0; sy3 = sdl_video.my_srect.y; dy3 = sdl_video.my_drect.y; h3 =  sdl_video.my_srect.h; }

            //which is lowest (or are they equal?)
            if (sy1 <= sy2 && sy1 <= sy3) { sdl_video.my_srect.y = sy1; sdl_video.my_drect.y = sy1; } else
            if (sy2 <= sy1 && sy2 <= sy3) { sdl_video.my_srect.y = sy2; sdl_video.my_drect.y = sy2; } else
            if (sy3 <= sy1 && sy3 <= sy2) { sdl_video.my_srect.y = sy3; sdl_video.my_drect.y = sy3; }

            //what is the highest value of y + h?
            if(sy1 + h1 >= sy2 + h2 && sy1 + h1 >= sy3 + h3) sdl_video.my_srect.h = sdl_video.my_drect.h = sy1 + h1 - sdl_video.my_srect.y; else
            if(sy2 + h2 >= sy1 + h1 && sy2 + h2 >= sy3 + h3) sdl_video.my_srect.h = sdl_video.my_drect.h = sy2 + h2 - sdl_video.my_srect.y; else
            if(sy3 + h3 >= sy1 + h1 && sy3 + h3 >= sy2 + h2) sdl_video.my_srect.h = sdl_video.my_drect.h = sy3 + h3 - sdl_video.my_srect.y;
          }//config.renderer < 2
/*
          if (config.gcw0_fullscreen)
          {
            gcw0_w = sdl_video.drect.w;
            gcw0_h = sdl_video.drect.h;
          }
*/
          SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.my_srect, sdl_video.surf_screen, &sdl_video.my_drect);
          skipval = 19;
        } //system = mcd or md
        else //for older systems we'll just blit, we don't need to optimise these further anyway.
        {
          SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.srect, sdl_video.surf_screen, &sdl_video.drect);
        }
      } //!do_not_blit so we'll do nothing...unless...
      else if (show_lightgun)
      {
        SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.srect, sdl_video.surf_screen, &sdl_video.drect);
        skipval = 19;
      }
    }
#endif //!SDL2
#ifdef GCWZERO
  //Add scanlines to Game Gear games if requested
    if ( (system_hw == SYSTEM_GG) && config.gg_scanlines)
    {
        SDL_Surface *scanlinesSurface;
        scanlinesSurface = IMG_Load("./scanlines.png");
        SDL_BlitSurface(scanlinesSurface, NULL, sdl_video.surf_screen, &sdl_video.drect);
	SDL_FreeSurface(scanlinesSurface);
    }
    if (show_lightgun)
    {
      //Remove previous cursor from black bars
        if (config.gcw0_fullscreen)
        {
            if (config.smsmaskleftbar && system_hw == SYSTEM_SMS2)
            {
                SDL_Rect srect;
                srect.x = srect.y = 0;
                srect.w = 4;
                srect.h = 192;
                SDL_FillRect(sdl_video.surf_screen, &srect, SDL_MapRGB(sdl_video.surf_screen->format, 0, 0, 0));
                srect.x = 252;
                SDL_FillRect(sdl_video.surf_screen, &srect, SDL_MapRGB(sdl_video.surf_screen->format, 0, 0, 0));
            }
        }
        /* get mouse coordinates (absolute values) */
        int x,y;
        SDL_GetMouseState(&x,&y);

        SDL_Rect lrect;
        lrect.x = x-7;
        lrect.y = y-7;
        lrect.w = lrect.h = 15;

        SDL_Surface *lightgunSurface;
        lightgunSurface = IMG_Load(cursor[config.cursor]);
        static int lightgun_af = 0;
        SDL_Rect srect;
        srect.y = 0;
        srect.w = srect.h = 15;

        //only show cursor if movement occurred within 3 seconds.
        time_t current_time2;
        current_time2 = time(NULL);

        if (lightgun_af >= 10)
        {
            srect.x = 0;
            if ( ( current_time2 - current_time ) < 3 )
                SDL_BlitSurface(lightgunSurface, &srect, sdl_video.surf_screen, &lrect);
        }
        else
        {
            if (config.cursor != 0) srect.x = 15;
            else                    srect.x = 0;
            if ( ( current_time2 - current_time ) < 3 )
                SDL_BlitSurface(lightgunSurface, &srect, sdl_video.surf_screen, &lrect);
        }
        lightgun_af++;
        if (lightgun_af == 20) lightgun_af = 0;
        SDL_FreeSurface(lightgunSurface);
    } //show_lightgun
#endif
    if ( !frameskip || (skipval > 19) )
    {
#ifdef SDL2
        SDL_UpdateTexture(texture, NULL, bitmap.data, 320 * sizeof(Uint16));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
#else
        if(!do_not_blit)
        {
          if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
          else                    SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
        }
        else if(show_lightgun)
        {
          if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
          else                    SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
        }
#endif
        skipval = 0;
    }

    if (++sdl_video.frames_rendered == 3)
        sdl_video.frames_rendered = 0;
}
 
static void sdl_video_close()
{
    if (sdl_video.surf_bitmap)
        SDL_FreeSurface(sdl_video.surf_bitmap);
    if (sdl_video.surf_screen)
        SDL_FreeSurface(sdl_video.surf_screen);
}
 
/* Timer Sync */
 
struct
{
    SDL_sem* sem_sync;
    unsigned ticks;
} sdl_sync;
 
Uint32 sdl_sync_timer_callback(Uint32 interval, void *param)
{
    SDL_SemPost(sdl_sync.sem_sync);
    post++;
    return interval;
}
 
static int sdl_sync_init()
{
#ifdef SDL2
    if (SDL_InitSubSystem(SDL_INIT_TIMER|SDL_INIT_EVENTS) < 0)
#else
    if (SDL_InitSubSystem(SDL_INIT_TIMER|SDL_INIT_EVENTTHREAD) < 0)
#endif
    {
        MessageBox(NULL, "SDL Timer initialization failed", "Error", 0);
        return 0;
    }
 
    sdl_sync.sem_sync = SDL_CreateSemaphore(0);
    sdl_sync.ticks = 0;
    return 1;
}
 
static void sdl_sync_close()
{
    if (sdl_sync.sem_sync)
        SDL_DestroySemaphore(sdl_sync.sem_sync);
}
 
static const uint16 vc_table[4][2] =
{
    /* NTSC, PAL */
    {0xDA , 0xF2},  /* Mode 4 (192 lines) */
    {0xEA , 0x102}, /* Mode 5 (224 lines) */
    {0xDA , 0xF2},  /* Mode 4 (192 lines) */
    {0x106, 0x10A}  /* Mode 5 (240 lines) */
};
#ifdef SDL2
static int sdl_control_update(SDL_Keycode keystate)
#else
static int sdl_control_update(SDLKey keystate)
#endif
{
    return 1;
}
 
static void shutdown()
{
    FILE *fp;
 
    if (system_hw == SYSTEM_MCD)
    {
        /* save internal backup RAM (if formatted) */
        char brm_file[256];
        if (!memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20))
        {
            sprintf(brm_file,"%s/%s", get_save_directory(), "scd.brm");
            fp = fopen(brm_file, "wb");
            if (fp!=NULL)
            {
                fwrite(scd.bram, 0x2000, 1, fp);
                fclose(fp);
            }
        }
 
        /* save cartridge backup RAM (if formatted) */
        if (scd.cartridge.id)
        {
            if (!memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20))
            {
                sprintf(brm_file,"%s/%s", get_save_directory(), "cart.brm");
                fp = fopen(brm_file, "wb");
                if (fp!=NULL)
                {
                    fwrite(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
                    fclose(fp);
                }
            }
        }
    }
 
    if (sram.on)
    {
        /* save SRAM */
        char save_file[256];
        if (rom_filename[0] != '\0') {
			sprintf(save_file,"%s/%s.srm", get_save_directory(), rom_filename);
			fp = fopen(save_file, "wb");
			if (fp!=NULL)
			{
				fwrite(sram.sram,0x10000,1, fp);
				fclose(fp);
			}
		}
    }
    audio_shutdown();
    error_shutdown();
 
    sdl_video_close();
    sdl_sound_close();
    sdl_sync_close();
    SDL_Quit();
}

void gcw0_savestate(int slot)
{
    char save_state_file[256];
    sprintf(save_state_file,"%s/%s.gp%d", get_save_directory(), rom_filename, slot);
    FILE *f = fopen(save_state_file,"wb");
    if (f)
    {
        uint8 buf[STATE_SIZE];
        int len = state_save(buf);
        fwrite(&buf, len, 1, f);
        fclose(f);
    }
}

void gcw0_loadstate(int slot)
{
    char save_state_file[256];
    sprintf(save_state_file,"%s/%s.gp%d", get_save_directory(), rom_filename, slot);
    FILE *f = fopen(save_state_file,"rb");
    if (f)
    {
        uint8 buf[STATE_SIZE];
        fread(&buf, STATE_SIZE, 1, f);
        state_load(buf);
        fclose(f);
    }
} 

#ifdef GCWZERO //menu!
static int gcw0menu(void)
{
    enum {MAINMENU = 0, GRAPHICS_OPTIONS = 1, REMAP_OPTIONS = 2, SAVE_STATE = 3, LOAD_STATE = 4, MISC_OPTIONS = 5};
    static int menustate = MAINMENU;
    static int renderer;
    renderer = config.renderer;

  //Menu text
    const char *gcw0menu_mainlist[9]=
    {
        "Resume game",
        "Save state",
        "Load state",
        "Graphics options",
        "Remap buttons",
        "Misc. Options",
        "", //spacer
        "Reset",
        "Quit"
    };
    const char *gcw0menu_gfxlist[5]=
    {
        "Return to main menu",
        "Renderer",
        "Scaling",
        "Keep aspect ratio",
        "Scanlines (GG)",
    };
    const char *gcw0menu_numericlist[5]=
    {
        " 0",
        " 1",
        " 2",
        " 3",
        " 4",
    };
    const char *gcw0menu_optimisations[3]=
    {
        " Off",
        " On: Balanced",
        " On: Performance",
    };
    const char *gcw0menu_renderer[3]=
    {
        " HW Triple Buf",
        " HW Double Buf",
        " Software",
    };
    const char *gcw0menu_onofflist[2]=
    {
        " Off",
        " On",
    };
    const char *gcw0menu_deadzonelist[7]=
    {
        " 0",
        " 5,000",
        " 10,000",
        " 15,000",
        " 20,000",
        " 25,000",
        " 30,000",
    };
    const char *gcw0menu_remapoptionslist[9]=
    {
        "Return to main menu",
        "A",
        "B",
        "C",
        "X",
        "Y",
        "Z",
        "Start",
        "Mode",
    };
    const char *gcw0menu_savestate[10]=
    {
        "Back to main menu",
        "Save state 1 (Quicksave)",
        "Save state 2",
        "Save state 3",
        "Save state 4",
        "Save state 5",
        "Save state 6",
        "Save state 7",
        "Save state 8",
        "Save state 9",
    };
    const char *gcw0menu_loadstate[10]=
    {
        "Back to main menu",
        "Load state 1 (Quickload)",
        "Load state 2",
        "Load state 3",
        "Load state 4",
        "Load state 5",
        "Load state 6",
        "Load state 7",
        "Load state 8",
        "Load state 9",
    };
    const char *gcw0menu_misc[9]=
    {
        "Back to main menu",
        "Optimisations (MCD/VR)",
        "Resume on Save/Load",
        "A-stick",
        "A-stick deadzone",
        "Lock-on(MD)",
        "FM sound (SMS)",
        "Lightgun speed",
        "Lightgun Cursor",
    };
    const char *lock_on_desc[4]=
    {
        "            Off  ",
        "       Game Genie",
        "    Action Replay",
        " Sonic & Knuckles",
    };

    SDL_PauseAudio(1);
    bitmap.viewport.changed = 1; //change screen res if required

  //set up surfaces
    SDL_Surface *tempbgSurface = NULL;
    SDL_Surface *bgSurface     = NULL;
    SDL_Surface *menuSurface   = NULL;
    menuSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 32, 0, 0, 0, 0);

    /* display menu */
  //change video mode
#ifndef SDL2
    if      (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, 32, SDL_HWSURFACE | SDL_TRIPLEBUF);
    else if (config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    else if (config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, 32, SDL_SWSURFACE                );
#endif //!SDL2

  //blank screen
    SDL_FillRect(sdl_video.surf_screen, 0, 0);

  //setup fonts
    TTF_Init();
    TTF_Font *ttffont = NULL;
    ttffont = TTF_OpenFont("./ProggyTiny.ttf", 16);
    SDL_Color text_color = {180, 180, 180};
    SDL_Color selected_text_color = {23, 86, 155}; //selected colour = Sega blue ;)

  //identify system we are using to show correct background just cos we can :P
    switch(system_hw)
    {
    case SYSTEM_PICO:
        tempbgSurface = IMG_Load( "./PICO.png" );
        break;
    case SYSTEM_SG: //SG-1000 I&II
        case SYSTEM_SGII:
        tempbgSurface = IMG_Load( "./SG1000.png" );
        break;
    case SYSTEM_MARKIII: //Mark III & Sega Master System I&II & Megadrive with power base converter
    case SYSTEM_SMS:
    case SYSTEM_GGMS:
    case SYSTEM_SMS2:
    case SYSTEM_PBC:
        tempbgSurface = IMG_Load( "./SMS.png" );
        break;
    case SYSTEM_GG:
        tempbgSurface = IMG_Load( "./GG.png" );
        break;
    case SYSTEM_MD:
        tempbgSurface = IMG_Load( "./MD.png" );
        break;
    case SYSTEM_MCD:
        tempbgSurface = IMG_Load( "./MCD.png" );
        break;
    default:
        tempbgSurface = IMG_Load( "./MD.png" );
        break;
    }
#ifdef SDL2
//TODO skip this to allow compile for now
#else
    bgSurface = SDL_DisplayFormat( tempbgSurface );
#endif
    SDL_FreeSurface(tempbgSurface);

  //start menu loop
    while(gotomenu)
    {
        static int selectedoption = 0;
        char remap_text[256];
        char load_state_screenshot[256];
        int savestate = 0;

      //Initialise surfaces
        SDL_Surface *textSurface;
        SDL_Surface *MenuBackground;

      //Blit the background image
        SDL_BlitSurface(bgSurface, NULL, menuSurface, NULL);

      //Fill menu box
        SDL_Rect rect;
        if (menustate == REMAP_OPTIONS)
        {
            MenuBackground = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 185, 16, 0, 0, 0, 0);
            rect.x = 0;
            rect.y = 35;
            rect.w = 320;
            rect.h = 185;
        }
        else
        {
            MenuBackground = SDL_CreateRGBSurface(SDL_SWSURFACE, 200, 185, 16, 0, 0, 0, 0);
            rect.x = 60;
            rect.y = 35;
            rect.w = 200;
            rect.h = 185;
        }
        SDL_FillRect(MenuBackground, 0, 0);
#ifdef SDL2
//TODO  skip this to allow compile for now
#else
        SDL_SetAlpha(MenuBackground, SDL_SRCALPHA, 50);
#endif

      //Show title
        SDL_Rect destination;
        destination.x = 70;
        destination.y = 40;
        destination.w = 100;
        destination.h = 50;
        textSurface = TTF_RenderText_Solid(ttffont, "Genesis Plus GX", text_color);

      //Blit background and title
        SDL_BlitSurface(MenuBackground, NULL, menuSurface, &rect);
        SDL_BlitSurface(textSurface,    NULL, menuSurface, &destination);

      //Free surfaces
        SDL_FreeSurface(MenuBackground);
        SDL_FreeSurface(textSurface);

        switch(menustate)
        {
        case MAINMENU:
            for(int i = 0; i < 9; i++)
            {
                destination.x = 70;
                destination.y = 70+(15*i);
                destination.w = 100;
                destination.h = 50;
                if (i == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_mainlist[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_mainlist[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
                SDL_FreeSurface(textSurface);
            }
            break;
        case GRAPHICS_OPTIONS:
            for(int i = 0; i < 5; i++)
            {
                destination.x = 70;
                destination.y = 70 + (15 * i);
                destination.w = 100;
                destination.h = 50;
                if ((i + 10) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_gfxlist[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_gfxlist[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
                SDL_FreeSurface(textSurface);
            }
            /* Display On/Off */
            destination.x = 210;
            destination.w = 100; 
            destination.h = 50;
          //Renderer
            destination.y = 70 + (15 * 1);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_renderer[config.renderer], selected_text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //Scaling
            destination.y = 70 + (15 * 2);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_onofflist[config.gcw0_fullscreen], selected_text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //Aspect ratio
            destination.y = 70 + (15 * 3);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_onofflist[config.keepaspectratio], selected_text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //Scanlines
            destination.y = 70 + (15 * 4);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_onofflist[config.gg_scanlines], selected_text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
            break;

        case REMAP_OPTIONS:
            sprintf(remap_text, "%s%25s", "GenPlus", "GCW-Zero");
            destination.x = 30;
            destination.y = 80;
            destination.w = 100;
            destination.h = 50;
            textSurface = TTF_RenderText_Solid(ttffont, remap_text, text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
            for(int i = 0; i < 9; i++)
            {
                if (!i)
                    sprintf(remap_text, gcw0menu_remapoptionslist[i]);  //for return option
                else
                    sprintf(remap_text, "%-5s                   %-7s", gcw0menu_remapoptionslist[i], gcw0_get_key_name(config.buttons[i - 1]));
                destination.x = 30;
                if (!i)
                    destination.y = 60;
                else
                    destination.y = 80 + (15 * i);
                destination.w = 100;
                destination.h = 50;
                if ((i + 20) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, remap_text, selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, remap_text, text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
                SDL_FreeSurface(textSurface);
            }
            break;
        case SAVE_STATE:
          //Show saved BMP as background if available
            sprintf(load_state_screenshot,"%s/%s.%d.bmp", get_save_directory(), rom_filename, selectedoption - 30);
            SDL_Surface* screenshot;
            screenshot = SDL_LoadBMP(load_state_screenshot);
            if (screenshot)
            {
                destination.x = (320 - screenshot->w) / 2;
                destination.y = (240 - screenshot->h) / 2;
                destination.w = 320;
                destination.h = 240;
                SDL_BlitSurface(screenshot, NULL, menuSurface, &destination);

              //Fill menu box
                SDL_Surface *MenuBackground = SDL_CreateRGBSurface(SDL_SWSURFACE, 180, 185, 16, 0, 0, 0, 0);
                SDL_Rect rect;
                rect.x = 60;
                rect.y = 35;
                rect.w = 180;
                rect.h = 185;
                SDL_FillRect(MenuBackground, 0, 0);
#ifdef SDL2
//TODO skip this to allow compile for now
#else
                SDL_SetAlpha(MenuBackground, SDL_SRCALPHA, 180);
#endif
                SDL_BlitSurface(MenuBackground, NULL, menuSurface, &rect);
                SDL_FreeSurface(MenuBackground);
            }
          //Show title
            destination.x = 70;
            destination.y = 40;
            destination.w = 100;
            destination.h = 50;
            textSurface = TTF_RenderText_Solid(ttffont, "Genesis Plus GX", text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);

            for(int i = 0; i < 10; i++)
            {
                destination.x = 70;
                destination.y = 70 + (15 * i);
                destination.w = 100;
                destination.h = 50;
                if ((i + 30) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_savestate[i], selected_text_color);
	        else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_savestate[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
                SDL_FreeSurface(textSurface);
            }
            savestate = 1;
        case LOAD_STATE:
            if (!savestate)
            {
              //Show saved BMP as background if available
                sprintf(load_state_screenshot,"%s/%s.%d.bmp", get_save_directory(), rom_filename, selectedoption - 40);
                screenshot = SDL_LoadBMP(load_state_screenshot);
                if (screenshot)
                {
                    destination.x = (320 - screenshot->w) / 2;
                    destination.y = (240 - screenshot->h) / 2;
                    destination.w = 320;
                    destination.h = 240;
                    SDL_BlitSurface(screenshot, NULL, menuSurface, &destination);

                  //Fill menu box
                    SDL_Surface *MenuBackground = SDL_CreateRGBSurface(SDL_SWSURFACE, 180, 185, 16, 0, 0, 0, 0);
                    SDL_Rect rect;
                    rect.x = 60;
                    rect.y = 35;
                    rect.w = 200;
                    rect.h = 185;
                    SDL_FillRect(MenuBackground, 0, 0);
#ifdef SDL2
//TODO skip this to allow compile for now
#else
                    SDL_SetAlpha(MenuBackground, SDL_SRCALPHA, 180);
#endif
                    SDL_BlitSurface(MenuBackground, NULL, menuSurface, &rect);
                    SDL_FreeSurface(MenuBackground);
                }

              //Show title
                destination.x = 70;
                destination.y = 40;
                destination.w = 100;
                destination.h = 50;
                textSurface = TTF_RenderText_Solid(ttffont, "Genesis Plus GX", text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
                SDL_FreeSurface(textSurface);
                for(int i = 0; i < 10; i++)
                {
                    destination.x = 70;
                    destination.y = 70 + (15 * i);
                    destination.w = 100;
                    destination.h = 50;
                    if ((i + 40) == selectedoption)
	                textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_loadstate[i], selected_text_color);
	            else
	                textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_loadstate[i], text_color);
                    SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
                    SDL_FreeSurface(textSurface);
                }
            }

            if (screenshot) SDL_FreeSurface(screenshot);
            savestate = 0;
            break;
        case MISC_OPTIONS:
            for(int i = 0; i < 9; i++)
            {
                destination.x = 70;
                destination.y = 70 + (15 * i);
                destination.w = 100;
                destination.h = 50;
                if ((i + 50) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_misc[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_misc[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
                SDL_FreeSurface(textSurface);
            }
            /* Display On/Off */
            destination.x = 210;
            destination.w = 100; 
            destination.h = 50;
          //Optimisations
            destination.y = 70 + (15 * 1);
    	    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_optimisations[config.optimisations], selected_text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //Save/load autoresume
            destination.y = 70 + (15 * 2);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_onofflist[config.sl_autoresume], selected_text_color);
    	    SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //A-stick
            destination.y = 70 + (15 * 3);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_onofflist[config.a_stick], selected_text_color);
    	    SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //A-stick sensitivity
            destination.y = 70 + (15 * 4);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_deadzonelist[config.deadzone], selected_text_color);
    	    SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //Display Lock-on Types
            destination.x = 144;
            destination.y = 70 + (15 * 5);
            textSurface = TTF_RenderText_Solid(ttffont, lock_on_desc[config.lock_on], selected_text_color);
    	    SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //FM sound(SMS)
            destination.x = 210;
            destination.y = 70 + (15 * 6);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_onofflist[config.ym2413], selected_text_color);
    	    SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //Lightgun speed
            destination.x = 210;
            destination.y = 70 + (15 * 7);
            textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_numericlist[config.lightgun_speed], selected_text_color);
    	    SDL_BlitSurface(textSurface, NULL, menuSurface, &destination);
            SDL_FreeSurface(textSurface);
          //Lightgun Cursor
            destination.y = 70 + (15 * 8);
            SDL_Surface *lightgunSurface;
            lightgunSurface = IMG_Load(cursor[config.cursor]);
            static int lightgun_af_demo = 0;
            SDL_Rect srect;
            srect.x = srect.y = 0;
            srect.w = srect.h = 15;
            if (lightgun_af_demo >= 10 && config.cursor != 0) srect.x = 15;
            lightgun_af_demo++;
            if (lightgun_af_demo == 20) lightgun_af_demo = 0;
            SDL_BlitSurface(lightgunSurface, &srect, menuSurface, &destination);
            SDL_FreeSurface(lightgunSurface);
            break;

/* other menu's go here */

        default:
            break;
        }

        /* Update display */
        SDL_Rect dest;
        dest.w = 320;
        dest.h = 240;
        dest.x = dest.y = 0;
        SDL_BlitSurface(menuSurface, NULL, sdl_video.surf_screen, &dest);
#ifdef SDL2
        SDL_RenderPresent(renderer);
#else
        if(renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
        else             SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
#endif
        /* Check for user input */
#ifdef SDL2
//TODO skip this to allow compile for now
#else
        SDL_EnableKeyRepeat(0, 0);
#endif
        static int keyheld = 0;
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_KEYDOWN:
                sdl_control_update(event.key.keysym.sym);
                break;
            case SDL_KEYUP:
                keyheld = 0;
                break;
            default:
                break;
            }
        }
        if (event.type == SDL_KEYDOWN && !keyheld)
        {
            keyheld++;
            uint8 *keystate2;
#ifdef SDL2
            keystate2 = SDL_GetKeyboardState(NULL);
#else
            keystate2 = SDL_GetKeyState(NULL);
#endif
            if (menustate == REMAP_OPTIONS)
            { //REMAP_OPTIONS needs to capture all input
#ifdef SDL2
                SDL_Keycode pressed_key = 0;
#else
                SDLKey pressed_key = 0;
#endif
                if      (keystate2[SDLK_RETURN]   ) pressed_key = SDLK_RETURN;
                else if (keystate2[SDLK_LCTRL]    ) pressed_key = SDLK_LCTRL;
                else if (keystate2[SDLK_LALT]     ) pressed_key = SDLK_LALT;
                else if (keystate2[SDLK_LSHIFT]   ) pressed_key = SDLK_LSHIFT;
                else if (keystate2[SDLK_SPACE]    ) pressed_key = SDLK_SPACE;
                else if (keystate2[SDLK_TAB]      ) pressed_key = SDLK_TAB;
                else if (keystate2[SDLK_BACKSPACE]) pressed_key = SDLK_BACKSPACE;
                else if (keystate2[SDLK_ESCAPE]   ) pressed_key = SDLK_ESCAPE;

                if (pressed_key)
                {
                    if (selectedoption == 20)
                    {
                        if(pressed_key == SDLK_LCTRL || pressed_key == SDLK_LALT) //return to main menu
                        { //this allows the next if statement to be activated, otherwise the code gets messy due to out of order changes to selectedoption.
                            SDL_Delay(130);
                            menustate = MAINMENU;
                        }
                    }
                    else
                    {
                        switch(selectedoption)
                        {
                        case 21:;//button a remap
                            config.buttons[A]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 22:;//button b remap
                            config.buttons[B]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 23:;//button c remap
                            config.buttons[C]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 24:;//button x remap
                            config.buttons[X]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 25:;//button y remap
                            config.buttons[Y]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 26:;//button z remap
                            config.buttons[Z]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 27:;//button start remap
                            config.buttons[START] = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 28:;//button mode remap
                            config.buttons[MODE]  = pressed_key;                                break;
                        }
                        SDL_Delay(130);
                        config_save();
                    } 
                }
            } //remap menu

            if (keystate2[SDLK_DOWN])
            {
                selectedoption++;
                if (selectedoption ==  6) selectedoption =  7; //main menu spacer
                if (selectedoption ==  9) selectedoption =  0; //main menu
                if (selectedoption == 15) selectedoption = 10; //graphics menu
                if (selectedoption == 29) selectedoption = 20; //remap menu
                if (selectedoption == 40) selectedoption = 30; //save menu
                if (selectedoption == 50) selectedoption = 40; //load menu
                if (selectedoption == 59) selectedoption = 50; //misc menu
                SDL_Delay(100);
    	    }
            else if (keystate2[SDLK_UP])
            {

                if      (selectedoption ==   0) selectedoption =  9; //main menu
                selectedoption--;
                if      (selectedoption ==   6) selectedoption =  5; //main menu spacer
                else if (selectedoption ==   9) selectedoption = 14; //graphics menu
                else if (selectedoption ==  19) selectedoption = 28; //remap menu
                else if (selectedoption ==  29) selectedoption = 39; //save menu
                else if (selectedoption ==  39) selectedoption = 49; //load menu
                else if (selectedoption ==  49) selectedoption = 58; //misc menu
                SDL_Delay(100);
            }
	    else if (keystate2[SDLK_LALT] && menustate != REMAP_OPTIONS) //back to last menu or quit menu
            {
                SDL_Delay(130);
                switch(menustate)
                {
                case GRAPHICS_OPTIONS:
                    menustate = MAINMENU;
                    selectedoption = 3;
                    break;
                case SAVE_STATE:
                    menustate = MAINMENU;
                    selectedoption = 1;
                    break;
                case LOAD_STATE:
                    menustate = MAINMENU;
                    selectedoption = 2;
                    break;
                case MISC_OPTIONS:
                    menustate = MAINMENU;
                    selectedoption = 5;
                    break;
                case MAINMENU:
                    if (selectedoption == 20)
                        selectedoption = 4;
                    else
                    {
                        gotomenu = 0;
                        selectedoption = 0;
                    }
	            break;
                }
            }
            else if (keystate2[SDLK_LCTRL] && menustate != REMAP_OPTIONS)
            {
                SDL_Delay(130);
                switch(selectedoption)
                {
                case 0: //Resume
	            gotomenu=0;
                    selectedoption=0;
	            break;
                case 1: //Save
                    menustate = SAVE_STATE;
                    selectedoption = 30;
	            break;
                case 2: //Load
                    menustate = LOAD_STATE;
                    selectedoption = 40;
	            break;
                case 3: //Graphics
                    menustate = GRAPHICS_OPTIONS;
                    selectedoption = 10;
	            break;
                case 4: //Remap
                    menustate = REMAP_OPTIONS;
                    selectedoption = 20;
	            break;
                case 5: //Misc.
                    menustate = MISC_OPTIONS;
                    selectedoption = 50;
	            break;
                case 7: //Reset
                    gotomenu = 0;
                    selectedoption = 0;
                    system_reset();
                    break;
                case 8: //Quit
                    exit(0);
	            break;
                case 10: //Back to main menu
                    menustate = MAINMENU;
                    selectedoption = 3;
	            break;
                case 11: //Renderer
                    config.renderer ++;
                    if (config.renderer == 3) config.renderer = 0;
                    config_save();
	            break;
                case 12: //Scaling
                    config.gcw0_fullscreen = !config.gcw0_fullscreen;
                    config_save();
	            break;
                case 13: //Keep aspect ratio
                    config.keepaspectratio = !config.keepaspectratio;
                    config_save();
                    do_once = 1;
	            break;
                case 14: //Scanlines (GG)
                    config.gg_scanlines = !config.gg_scanlines;
                    config_save();
	            break;
                case 20: //Back to main menu
                    selectedoption = 4;
                    break;
                case 30: //Back to main menu
                    menustate = MAINMENU;
                    selectedoption = 1;
	            break;
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                case 36:
                case 37:
                case 38:
                case 39:;//Save state 1-9
                    SDL_Delay(120);
                    gcw0_savestate(selectedoption - 30);
                  //Save BMP screenshot
                    char save_state_screenshot[256];
                    sprintf(save_state_screenshot,"%s/%s.%d.bmp", get_save_directory(), rom_filename, selectedoption-30);
                    char save_state_screenshot2[256];
                    sprintf(save_state_screenshot2,"%s/%s.", get_save_directory(), rom_filename);
                    rename(save_state_screenshot2, save_state_screenshot);
                    if (config.sl_autoresume)
                    {
                        menustate = MAINMENU;
                        gotomenu  = selectedoption = 0;
                    }
                    break;
                case 40: //Back to main menu
                    menustate = MAINMENU;
                    selectedoption = 2;
	            break;
                case 41:
                case 42:
                case 43:
                case 44:
                case 45:
                case 46:
                case 47:
                case 48:
                case 49:;//Load state 1-9
                    SDL_Delay(120);
                    gcw0_loadstate(selectedoption - 40);
                    if (config.sl_autoresume)
                    {
                        menustate = MAINMENU;
                        gotomenu  = selectedoption = 0;
                    }
                    break;
                case 50: //return to main menu
                    menustate = MAINMENU;
                    selectedoption = 5;
	            break;
                case 51: //Optimisations
                    config.optimisations ++;
                    if (config.optimisations == 3) config.optimisations = 0;
                    config_save();
	            break;
                case 52: //toggle auto resume when save/loading
                    config.sl_autoresume = !config.sl_autoresume;
                    config_save();
	            break;
                case 53: //toggle A-Stick
                    config.a_stick = !config.a_stick;
                    config_save();
	            break;
                case 54: //toggle A-Stick deadzone
                    ++config.deadzone;
                    if (config.deadzone == 7) config.deadzone = 0;
                    config_save();
	            break;
                case 55: //toggle or change lock-on device
                    config.lock_on = (++config.lock_on == 4)? 0 : config.lock_on;
                    config_save();
	            break;
                case 56: //Toggle high quality FM for SMS
                    config.ym2413 = !config.ym2413;
                    config_save();
	            break;
                case 57: //Lightgun speed
                    config.lightgun_speed++;
                    if (config.lightgun_speed == 4)
                        config.lightgun_speed  = 1;
                    config_save();
	            break;
                case 58:
                    config.cursor++;
                    if (config.cursor == 4)
                        config.cursor  = 0;
                    config_save();
	            break;
                default:
	            break;
                }
            }
        }
    }//menu loop

    TTF_CloseFont (ttffont);
    SDL_FreeSurface(menuSurface);
    SDL_FreeSurface(bgSurface);
    SDL_PauseAudio(0);

    if (config.gcw0_fullscreen) 
    {
        if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
        {
            gcw0_w = sdl_video.drect.w;
            gcw0_h = sdl_video.drect.h;
#ifdef SDL2
#else
            if     (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
            else if(config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
            else if(config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, 16, SDL_SWSURFACE                );
#endif
        }
        else
        { 
            sdl_video.drect.w = sdl_video.srect.w;
            sdl_video.drect.h = sdl_video.srect.h;

            sdl_video.drect.y = 0;
            sdl_video.drect.x = sdl_video.drect.y = 0;
            gcw0_w = sdl_video.drect.w;
            gcw0_h = sdl_video.drect.h;

//added
            old_srect_y = sdl_video.srect.y;
            old_drect_y = sdl_video.drect.y;
            drawn_from_line = 0;
            sy1 = sy2 = sy3 = sdl_video.srect.y;
            dy1 = dy2 = dy3 = sdl_video.drect.y;
            h1  =  h2 =  h3 = sdl_video.srect.h;
            sdl_video.my_srect.x = sdl_video.srect.x;
            sdl_video.my_srect.y = sdl_video.srect.y;
            sdl_video.my_srect.w = sdl_video.srect.w;
            sdl_video.my_srect.h = sdl_video.srect.h;
            sdl_video.my_drect.x = sdl_video.drect.x;
            sdl_video.my_drect.y = sdl_video.drect.y;
            sdl_video.my_drect.w = sdl_video.drect.w;
            sdl_video.my_drect.h = sdl_video.drect.h;
#ifdef SDL2
#else
            if     (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
            else if(config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
            else if(config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, 16, SDL_SWSURFACE                );
#endif
        } 
    } else 
    {
#ifdef SDL2
#else
            if     (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
            else if(config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
            else if(config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, 16, SDL_SWSURFACE                );
#endif
    }
#ifdef SDL2
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    for(int i = 0; i < 3; i++)
    {
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
#else
    for(int i = 0; i < 3; i++)
    {
        SDL_FillRect(sdl_video.surf_screen, 0, 0);
        if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
        else                    SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
    }
#endif

  //reset semaphore to avoid temporary speedups on menu exit
    if (post)
    {
        do
        {
            SDL_SemWait(sdl_sync.sem_sync);
            --post;
        }
        while(post);
    }
    return 1;
}
#endif
 
int sdl_input_update(void)
{
#ifdef SDL2
//TODO begun but unfinished
#else

#ifdef SDL2
    uint8 *keystate = SDL_GetKeyboardState(NULL);
#else
    uint8 *keystate = SDL_GetKeyState(NULL);
#endif
    /* reset input */
    input.pad[joynum] = 0;
    if (show_lightgun)
        input.pad[4] = 0; //player2:
    switch (input.dev[4])
    {
    case DEVICE_LIGHTGUN:
        show_lightgun = 1;
        /* get mouse coordinates (absolute values) */
        int x,y;
        SDL_GetMouseState(&x,&y);
 
        if (config.gcw0_fullscreen)
        {
            input.analog[4][0] =  x;
            input.analog[4][1] =  y;
        } else
        {
            input.analog[4][0] =  x - (VIDEO_WIDTH  - bitmap.viewport.w) / 2;
            input.analog[4][1] =  y - (VIDEO_HEIGHT - bitmap.viewport.h) / 2;
        } 
        if (config.smsmaskleftbar) x += 8;
        /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
        if (keystate[SDLK_ESCAPE])  input.pad[4] |= INPUT_START;
    default:
        break;
    }
    switch (input.dev[joynum])
    {
    case DEVICE_LIGHTGUN:
    {
#ifdef GCWZERO
        show_lightgun = 2;
        /* get mouse coordinates (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);
 
        if (config.gcw0_fullscreen)
        {
            input.analog[0][0] =  x;
            input.analog[0][1] =  y;
        } else
        {
            input.analog[0][0] =  x - (VIDEO_WIDTH  - bitmap.viewport.w) / 2;
            input.analog[0][1] =  y - (VIDEO_HEIGHT - bitmap.viewport.h) / 2;
        } 
        if (config.smsmaskleftbar) x += 8;
        /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_A;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_C;
        if (keystate[SDLK_ESCAPE])    input.pad[0]      |= INPUT_START;
#else
        /* get mouse coordinates (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);
 
        /* X axis */
        input.analog[joynum][0] =  x - (VIDEO_WIDTH  - bitmap.viewport.w) / 2;
 
        /* Y axis */
        input.analog[joynum][1] =  y - (VIDEO_HEIGHT - bitmap.viewport.h) / 2;
 
        /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_A;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_C;
        if (keystate[SDLK_f])         input.pad[joynum] |= INPUT_START;
        break;
#endif
    }
#ifndef GCWZERO
    case DEVICE_PADDLE:
    {
        /* get mouse (absolute values) */
        int x;
        int state = SDL_GetMouseState(&x, NULL);
 
        /* Range is [0;256], 128 being middle position */
        input.analog[joynum][0] = x * 256 / VIDEO_WIDTH;
 
        /* Button I -> 0 0 0 0 0 0 0 I*/
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
 
        break;
    }
 
    case DEVICE_SPORTSPAD:
    {
        /* get mouse (relative values) */
        int x,y;
        int state = SDL_GetRelativeMouseState(&x,&y);
 
        /* Range is [0;256] */
        input.analog[joynum][0] = (unsigned char)(-x & 0xFF);
        input.analog[joynum][1] = (unsigned char)(-y & 0xFF);
 
        /* Buttons I & II -> 0 0 0 0 0 0 II I*/
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;
 
        break;
    }
 
    case DEVICE_MOUSE:
    {
    SDL_ShowCursor(1);
        /* get mouse (relative values) */
        int x,y;
        int state = SDL_GetRelativeMouseState(&x,&y);
 
        /* Sega Mouse range is [-256;+256] */
        input.analog[joynum][0] = x * 2;
        input.analog[joynum][1] = y * 2;
 
        /* Vertical movement is upsidedown */
        if (!config.invert_mouse)
            input.analog[joynum][1] = 0 - input.analog[joynum][1];
 
        /* Start,Left,Right,Middle buttons -> 0 0 0 0 START MIDDLE RIGHT LEFT */
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;
        if (state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_A;
        if (keystate[SDLK_f])         input.pad[joynum] |= INPUT_START;
 
        break;
    }
 
    case DEVICE_XE_1AP:
    {
        /* A,B,C,D,Select,START,E1,E2 buttons -> E1(?) E2(?) START SELECT(?) A B C D */
        if (keystate[SDLK_a])  input.pad[joynum] |= INPUT_START;
        if (keystate[SDLK_s])  input.pad[joynum] |= INPUT_A;
        if (keystate[SDLK_d])  input.pad[joynum] |= INPUT_C;
        if (keystate[SDLK_f])  input.pad[joynum] |= INPUT_Y;
        if (keystate[SDLK_z])  input.pad[joynum] |= INPUT_B;
        if (keystate[SDLK_x])  input.pad[joynum] |= INPUT_X;
        if (keystate[SDLK_c])  input.pad[joynum] |= INPUT_MODE;
        if (keystate[SDLK_v])  input.pad[joynum] |= INPUT_Z;
 
        /* Left Analog Stick (bidirectional) */
        if      (keystate[SDLK_UP])     input.analog[joynum][1] -=   2;
        else if (keystate[SDLK_DOWN])   input.analog[joynum][1] +=   2;
        else                            input.analog[joynum][1]  = 128;
        if      (keystate[SDLK_LEFT])   input.analog[joynum][0] -=   2;
        else if (keystate[SDLK_RIGHT])  input.analog[joynum][0] +=   2;
        else                            input.analog[joynum][0]  = 128;
 
        /* Right Analog Stick (unidirectional) */
        if      (keystate[SDLK_KP8])    input.analog[joynum + 1][0] -=   2;
        else if (keystate[SDLK_KP2])    input.analog[joynum + 1][0] +=   2;
        else if (keystate[SDLK_KP4])    input.analog[joynum + 1][0] -=   2;
        else if (keystate[SDLK_KP6])    input.analog[joynum + 1][0] +=   2;
        else                            input.analog[joynum + 1][0]  = 128;
 
        /* Limiters */
        if      (input.analog[joynum][0]     > 0xFF) input.analog[joynum][0]     = 0xFF;
        else if (input.analog[joynum][0]     < 0   ) input.analog[joynum][0]     = 0;
        if      (input.analog[joynum][1]     > 0xFF) input.analog[joynum][1]     = 0xFF;
        else if (input.analog[joynum][1]     < 0   ) input.analog[joynum][1]     = 0;
        if      (input.analog[joynum + 1][0] > 0xFF) input.analog[joynum + 1][0] = 0xFF;
        else if (input.analog[joynum + 1][0] < 0   ) input.analog[joynum + 1][0] = 0;
        if      (input.analog[joynum + 1][1] > 0xFF) input.analog[joynum + 1][1] = 0xFF;
        else if (input.analog[joynum + 1][1] < 0   ) input.analog[joynum + 1][1] = 0;
        break;
    }
 
    case DEVICE_PICO:
    {
        /* get mouse (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);
 
        /* Calculate X Y axis values */
        input.analog[0][0] = 0x3c  + (x * (0x17c-0x03c+1)) / VIDEO_WIDTH;
        input.analog[0][1] = 0x1fc + (y * (0x2f7-0x1fc+1)) / VIDEO_HEIGHT;
 
        /* Map mouse buttons to player #1 inputs */
        if (state & SDL_BUTTON_MMASK) pico_current  = (pico_current + 1) & 7;
        if (state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_PICO_RED;
        if (state & SDL_BUTTON_LMASK) input.pad[0] |= INPUT_PICO_PEN;
 
        break;
    }
 
    case DEVICE_TEREBI:
    {
        /* get mouse (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);
 
        /* Calculate X Y axis values */
        input.analog[0][0] = (x * 250) / VIDEO_WIDTH;
        input.analog[0][1] = (y * 250) / VIDEO_HEIGHT;
 
        /* Map mouse buttons to player #1 inputs */
        if (state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_B;
 
        break;
    }
 
    case DEVICE_GRAPHIC_BOARD:
    {
        /* get mouse (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);
 
        /* Calculate X Y axis values */
        input.analog[0][0] = (x * 255) / VIDEO_WIDTH;
        input.analog[0][1] = (y * 255) / VIDEO_HEIGHT;
 
        /* Map mouse buttons to player #1 inputs */
        if (state & SDL_BUTTON_LMASK) input.pad[0] |= INPUT_GRAPHIC_PEN;
        if (state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_GRAPHIC_MENU;
        if (state & SDL_BUTTON_MMASK) input.pad[0] |= INPUT_GRAPHIC_DO;
 
        break;
    }
 
    case DEVICE_ACTIVATOR:
    {
        if (keystate[SDLK_g])  input.pad[joynum] |= INPUT_ACTIVATOR_7L;
        if (keystate[SDLK_h])  input.pad[joynum] |= INPUT_ACTIVATOR_7U;
        if (keystate[SDLK_j])  input.pad[joynum] |= INPUT_ACTIVATOR_8L;
        if (keystate[SDLK_k])  input.pad[joynum] |= INPUT_ACTIVATOR_8U;
    }
#endif
    default:
    {
#ifdef GCWZERO
        if (keystate[config.buttons[A]])     input.pad[joynum] |= INPUT_A;
        if (keystate[config.buttons[B]])     input.pad[joynum] |= INPUT_B;
        if (keystate[config.buttons[C]])     input.pad[joynum] |= INPUT_C;
        if (keystate[config.buttons[START]]) input.pad[joynum] |= INPUT_START;
        if (show_lightgun == 1 || show_lightgun == 2)
        {
            if (keystate[config.buttons[X]]) input.pad[4]      |= INPUT_A; //player 2
            if (keystate[config.buttons[Y]]) input.pad[4]      |= INPUT_B; //player 2
            if (keystate[config.buttons[Z]]) input.pad[4]      |= INPUT_C; //player 2
        } else
        {
            if (keystate[config.buttons[X]]) input.pad[joynum] |= INPUT_X;
            if (keystate[config.buttons[Y]]) input.pad[joynum] |= INPUT_Y;
            if (keystate[config.buttons[Z]]) input.pad[joynum] |= INPUT_Z;
        }
        if (keystate[config.buttons[MODE]])  input.pad[joynum] |= INPUT_MODE;
        if (keystate[SDLK_ESCAPE] && keystate[SDLK_RETURN])
        {
            gotomenu=1;

//TODO      create screenshot now - this is not good practice as it shortens the SD card life slightly but I cannot figure out how to do it from within the menu :(
            if (!config.gcw0_fullscreen)
            {
                char save_state_screenshot[256];
                sprintf(save_state_screenshot,"%s/%s.", get_save_directory(), rom_filename);
                SDL_Surface* save_screenshot;
                save_screenshot = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_video.srect.w, sdl_video.srect.h, 16, 0, 0, 0, 0);
                SDL_Rect temp;
                temp.x = temp.y = 0;
                temp.w = sdl_video.srect.w;
                temp.h = sdl_video.srect.h;

                SDL_BlitSurface(sdl_video.surf_bitmap, &temp, save_screenshot, &temp);
                SDL_SaveBMP(save_screenshot, save_state_screenshot);
                SDL_FreeSurface(save_screenshot);
            }
            else
            {
                char save_state_screenshot[256];
                sprintf(save_state_screenshot,"%s/%s.", get_save_directory(), rom_filename);
                SDL_Surface* save_screenshot;
                save_screenshot = SDL_CreateRGBSurface(SDL_SWSURFACE, gcw0_w, gcw0_h, 16, 0, 0, 0, 0);
                SDL_Rect temp;
                temp.x = temp.y = 0;
                temp.w = gcw0_w;
                temp.h = gcw0_h;

                SDL_BlitSurface(sdl_video.surf_bitmap, &temp, save_screenshot, &temp);
                SDL_SaveBMP(save_screenshot, save_state_screenshot);
                SDL_FreeSurface(save_screenshot);
            }
        }
        if (keystate[SDLK_ESCAPE] && keystate[SDLK_TAB])
        {
          //save to quicksave slot
            char save_state_file[256];
            sprintf(save_state_file,"%s/%s.gp1", get_save_directory(), rom_filename);
                FILE *f = fopen(save_state_file,"wb");
                if (f)
                {
                    uint8 buf[STATE_SIZE];
                    int len = state_save(buf);
                    fwrite(&buf, len, 1, f);
                    fclose(f);
                }
          //Save BMP screenshot
            char save_state_screenshot[256];
            sprintf(save_state_screenshot,"%s/%s.1.bmp", get_save_directory(), rom_filename);
            SDL_Surface* screenshot;
            if (!config.gcw0_fullscreen)
            {
                screenshot = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_video.srect.w, sdl_video.srect.h, 16, 0, 0, 0, 0);
                SDL_Rect temp;
                temp.x = temp.y = 0;
                temp.w = sdl_video.srect.w;
                temp.h = sdl_video.srect.h;

                SDL_BlitSurface(sdl_video.surf_bitmap, &temp, screenshot, &temp);
                SDL_SaveBMP(screenshot, save_state_screenshot);
                SDL_FreeSurface(screenshot);
            }
            else
            {
                screenshot = SDL_CreateRGBSurface(SDL_SWSURFACE, gcw0_w, gcw0_h, 16, 0, 0, 0, 0);
                SDL_Rect temp;
                temp.x = temp.y = 0;
                temp.w = gcw0_w;
                temp.h = gcw0_h;

                SDL_BlitSurface(sdl_video.surf_bitmap, &temp, screenshot, &temp);
                SDL_SaveBMP(screenshot, save_state_screenshot);
                SDL_FreeSurface(screenshot);
            }

            SDL_Delay(250);
        }
        if (keystate[SDLK_ESCAPE] && keystate[SDLK_BACKSPACE])
        {
          //load quicksave slot
            char save_state_file[256];
            sprintf(save_state_file,"%s/%s.gp1", get_save_directory(), rom_filename );
            FILE *f = fopen(save_state_file,"rb");
            if (f)
            {
                uint8 buf[STATE_SIZE];
                fread(&buf, STATE_SIZE, 1, f);
                state_load(buf);
                fclose(f);
            }
            SDL_Delay(250);

        }
#else
        if (keystate[SDLK_a])  input.pad[joynum] |= INPUT_A;
        if (keystate[SDLK_s])  input.pad[joynum] |= INPUT_B;
        if (keystate[SDLK_d])  input.pad[joynum] |= INPUT_C;
        if (keystate[SDLK_f])  input.pad[joynum] |= INPUT_START;
        if (keystate[SDLK_z])  input.pad[joynum] |= INPUT_X;
        if (keystate[SDLK_x])  input.pad[joynum] |= INPUT_Y;
        if (keystate[SDLK_c])  input.pad[joynum] |= INPUT_Z;
        if (keystate[SDLK_v])  input.pad[joynum] |= INPUT_MODE;
#endif

#ifdef GCWZERO //A-stick support
        static int MoveLeft  = 0;
        static int MoveRight = 0;
        static int MoveUp    = 0;
        static int MoveDown  = 0;
        Sint32     x_move    = 0;
        Sint32     y_move    = 0;
        static int lg_left   = 0;
        static int lg_right  = 0;
        static int lg_up     = 0;
        static int lg_down   = 0;
        SDL_Joystick* joy;
        if (SDL_NumJoysticks() > 0)
        {
            joy    = SDL_JoystickOpen(0);
            x_move = SDL_JoystickGetAxis(joy, 0);
            y_move = SDL_JoystickGetAxis(joy, 1);
        }

      //Define cetral deadzone of analogue joystick
        int deadzone = config.deadzone;
        deadzone *= 5000;

       //Control lightgun with A-stick if activated
        if (show_lightgun)
        {
            lg_left = lg_right = lg_up = lg_down = 0;

            if (x_move < -deadzone || x_move > deadzone)
            {
                if (x_move < -deadzone) lg_left  = 1;
                if (x_move < -20000)    lg_left  = 3;
                if (x_move >  deadzone) lg_right = 1;
                if (x_move >  20000)    lg_right = 3;
                current_time = time(NULL); //cursor disappears after 3 seconds...
            }
            if (y_move < -deadzone || y_move > deadzone)
            {
                if (y_move < -deadzone) lg_up   = 1;
                if (y_move < -20000)    lg_up   = 3;
                if (y_move >  deadzone) lg_down = 1;
                if (y_move >  20000)    lg_down = 3;
                current_time = time(NULL);
            }
          //Keep mouse within screen, wrap around!
            int x,y;
            SDL_GetMouseState(&x,&y);
            if (!config.gcw0_fullscreen)
            {
                if ((x - lg_left ) < sdl_video.drect.x )               x = VIDEO_WIDTH  - sdl_video.drect.x;
                if ((y - lg_up   ) < sdl_video.drect.y )               y = VIDEO_HEIGHT - sdl_video.drect.y;
                if ((x + lg_right) > VIDEO_WIDTH  - sdl_video.drect.x) x = sdl_video.drect.x;
                if ((y + lg_down ) > VIDEO_HEIGHT - sdl_video.drect.y) y = sdl_video.drect.y;
            } else //scaling on
            {
                if ((x - lg_left) < 0)       x = gcw0_w;
                if ((y - lg_up  ) < 0)       y = gcw0_h;
                if ((x + lg_right) > gcw0_w) x = 0;
                if ((y + lg_down ) > gcw0_h) y = 0;
            }
#ifdef SDL2
//TODO
//          SDL_WarpMouseInWindow( window, ( x+ ( ( lg_right - lg_left ) * config.lightgun_speed ) ) ,
//                               ( y+ ( ( lg_down  - lg_up   ) * config.lightgun_speed ) ) );
#else
            SDL_WarpMouse((x + ((lg_right - lg_left) * config.lightgun_speed)),
                          (y + ((lg_down  - lg_up  ) * config.lightgun_speed)));
#endif

        } else
      //otherwise it's just mirroring the D-pad controls
        if (config.a_stick)
        {
            int deadzone = config.deadzone;
            deadzone *= 5000;
            MoveLeft = MoveRight = MoveUp = MoveDown = 0;

            if (x_move < -deadzone) MoveLeft  = 1;
            if (x_move >  deadzone) MoveRight = 1;
            if (y_move < -deadzone) MoveUp    = 1;
            if (y_move >  deadzone) MoveDown  = 1;
        }
        if (show_lightgun == 1) //Genesis/MD D-pad controls player 2
        {
            if (MoveUp              )  input.pad[4]      |= INPUT_UP;
            if (MoveDown            )  input.pad[4]      |= INPUT_DOWN;
            if (MoveLeft            )  input.pad[4]      |= INPUT_LEFT;
            if (MoveRight           )  input.pad[4]      |= INPUT_RIGHT;
            if (keystate[SDLK_UP]   )  input.pad[joynum] |= INPUT_UP;
            if (keystate[SDLK_DOWN] )  input.pad[joynum] |= INPUT_DOWN;
            if (keystate[SDLK_LEFT] )  input.pad[joynum] |= INPUT_LEFT;
            if (keystate[SDLK_RIGHT])  input.pad[joynum] |= INPUT_RIGHT;
        } else
        if (show_lightgun == 2) //SMS D-pad controls player 2
        {
            if (MoveUp              )  input.pad[joynum] |= INPUT_UP;
            if (MoveDown            )  input.pad[joynum] |= INPUT_DOWN;
            if (MoveLeft            )  input.pad[joynum] |= INPUT_LEFT;
            if (MoveRight           )  input.pad[joynum] |= INPUT_RIGHT;
            if (keystate[SDLK_UP]   )  input.pad[4]      |= INPUT_UP;
            if (keystate[SDLK_DOWN] )  input.pad[4]      |= INPUT_DOWN;
            if (keystate[SDLK_LEFT] )  input.pad[4]      |= INPUT_LEFT;
            if (keystate[SDLK_RIGHT])  input.pad[4]      |= INPUT_RIGHT;
        } else
        {
#ifdef SDL2
//TODO
#else
            if      (keystate[SDLK_UP]    || MoveUp   )  input.pad[joynum] |= INPUT_UP;
            else if (keystate[SDLK_DOWN]  || MoveDown )  input.pad[joynum] |= INPUT_DOWN;
            if      (keystate[SDLK_LEFT]  || MoveLeft )  input.pad[joynum] |= INPUT_LEFT;
            else if (keystate[SDLK_RIGHT] || MoveRight)  input.pad[joynum] |= INPUT_RIGHT;
#endif
        }
#else
        if      (keystate[SDLK_UP]   )  input.pad[joynum] |= INPUT_UP;
        else if (keystate[SDLK_DOWN] )  input.pad[joynum] |= INPUT_DOWN;
        if      (keystate[SDLK_LEFT] )  input.pad[joynum] |= INPUT_LEFT;
        else if (keystate[SDLK_RIGHT])  input.pad[joynum] |= INPUT_RIGHT;
#endif 
        }
    }
#endif //SDL2 skip this for now.
    return 1;
}
 
int main (int argc, char **argv)
{
    FILE *fp;
    int running = 1;
    atexit(shutdown);
    /* Print help if no game specified */
    if (argc < 2)
    {
        char caption[256];
        sprintf(caption, "Genesis Plus GX\\SDL\nusage: %s gamename\n", argv[0]);
        MessageBox(NULL, caption, "Information", 0);
        exit(1);
    }
 
    error_init();
    create_default_directories();

    /* set default config */
    set_config_defaults();
    
    /* using rom file name instead of crc code to save files */
    sprintf(rom_filename, "%s",  get_file_name(argv[1]));
 
    /* mark all BIOS as unloaded */
    system_bios = 0;
 
    /* Genesis BOOT ROM support (2KB max) */
    memset(boot_rom, 0xFF, 0x800);
    fp = fopen(MD_BIOS, "rb");
    if (fp != NULL)
    {
        int i;
 
        /* read BOOT ROM */
        fread(boot_rom, 1, 0x800, fp);
        fclose(fp);
 
        /* check BOOT ROM */
        if (!memcmp((char *)(boot_rom + 0x120),"GENESIS OS", 10))
        {
            /* mark Genesis BIOS as loaded */
            system_bios = SYSTEM_MD;
        }
 
        /* Byteswap ROM */
        for (i=0; i<0x800; i+=2)
        {
            uint8 temp = boot_rom[i];
            boot_rom[i] = boot_rom[i+1];
            boot_rom[i+1] = temp;
        }
    }
 
    /* Load game file */
    if (!load_rom(argv[1]))
    {
        char caption[256];
        sprintf(caption, "Error loading file `%s'.", argv[1]);
        MessageBox(NULL, caption, "Error", 0);
        exit(1);
    }

    /* Per-game configuration (speed hacks) */
    if (strstr(rominfo.international,"Virtua Racing"))
    {
        virtua_racing         = 1; //further speed hacks required
        frameskip             = 4;
        config.hq_fm          = 0;
        config.psgBoostNoise  = 0;
        config.filter         = 0;
//        config.dac_bits       = 9;
        config.dac_bits       = 7;
    }
    else if (system_hw == SYSTEM_MCD)
    {
        frameskip             = 4;
        config.hq_fm          = 0;
        config.psgBoostNoise  = 0;
//        config.filter         = 1;
        config.filter         = 0;
//        config.dac_bits       = 9;
        config.dac_bits       = 7;
    }
    else
    {
        config.hq_fm          = 1;
        config.psgBoostNoise  = 1;
        config.filter         = 1;
        config.dac_bits       = 14;
    }

    /* initialize SDL */
    if (SDL_Init(0) < 0)
    {
        char caption[256];
        sprintf(caption, "SDL initialization failed");
        MessageBox(NULL, caption, "Error", 0);
        exit(1);
    }
#ifdef GCWZERO
    sdl_joystick_init();
#endif
    sdl_video_init();
    if (use_sound) sdl_sound_init();
    sdl_sync_init();
 


    /* initialize Genesis virtual system */
    SDL_LockSurface(sdl_video.surf_bitmap);
    memset(&bitmap, 0, sizeof(t_bitmap));
    bitmap.width        = 320;
    bitmap.height       = 240;
#if defined(USE_8BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 1);
#elif defined(USE_15BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 2);
#elif defined(USE_16BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 2);
#elif defined(USE_32BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 4);
#endif
    bitmap.data         = sdl_video.surf_bitmap->pixels;
    SDL_UnlockSurface(sdl_video.surf_bitmap);
    bitmap.viewport.changed = 3;
 
 

    /* initialize system hardware */

    if (strstr(rominfo.international,"Virtua Racing"))
        audio_init(SOUND_FREQUENCY / 4, (vdp_pal ? 50 : 60));
    else if (system_hw == SYSTEM_MCD)
        audio_init(SOUND_FREQUENCY,     (vdp_pal ? 50 : 60));
    else
        audio_init(SOUND_FREQUENCY, 0);
    system_init();
 
    /* Mega CD specific */
    char brm_file[256];
    if (system_hw == SYSTEM_MCD)
    {
        /* load internal backup RAM */
        sprintf(brm_file,"%s/%s", get_save_directory(), "scd.brm");
        fp = fopen(brm_file, "rb");
        if (fp!=NULL)
        {
            fread(scd.bram, 0x2000, 1, fp);
            fclose(fp);
        }
 
        /* check if internal backup RAM is formatted */
        if (memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20))
        {
            /* clear internal backup RAM */
            memset(scd.bram, 0x00, 0x200);
 
            /* Internal Backup RAM size fields */
            brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = 0x00;
            brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (sizeof(scd.bram) / 64) - 3;
 
            /* format internal backup RAM */
            memcpy(scd.bram + 0x2000 - 0x40, brm_format, 0x40);
        }
 
        /* load cartridge backup RAM */
        if (scd.cartridge.id)
        {
            sprintf(brm_file,"%s/%s", get_save_directory(), "cart.brm");
            fp = fopen(brm_file, "rb");
            if (fp!=NULL)
            {
                fread(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
                fclose(fp);
            }
 
            /* check if cartridge backup RAM is formatted */
            if (memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20))
            {
                /* clear cartridge backup RAM */
                memset(scd.cartridge.area, 0x00, scd.cartridge.mask + 1);
 
                /* Cartridge Backup RAM size fields */
                brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = (((scd.cartridge.mask + 1) / 64) - 3) >> 8;
                brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (((scd.cartridge.mask + 1) / 64) - 3) & 0xff;
 
                /* format cartridge backup RAM */
                memcpy(scd.cartridge.area + scd.cartridge.mask + 1 - sizeof(brm_format), brm_format, sizeof(brm_format));
            }
        }
    }
 
    if (sram.on)
    {
        /* load SRAM */
        char save_file[256];
        sprintf(save_file,"%s/%s.srm", get_save_directory(), rom_filename);
        fp = fopen(save_file, "rb");
        if (fp!=NULL)
        {
            fread(sram.sram,0x10000,1, fp);
            fclose(fp);
        }
    }
 
    /* reset system hardware */
    system_reset();
 
    if (use_sound) SDL_PauseAudio(0);
 
    /* 3 frames = 50 ms (60hz) or 60 ms (50hz) */
    if (sdl_sync.sem_sync)
        SDL_AddTimer(vdp_pal ? 60 : 50, sdl_sync_timer_callback, (void *) 1);

    /* emulation loop */
    while(running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    running = 0;
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_HOME) //Powerslider up
                        gotomenu = 1;
                default:
                    break;
            }
        }

#ifdef GCWZERO
        if (do_once) 
        {
            do_once = 0; //don't waste write cycles!
            if (config.keepaspectratio)
            {
                FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
                if (aspect_ratio_file)
                { 
                    fwrite("Y", 1, 1, aspect_ratio_file);
                    fclose(aspect_ratio_file);
                }
            }
            if (!config.keepaspectratio)
    	    {
                FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
                if (aspect_ratio_file)
                { 
                    fwrite("N", 1, 1, aspect_ratio_file);
                    fclose(aspect_ratio_file);
                }
            }
	}

        /* SMS automask leftbar if screen mode changes (eg Alex Kidd in Miracle World) */
        static int smsmaskleftbar = 0;
        if (config.smsmaskleftbar != smsmaskleftbar)
        {
            /* force screen change */
            bitmap.viewport.changed = 1;
            smsmaskleftbar = config.smsmaskleftbar;
        }
#endif

        sdl_video_update();
        sdl_sound_update(use_sound);

        if (!sdl_video.frames_rendered)
        {
            if (system_hw == SYSTEM_MCD || virtua_racing)
            {
                if (config.optimisations)
                    calc_framerate(1);
                else
                {
                    if(turbo_mode || frameskip) turbo_mode = frameskip = 0;
//                    if(calc_framerate(0) > 50) turbo_mode= 1; else turbo_mode = 0;
                    if(gcwzero_cycles != 3420) gcwzero_cycles = 3420;
                    if(SVP_cycles != 800) SVP_cycles = 800;
                }
            }

            if (!gotomenu)
            {
                if (!turbo_mode) //aggresive speed optimisations toggle this disabling the semaphore wait.
                {
                    SDL_SemWait(sdl_sync.sem_sync);
                    if (post)    --post;
                    if (post)
                    {
                        do
                        {
                            SDL_SemWait(sdl_sync.sem_sync);
                            --post;
                        }
                        while(post);
                    }
                }
            }
            else    gcw0menu();
        }
    }
    return 0;
}

