#include <string.h>
#include <SDL.h>

/**
 * getMountPoint (int drive, char *mountpoint)
 *
 * gets the mount point for numbered SDL CD drive.
 * mount path copied into mountpoint - buffer must be large enough to receive path.
 *
 * The mount point must end in a directory separator so that a file name may
 * be directly concatenated onto its end.
 *
 * returns 1 if successful
 *         0 if error
 *
 */
int getMountPoint(int drive, char *mountpoint)
{
    strcpy(mountpoint,SDL_CDName(drive));
    
    return 1;
}

