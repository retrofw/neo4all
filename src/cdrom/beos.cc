#include <string.h>
#include <SDL.h>
#include <be/storage/Volume.h>
#include <be/storage/VolumeRoster.h>
#include <be/kernel/fs_info.h>

extern "C" {

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
	int32 dev;	
	fs_info fs_info;
	
	int j=0;
	char name[B_FILE_NAME_LENGTH];
	char buffer[B_FILE_NAME_LENGTH];	
		
	BVolume *volume=new BVolume();
	BVolumeRoster *volumeroster=new BVolumeRoster();	
	
	/* get device path for cd drive */
	strcpy(name,SDL_CDName(drive));
	
	/* trim file from device path */
	while(name[j]) j++;
	j--;
	while(name[j] && name[j]!='/') j--;
	name[j]=0;
	
		
	/* get mount path */
	buffer[0]='/';	
	volumeroster->Rewind();
	while (volumeroster->GetNextVolume(volume)!=B_BAD_VALUE) {
		volume->GetName(&buffer[1]);
		if (buffer[1]=='/') buffer[1]=':';
		
		dev=dev_for_path(buffer);
		fs_stat_dev(dev,&fs_info);
		
		if (!strncmp(fs_info.device_name,name,strlen(name))) {
		    strcpy(mountpoint,buffer);
			strcat(mountpoint,"/");
			volume->~BVolume();
			volumeroster->~BVolumeRoster();
			return 1;
		}
	}
	volume->~BVolume();
	volumeroster->~BVolumeRoster();
	return 0;				
}

}  /* extern "C" */

