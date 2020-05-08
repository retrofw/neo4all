#include <string.h>
#include <SDL.h>
#include <mntent.h>
#include <sys/stat.h>

#define MOUNT_TAB "/etc/mtab"


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
// #ifdef DEBUG_MEMORY
#if 1
strcpy(mountpoint,"/tmp/cdrom/");
mkdir(mountpoint, 0777);
return 1;
#else
    FILE *mtab;
    struct mntent *mntent;
    const char* name=SDL_CDName(drive);

    mtab=setmntent(MOUNT_TAB,"r");

    if (mtab != NULL) {
        while ((mntent=getmntent(mtab)) != NULL) {
            if (strcmp(name,mntent->mnt_fsname) == 0) {
                strcpy(mountpoint,mntent->mnt_dir);
                strcat(mountpoint,"/");
                endmntent(mtab);
                return 1;
            }
        }
        endmntent(mtab);
    }
    return 0;
#endif
}

