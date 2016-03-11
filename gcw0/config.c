#include "osd.h"

t_config config;
t_config configTemp;

static int config_load(void)
{
    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL) homedir = getpwuid(getuid())->pw_dir;

    /* open configuration file */
    char fname[MAXPATHLEN];
    sprintf (fname, "%s%s/config.ini", homedir, DEFAULT_PATH);
    FILE *fp = fopen(fname, "rb");
    if (fp)
    {
        /* check file size */
        fseek(fp, 0, SEEK_END);
        if (ftell(fp) != sizeof(config))
        {
            fclose(fp);
            return 0;
        }

        /* read file */
        fseek(fp, 0, SEEK_SET);
        fread(&config, sizeof(config), 1, fp);
        fclose(fp);
        return 1;
    }
    return 0;
}


void set_config_defaults(void)
{
    int i;

    /* sound options */
    config.use_sound       = 1; /* 0 = OFF, 1 = ON */
    config.skip_prevention = 1; /* 0 = OFF, 1 = ON */
    config.ym2413          = 1; /* = AUTO (0 = always OFF, 1 = always ON) */
    config.psg_preamp      = 150;
    config.fm_preamp       = 100;
    config.hq_fm           = 1;
    config.psgBoostNoise   = 1;
    config.filter          = 2;
    config.low_freq        = 200;
    config.high_freq       = 8000;
    config.lg              = 1.0;
    config.mg              = 1.0;
    config.hg              = 1.0;
    config.lp_range        = 0x9999; /* 0.6 in 16.16 fixed point */
    config.dac_bits        = 14;
    config.mono            = 0;

    /* system options */
    config.system         = 0; /* = AUTO (or SYSTEM_SG, SYSTEM_MARKIII, SYSTEM_SMS, SYSTEM_SMS2, SYSTEM_GG, SYSTEM_MD) */
    config.region_detect  = 0; /* = AUTO (1 = USA, 2 = EUROPE, 3 = JAPAN/NTSC, 4 = JAPAN/PAL) */
    config.vdp_mode       = 0; /* = AUTO (1 = NTSC, 2 = PAL) */
    config.master_clock   = 0; /* = AUTO (1 = NTSC, 2 = PAL) */
    config.force_dtack    = 0;
    config.addr_error     = 1;
    config.bios           = 0;
    config.lock_on        = 0; /* = OFF (can be TYPE_SK, TYPE_GG & TYPE_AR) */
    config.ntsc           = 0;
    config.lcd            = 0; /* 0.8 fixed point */

    /* display options */
    config.overscan         = 0;    /* 3 = all borders (0 = no borders , 1 = vertical borders only, 2 = horizontal borders only) */
    config.gg_extra         = 0;    /* 1 = show extended Game Gear screen (256x192) */
    config.render           = 0;    /* 1 = double resolution output (only when interlaced mode 2 is enabled) */
    config.gcw0_fullscreen  = 1;    /* 1 = use IPU scaling */
    config.keepaspectratio  = 1;    /* 1 = aspect ratio correct with black bars, 0 = fullscreen without correct aspect ratio */
    config.gg_scanlines     = 1;    /* 1 = use scanlines on Game Gear */
    config.smsmaskleftbar   = 1;    /* 1 = Mask left bar on SMS (better for horizontal scrolling) */
    config.sl_autoresume    = 1;    /* 1 = Automatically resume when saving and loading snapshots */
    config.a_stick          = 0;    /* 1 = A-Stick on */
    config.lightgun_speed   = 1;    /* 1 = simple speed multiplier for lightgun */
    config.optimisations    = 0;    /* 0 = Off, 1 = On(Conservative), 2 = On(Performance). Only affects MCD games and Virtua Racing */
    config.deadzone         = 2;    /* Analogue joystick deadzone. Lower values are more sensitive but prone to accidental movement */
    config.renderer         = 0;    /* 0 = Triple buffering, 1 = Double buffering, 2 = Software rendering */

    /* controllers options */
    config.cursor         = 0;  /* different cursor designs */
    input.system[0]       = SYSTEM_GAMEPAD;
    input.system[1]       = SYSTEM_GAMEPAD;
    config.gun_cursor[0]  = 1;
    config.gun_cursor[1]  = 1;
    config.invert_mouse   = 0;
    for (i=0; i<MAX_INPUTS; i++)
    {
        /* autodetect control pad type */
        config.input[i].padtype = DEVICE_PAD2B | DEVICE_PAD3B | DEVICE_PAD6B;
    }
    config.buttons[A]     = SDLK_LSHIFT;   //x
    config.buttons[B]     = SDLK_LALT;     //b
    config.buttons[C]     = SDLK_LCTRL;    //a
    config.buttons[X]     = SDLK_TAB;      //l
    config.buttons[Y]     = SDLK_SPACE;    //y
    config.buttons[Z]     = SDLK_BACKSPACE;//r
    config.buttons[START] = SDLK_RETURN;   //start
    config.buttons[MODE]  = SDLK_ESCAPE;   //select

    /* try to restore user config */
    int loaded = config_load();
    if (!loaded) printf("Default Settings restored\n");
}


void config_save(void)
{
  int saveConfig = NULL;

  /* Get home directory */
  const char *homedir;
  if ((homedir = getenv("HOME")) == NULL) homedir = getpwuid(getuid())->pw_dir;

  /* Check if config settings have changed */
  if(config.use_sound       != configTemp.use_sound      ) { config.use_sound       = configTemp.use_sound      ; saveConfig = 1; }
  if(config.skip_prevention != configTemp.skip_prevention) { config.skip_prevention = configTemp.skip_prevention; saveConfig = 1; }
  if(config.ym2413          != configTemp.ym2413         ) { config.ym2413          = configTemp.ym2413         ; saveConfig = 1; }
  if(config.psg_preamp      != configTemp.psg_preamp     ) { config.psg_preamp      = configTemp.psg_preamp     ; saveConfig = 1; }
  if(config.fm_preamp       != configTemp.fm_preamp      ) { config.fm_preamp       = configTemp.fm_preamp      ; saveConfig = 1; }
  if(config.hq_fm           != configTemp.hq_fm          ) { config.hq_fm           = configTemp.hq_fm          ; saveConfig = 1; }
  if(config.psgBoostNoise   != configTemp.psgBoostNoise  ) { config.psgBoostNoise   = configTemp.psgBoostNoise  ; saveConfig = 1; }
  if(config.filter          != configTemp.filter         ) { config.filter          = configTemp.filter         ; saveConfig = 1; }
  if(config.low_freq        != configTemp.low_freq       ) { config.low_freq        = configTemp.low_freq       ; saveConfig = 1; }
  if(config.high_freq       != configTemp.high_freq      ) { config.high_freq       = configTemp.high_freq      ; saveConfig = 1; }
  if(config.lg              != configTemp.lg             ) { config.lg              = configTemp.lg             ; saveConfig = 1; }
  if(config.mg              != configTemp.mg             ) { config.mg              = configTemp.mg             ; saveConfig = 1; }
  if(config.hg              != configTemp.hg             ) { config.hg              = configTemp.hg             ; saveConfig = 1; }
  if(config.lp_range        != configTemp.lp_range       ) { config.lp_range        = configTemp.lp_range       ; saveConfig = 1; }
  if(config.dac_bits        != configTemp.dac_bits       ) { config.dac_bits        = configTemp.dac_bits       ; saveConfig = 1; }
  if(config.mono            != configTemp.mono           ) { config.mono            = configTemp.mono           ; saveConfig = 1; }
  if(config.system          != configTemp.system         ) { config.system          = configTemp.system         ; saveConfig = 1; }
  if(config.region_detect   != configTemp.region_detect  ) { config.region_detect   = configTemp.region_detect  ; saveConfig = 1; }
  if(config.vdp_mode        != configTemp.vdp_mode       ) { config.vdp_mode        = configTemp.vdp_mode       ; saveConfig = 1; }
  if(config.master_clock    != configTemp.master_clock   ) { config.master_clock    = configTemp.master_clock   ; saveConfig = 1; }
  if(config.addr_error      != configTemp.addr_error     ) { config.addr_error      = configTemp.addr_error     ; saveConfig = 1; }
  if(config.bios            != configTemp.bios           ) { config.bios            = configTemp.bios           ; saveConfig = 1; }
  if(config.lock_on         != configTemp.lock_on        ) { config.lock_on         = configTemp.lock_on        ; saveConfig = 1; }
  if(config.ntsc            != configTemp.ntsc           ) { config.ntsc            = configTemp.ntsc           ; saveConfig = 1; }
  if(config.lcd             != configTemp.lcd            ) { config.lcd             = configTemp.lcd            ; saveConfig = 1; }
  if(config.overscan        != configTemp.overscan       ) { config.overscan        = configTemp.overscan       ; saveConfig = 1; }
  if(config.gg_extra        != configTemp.gg_extra       ) { config.gg_extra        = configTemp.gg_extra       ; saveConfig = 1; }
  if(config.render          != configTemp.render         ) { config.render          = configTemp.render         ; saveConfig = 1; }
  if(config.gcw0_fullscreen != configTemp.gcw0_fullscreen) { config.gcw0_fullscreen = configTemp.gcw0_fullscreen; saveConfig = 1; }
  if(config.keepaspectratio != configTemp.keepaspectratio) { config.keepaspectratio = configTemp.keepaspectratio; saveConfig = 1; }
  if(config.gg_scanlines    != configTemp.gg_scanlines   ) { config.gg_scanlines    = configTemp.gg_scanlines   ; saveConfig = 1; }
  if(config.smsmaskleftbar  != configTemp.smsmaskleftbar ) { config.smsmaskleftbar  = configTemp.smsmaskleftbar ; saveConfig = 1; }
  if(config.sl_autoresume   != configTemp.sl_autoresume  ) { config.sl_autoresume   = configTemp.sl_autoresume  ; saveConfig = 1; }
  if(config.a_stick         != configTemp.a_stick        ) { config.a_stick         = configTemp.a_stick        ; saveConfig = 1; }
  if(config.lightgun_speed  != configTemp.lightgun_speed ) { config.lightgun_speed  = configTemp.lightgun_speed ; saveConfig = 1; }
  if(config.optimisations   != configTemp.optimisations  ) { config.optimisations   = configTemp.optimisations  ; saveConfig = 1; }
  if(config.deadzone        != configTemp.deadzone       ) { config.deadzone        = configTemp.deadzone       ; saveConfig = 1; }
  if(config.renderer        != configTemp.renderer       ) { config.renderer        = configTemp.renderer       ; saveConfig = 1; }
  if(config.cursor          != configTemp.cursor         ) { config.cursor          = configTemp.cursor         ; saveConfig = 1; }
  if(config.gun_cursor[0]   != configTemp.gun_cursor[0]  ) { config.gun_cursor[0]   = configTemp.gun_cursor[0]  ; saveConfig = 1; }
  if(config.gun_cursor[1]   != configTemp.gun_cursor[1]  ) { config.gun_cursor[1]   = configTemp.gun_cursor[1]  ; saveConfig = 1; }
  if(config.invert_mouse    != configTemp.invert_mouse   ) { config.invert_mouse    = configTemp.invert_mouse   ; saveConfig = 1; }
  if(config.buttons[A]      != configTemp.buttons[A]     ) { config.buttons[A]      = configTemp.buttons[A]     ; saveConfig = 1; }
  if(config.buttons[B]      != configTemp.buttons[B]     ) { config.buttons[B]      = configTemp.buttons[B]     ; saveConfig = 1; }
  if(config.buttons[C]      != configTemp.buttons[C]     ) { config.buttons[C]      = configTemp.buttons[C]     ; saveConfig = 1; }
  if(config.buttons[X]      != configTemp.buttons[X]     ) { config.buttons[X]      = configTemp.buttons[X]     ; saveConfig = 1; }
  if(config.buttons[Y]      != configTemp.buttons[Y]     ) { config.buttons[Y]      = configTemp.buttons[Y]     ; saveConfig = 1; }
  if(config.buttons[Z]      != configTemp.buttons[Z]     ) { config.buttons[Z]      = configTemp.buttons[Z]     ; saveConfig = 1; }
  if(config.buttons[START]  != configTemp.buttons[START] ) { config.buttons[START]  = configTemp.buttons[START] ; saveConfig = 1; }
  if(config.buttons[MODE]   != configTemp.buttons[MODE]  ) { config.buttons[MODE]   = configTemp.buttons[MODE]  ; saveConfig = 1; }

  if(saveConfig)
  {
    /* Open configuration file */
    char fname[MAXPATHLEN];
    sprintf (fname, "%s%s/config.ini", homedir, DEFAULT_PATH);
    FILE *fp = fopen(fname, "wb");
    if (fp)
    {
      /* write file */
      fwrite(&configTemp, sizeof(configTemp), 1, fp);
      fclose(fp);
    }
  }
}

void save_current_config(void)
{
    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL) homedir = getpwuid(getuid())->pw_dir;

    /* open configuration file */
    char fname[MAXPATHLEN];
    sprintf (fname, "%s%s/config.ini", homedir, DEFAULT_PATH);
    FILE *fp = fopen(fname, "rb");
    if (fp)
    {
        /* check file size */
        fseek(fp, 0, SEEK_END);
        if (ftell(fp) != sizeof(config)) fclose(fp);

        /* read file */
        fseek(fp, 0, SEEK_SET);
        fread(&configTemp, sizeof(configTemp), 1, fp);
        fclose(fp);
    }
}
