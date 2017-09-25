/** 
 * gdiopt.c 
 * Copyright (c) 2014-2015 SWAT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

FILE *logfd = NULL;

int bin2iso(const char *source, const char *target) 
{
    int   seek_header, seek_ecc, sector_size;
    long  i, source_length;
    char  buf[2352];
    const unsigned char SYNC_HEADER[12] = 
        {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
    
    FILE *fpSource, *fpTarget;

    fpSource = fopen(source, "rb");
    fpTarget = fopen(target, "wb");
    
    if ((fpSource==NULL) || (fpTarget==NULL)) 
    {
        return -1;
    }
    
    fread(buf, sizeof(char), 16, fpSource);

    if (memcmp(SYNC_HEADER, buf, 12))
    {
        seek_header = 8;        
        seek_ecc = 280;
        sector_size = 2336;
    }
    else        
    {
        switch(buf[15])
        {
            case 2:
            {    
                seek_header = 24;    // Mode2/2352    
                seek_ecc = 280;
                sector_size = 2352;
                break;
            }

            case 1:
            {
                seek_header = 16;    // Mode1/2352
                seek_ecc = 288;
                sector_size = 2352;
                break;
            }

            default:
            {
                fclose(fpTarget);
                fclose(fpSource);
                return -1;
            }
        }
    }

    fseek(fpSource, 0L, SEEK_END);
    source_length = ftell(fpSource)/sector_size;
    fseek(fpSource, 0L, SEEK_SET);

    for(i=0; i<source_length; i++)
    {
		fseek(fpSource, seek_header, SEEK_CUR);
		fread(buf, sizeof(char), 2048, fpSource);  
		fwrite(buf, sizeof(char), 2048, fpTarget);
		fseek(fpSource, seek_ecc, SEEK_CUR);
    }

    fclose(fpTarget);
    fclose(fpSource);

    return 0;
}

int convert_gdi(char *ingdi, char *outgdi, const char *folder) 
{
	FILE *fr, *fw;
	int i, rc, track_no, track_count;
	unsigned long start_lba, flags, sector_size, offset;
	char fn_old[256],  fn_new[256], full_fn_old[1024],  full_fn_new[1024];
	
	fr = fopen(ingdi, "r");
	
	if(!fr) 
	{
		printf("Can't open for read: %s\n", ingdi);
		fprintf (logfd, "Can't open for read: %s\n", ingdi);
		return -1;
	}
	
	fw = fopen(outgdi, "w");
	
	if(!fw) 
	{
		printf("Can't open for write: %s\n", outgdi);
		fprintf (logfd, "Can't open for write: %s\n", outgdi);
		fclose(fr);
		return -1;
	}
	
	printf("Converting %s\n", ingdi);
	fprintf (logfd, "Converting %s\n", ingdi);
	
	rc = fscanf(fr, "%d", &track_count);
	
	if(rc == 1) 
	{
		fprintf(fw, "%d\n", track_count);
		
		for(i = 0; i < track_count; i++) 
		{
			start_lba = flags = sector_size = offset = 0;
			memset(fn_new, 0, sizeof(fn_new));
			memset(fn_old, 0, sizeof(fn_old));
			
			rc = fscanf(fr, "%d %ld %ld %ld %s %ld", 
					&track_no, &start_lba, &flags, 
					&sector_size, fn_old, &offset);
			
			if (sector_size == 2048)
				continue;
			
			if(flags == 4) 
			{
				if (sector_size != 2048)
				{
					int len = strlen(fn_old);
					strncpy(fn_new, fn_old, sizeof(fn_new));
					fn_new[len - 3] = 'i';
					fn_new[len - 2] = 's';
					fn_new[len - 1] = 'o';
					fn_new[len] = '\0';
					sector_size = 2048;
					
					printf("Converting %s to %s ...\n", fn_old, fn_new);
					fprintf (logfd, "Converting %s to %s ...\n", fn_old, fn_new);
					
					snprintf(full_fn_old, 1024, "%s%s",folder, fn_old);
					snprintf(full_fn_new, 1024, "%s%s",folder, fn_new);
					
					if(bin2iso(full_fn_old, full_fn_new) < 0) 
					{
						printf("Error!\n");
						fclose(fr);
						fclose(fw);
						return -1;
					}
					
					unlink(full_fn_old);
				}
				
			}
			
			fprintf(fw, "%d %ld %ld %ld %s %ld\n", 
						track_no, start_lba, flags, sector_size, 
						(flags == 4 ? fn_new : fn_old), offset);
		}
	}
	
	fclose(fr);
	fclose(fw);
	unlink(ingdi);
	
	return 0;
}

int convert_folder(const char *infldr)
{
	DIR *indir;
	struct dirent *de;
	struct stat st;
	char name[1024], oname[1024];
	
	if (!(indir = opendir(infldr)))
	{
		printf("ERROR can't open directory %s\n", infldr);
		fprintf (logfd, "ERROR can't open directory %s\n", infldr);
		return -1;
	}
	
	while(de = readdir(indir))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		snprintf(name, 1024, "%s%s", infldr, de->d_name);
		
		stat(name, &st);
		
		if(S_ISREG(st.st_mode))
		{
			int len = strlen(de->d_name);
			
			if (len < 5)
				continue;
			
			if (de->d_name[len-4] != '.' ||
				de->d_name[len-3] != 'g' ||
				de->d_name[len-2] != 'd' ||
				de->d_name[len-1] != 'i' )
			{
				continue;
			}
			
			if (!strcmp(de->d_name, "disc_optimized.gdi"))
			{
				continue;
			}
			
			
			snprintf(oname, 1024, "%sdisc_optimized.gdi", infldr);
			
			if (convert_gdi(name, oname, infldr))
				fprintf (logfd, "ERROR: Can't optimize %s\n", name);
			
		}
		else if (S_ISDIR(st.st_mode))
		{
			snprintf(name, 1024, "%s%s/", infldr, de->d_name);
			
			convert_folder(name);
		}
	}
	
	closedir(indir);
	
	return 0;
}

int main(int argc, char *argv[]) 
{
	if(argc < 2) 
	{
		printf("GDI optimizer v0.2 by SWAT & megavolt85\n");
		printf("Usage: %s input_folder\n", argv[0]);
		return 0;
	}
	
	logfd = fopen("log.txt", "at");
	
	if (!logfd)
	{
		printf("ERROR can't create log file\n");
		return -1;
	}
	
	struct tm *ptr;
	time_t lt;
	lt = time(NULL);
	ptr = localtime(&lt);
	
	fprintf (logfd, "%s------ LOG Starte'd ------\n\n", asctime(ptr));
	
	convert_folder(argv[1]);
	
	
	fprintf (logfd, "\n\n------ LOG END ------\n\n");
	
	fclose(logfd);
	
	return 0;
}

