#include <sys/stat.h>
#include <dirent.h>

#include "utils.h"
#include "osd.h"

void create_default_directories(void) {
    const char *pathname_list [21] = 
    {
    /* base directory */
        "%s%s",
    /* default SRAM & Savestate files directories */ 
        "%s%s/saves",
        "%s%s/saves/md",
        "%s%s/saves/ms",
        "%s%s/saves/gg",
        "%s%s/saves/sg",
        "%s%s/saves/cd",
    /* default Snapshot files directories */ 
        "%s%s/snaps",
        "%s%s/snaps/md",
        "%s%s/snaps/ms",
        "%s%s/snaps/gg",
        "%s%s/snaps/sg",
        "%s%s/snaps/cd",
    /* default Cheat files directories */ 
        "%s%s/cheats",
        "%s%s/cheats/md",
        "%s%s/cheats/ms",
        "%s%s/cheats/gg",
        "%s%s/cheats/sg",
        "%s%s/cheats/cd",
    /* default BIOS ROM files directories */ 
        "%s%s/bios",
    /* default LOCK-ON ROM files directories */ 
        "%s%s/lock-on"
    };
    const char *homedir;
    char pathname[MAXPATHLEN];
    DIR *dir;
    int i;

    if ((homedir = getenv("HOME")) == NULL)
        homedir = getpwuid(getuid())->pw_dir;
    for(i = 0; i < 21; i++)
    {
        sprintf (pathname, pathname_list[i], homedir, DEFAULT_PATH);
        dir = opendir(pathname);
        if (dir) closedir(dir);
        else mkdir(pathname, S_IRWXU);
    }
}

char* get_save_directory(void) {
	const char *homedir;
	const char *system_dir;
    char* pathname=malloc(MAXPATHLEN);

    if ((homedir = getenv("HOME")) == NULL)
        homedir = getpwuid(getuid())->pw_dir;
  
    if      (system_hw <= SYSTEM_MARKIII                            ) system_dir = "/saves/sg";
    else if (system_hw  > SYSTEM_MARKIII && system_hw <= SYSTEM_SMS2) system_dir = "/saves/ms";
    else if (system_hw  > SYSTEM_SMS2    && system_hw <= SYSTEM_GGMS) system_dir = "/saves/gg";
    else if (system_hw == SYSTEM_MD                                 ) system_dir = "/saves/md";
    else if (system_hw == SYSTEM_MCD                                ) system_dir = "/saves/cd";
    else		                                                      system_dir = "/saves/";

    sprintf (pathname, "%s%s%s", homedir, DEFAULT_PATH, system_dir);
    
    return pathname;
	
}

char* gcw0_get_key_name(int keycode)
{
    switch(keycode)
    {
    case SDLK_UP:        return "Up";
    case SDLK_DOWN:      return "Down";
    case SDLK_LEFT:      return "Left";
    case SDLK_RIGHT:     return "Right";
    case SDLK_LCTRL:     return "A";
    case SDLK_LALT:      return "B";
    case SDLK_LSHIFT:    return "X";
    case SDLK_SPACE:     return "Y";
    case SDLK_TAB:       return "L";
    case SDLK_BACKSPACE: return "R";
    case SDLK_RETURN:    return "Start";
    case SDLK_ESCAPE:    return "Select";
    default:             return "...";
    }
}

char *get_file_name(char *full_path)
{
    char* file_name=malloc(256);
    sprintf(file_name, "%s", basename(full_path));
	
    /* remove file extension */
    int i = strlen(file_name) - 1;
    while ((i > 0) && (file_name[i] != '.')) i--;
    if (i > 0) file_name[i] = 0;
	
    return file_name;
}

