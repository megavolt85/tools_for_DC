/*
 * main.c
 * 
 * Copyright 2017 megavolt
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
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
#include <ctype.h>
#include <errno.h>

#define ADX  0x20000080
#define PPVP 0x50565050
#define PVPL 0x00000020
#define MPAC 0x4341504D
#define STR  0x44535053
#define SRF1 0x00000008
#define SRF2 0x52494B41

typedef struct
{
	uint32_t offset;
	uint32_t len;
}flist_s;

typedef struct
{
	char filename[32];
	uint32_t unknown[3];
	uint32_t size;
} fnames_s;

typedef struct
{
	char path[256];
	uint32_t rsize;
	uint32_t bsize;
	uint32_t offset;
} build_list_s;

typedef struct
{
	char magic[4]; // string AFS\0
	uint32_t numfiles;
	flist_s *flist;
	flist_s fnamelistptr;
	fnames_s *fnames;
} header_t;

static header_t header_s;

#ifdef NO_GUI
#define VERSION "0.2"
extern char *optarg;
#else
#include "win.h"
#endif

char list = 0;
char *extnamelist = NULL;
char nofn = 0;

uint8_t have_ext(const char *fn)
{
	uint32_t len = strlen(fn), i;
	
	for (i = 0; i < len; i++)
	{
		if (fn[i] == '.')
		{
			return 1;
		}
		
	}
	
	return 0;
}

const char *fn2fldrn(const char *fn)
{
	uint32_t len = strlen(fn);
	
	for (len--; len ; len--)
		if (fn[len] == '/')
		{
			len++;
			break;
		}
	
	return &fn[len];
}

char *strtolower(char *text)
{
	char *p;
	static char txt[256];
	
	strncpy(txt, text, 256);
	
	for(p = txt; *p; p++)
	{
		if(isupper((uint8_t)*p))
			{ *p = tolower((uint8_t) * p); }
	}
	
	return txt;
}

int unpack_afs(const char *fn)
{
	char ext[6];
	DIR *outdir;
	uint32_t i;
	char odirname[256];
	char ofname[256];
	char filename[256];
	struct stat st;
	int rv = 0;
	FILE *fp = NULL, *fo = NULL;
	
#ifndef NO_GUI
	pb->w_pbar2->value(0.0f);
	pb->w_pbar2->label("0%");
	Fl::check();
#endif
	
	fp = fopen(fn, "rb");
	
	if (!fp)
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка открытия %s\n", fn);
#else
		printf("ERROR can't open %s\n", fn);
#endif
		return -1;
	}
	
	memset((void *)&header_s, 0, sizeof(header_s));
	
	fread((void *)&header_s, 4, 2, fp);
	
	if (strncmp(header_s.magic, "AFS", 3))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, это не AFS файл (%s)\n", fn);
#else
		printf("ERROR this is not correct AFS file (%s)\n", fn);
#endif
		return -1;
	}
	
	header_s.flist = (flist_s*) calloc(header_s.numfiles, sizeof(flist_s));
	
	if (!header_s.flist)
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, нет свободной памяти\n");
#else
		printf("ERROR no free memory\n");
#endif	
		return -1;		
	}
	
	fread((void *) header_s.flist, sizeof(flist_s), header_s.numfiles, fp);
	
	fseek(fp, header_s.flist[0].offset-8, SEEK_SET);
	
	fread((void *) &header_s.fnamelistptr, sizeof(flist_s), 1, fp);
	
	if (extnamelist)
	{
		uint32_t efilenum = 0, eoffset = 0;
		
		if (!(fo = fopen(extnamelist, "rb")))
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка открытия %s\n", extnamelist);
#else
			printf("ERROR can't open %s\n", extnamelist);
#endif
			rv = -1;
			goto ext_exit;
		}
		
		fstat(fileno(fo), &st);
		
		if(!S_ISREG(st.st_mode) || strncasecmp(&extnamelist[strlen(extnamelist)-4], ".lst", 4))
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка, это не *.lst файл\n%s\n", &extnamelist[strlen(extnamelist)-4]);
#else
			printf("ERROR is not lst file\n");
			printf("%s\n", &extnamelist[strlen(extnamelist)-4]);
#endif
			fclose (fo);
			fo = NULL;
			rv = -1;
			goto ext_exit;
		}
		
		fread(&efilenum, 4, 1, fo);
		fread(&eoffset, 4, 1, fo);
		
		if (st.st_size > 8 && efilenum == header_s.numfiles)
		{
			if(!(header_s.fnames = (fnames_s*) calloc(efilenum+1, sizeof(fnames_s))))
			{
#ifndef NO_GUI
				fl_beep(FL_BEEP_ERROR);
				fl_alert("Ошибка, нет свободной памяти\n");
#else
				printf("ERROR no free memory\n");
#endif
				fclose (fo);
				rv = -1;
				goto ext_exit;
			}
			
			fread(header_s.fnames, st.st_size-8, 1, fo);
			
			header_s.fnamelistptr.offset = eoffset;
			
			header_s.fnamelistptr.len = st.st_size - 8;
		}
		
		fclose (fo);
	}
	else if (header_s.fnamelistptr.offset)
	{
		header_s.fnames = (fnames_s*) calloc(header_s.fnamelistptr.len, sizeof(fnames_s));
		fseek(fp, header_s.fnamelistptr.offset, SEEK_SET);
		fread((void *) header_s.fnames, sizeof(flist_s), header_s.fnamelistptr.len, fp);
	}
	
#ifdef NO_GUI	
	printf("magic %s\t count %d\n", header_s.magic, header_s.numfiles);
#endif
	
	if (!list)
	{
		if (!(outdir = opendir("workdir/")))
		{
			if (mkdir("workdir/"
#ifndef WIN32
			, 0777
#endif
			))
			{
#ifndef NO_GUI
				fl_beep(FL_BEEP_ERROR);
				fl_alert("Ошибка, не могу создать папку \"workdir\"\n");
#else
				printf("ERROR can't create directory \"workdir\"\n");
#endif
				rv = -1;
				goto ext_exit;
			}
		}
		else
			closedir(outdir);
		
		snprintf(odirname, 256, "workdir/%s/", fn2fldrn(fn));
		
		if (!(outdir = opendir(odirname)))
		{
			if (mkdir(odirname
#ifndef WIN32
			, 0777
#endif
			))
			{
#ifndef NO_GUI
				fl_beep(FL_BEEP_ERROR);
				fl_alert("Ошибка, не могу создать папку (%s)\n", odirname);
#else
				printf("ERROR can't create create directory (%s)\n", odirname);
#endif
				rv = -1;
				goto ext_exit;
			}
		}
		else
			closedir(outdir);
		
		snprintf(ofname, 256, "workdir/%s.lst", fn2fldrn(fn));
			
		if (!(fo = fopen(ofname, "wb")))
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка, не могу создать файл %s\n", ofname);
#else
			printf("ERROR can't create file %s\n", ofname);
#endif
			rv = -1;
			goto ext_exit;
		}
			
		fwrite((void *) &header_s.numfiles, sizeof(uint32_t), 1, fo);
		
		fwrite((void *) &header_s.flist[0].offset, sizeof(uint32_t), 1, fo);
		
		if (header_s.fnamelistptr.offset)
		{
			fwrite((void *) header_s.fnames, header_s.fnamelistptr.len, 1, fo);
		}
		
		fclose(fo);
	}
	
#ifndef NO_GUI	
	int percent, oldpercent;
	percent = oldpercent = 0;
#endif
	
	for (i = 0; i < header_s.numfiles; i++)
	{
#ifndef NO_GUI
		percent = int((i/(float)header_s.numfiles)*100.0);
		
		if (percent > oldpercent)
		{
			oldpercent = percent;
			snprintf(l_pbar2, sizeof(l_pbar2), "%d%%", percent);
			pb->w_pbar2->value((float) (i/(float)header_s.numfiles));
			pb->w_pbar2->label(l_pbar2);
			Fl::check();
		}
#endif	
		uint32_t fheaders[2];
		
		fseek(fp, header_s.flist[i].offset, SEEK_SET);
		fread((void*) fheaders, 2, 4, fp);
		
		switch (fheaders[0])
		{
			case ADX:
				strcpy(ext, ".adx");
				break;
			
			case PPVP:
				strcpy(ext, ".ppvp");
				break;
			
			case PVPL:
				strcpy(ext, ".pvpl");
				break;
			
			case MPAC:
				strcpy(ext, ".mpac");
				break;
			
			case STR:
				strcpy(ext, ".str");
				break;
			
			case SRF1:
				if (fheaders[1] == SRF2)
				{
					strcpy(ext, ".SRF");
					break;
				}
			
			default:
				strcpy(ext, ".bin");
				break;
		}
		
		
		if (!header_s.fnamelistptr.offset || !header_s.fnames[i].filename[0] || nofn)
			snprintf(filename, 256, "no_name_%d%s", i+1, ext);
		else if (!strstr(strtolower(header_s.fnames[i].filename), ext) && !have_ext(header_s.fnames[i].filename))
			snprintf(filename, 256, "%s%s", header_s.fnames[i].filename, ext);
		else
			snprintf(filename, 256, "%s", header_s.fnames[i].filename);
		
#ifdef NO_GUI
		printf("%s    \toffset 0x%08X\t size %d\n", filename, header_s.flist[i].offset, header_s.flist[i].len);
#endif
		
		if (!list)
		{
			snprintf(ofname, 256, "%s%s", odirname, filename);
			
			if (!(fo = fopen(ofname, "wb")))
			{
#ifndef NO_GUI
				fl_beep(FL_BEEP_ERROR);
				fl_alert("Ошибка, не могу создать файл %s\n", ofname);
#else
				printf("ERROR can't create file %s\n", ofname);
#endif
				rv = -1;
				goto ext_exit;
			}
			
			void *tmpbuf = malloc(header_s.flist[i].len+8);
			
			if (!tmpbuf)
			{
#ifndef NO_GUI
				fl_beep(FL_BEEP_ERROR);
				fl_alert("Ошибка, нет свободной памяти\n");
#else
				printf("ERROR no free memory\n");
#endif
				fclose(fo);
				rv = -1;
				goto ext_exit;
			}
			
			fseek(fp, header_s.flist[i].offset, SEEK_SET);
			fread (tmpbuf, header_s.flist[i].len, 1, fp);
			fwrite(tmpbuf, header_s.flist[i].len, 1, fo);
			fclose(fo);
			free(tmpbuf);
		}
		
	}
#ifdef NO_GUI
	printf("filename list offset 0x%08X\t filename list size %d\n", header_s.fnamelistptr.offset, header_s.fnamelistptr.len);
#else
	pb->w_pbar2->value(1.0f);
	pb->w_pbar2->label("100%");
#endif

ext_exit:
	
	if (fp)
		fclose(fp);
	
	if (header_s.flist)
		free(header_s.flist);
	
	if (header_s.fnames)
		free(header_s.fnames);
	
	memset((void *)&header_s, 0, sizeof(header_s));
	
	return rv;
}

int unpack_folder(const char *fn)
{
	DIR *indir;
	struct dirent *de;
	int rv = 0;
	
	if (!(indir = opendir(fn)))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, не могу открыть папку %s\n", fn);
#else
		printf("ERROR can't open directory %s\n", fn);
#endif
		return -1;
	}
	
	while(de = readdir(indir))
	{
		struct stat st;
		char name[256];
		
		snprintf(name, 256, "%s%s", fn, de->d_name);
		
		stat(name, &st);
		
		if(S_ISREG(st.st_mode) && !strncasecmp(&de->d_name[strlen(de->d_name)-4], ".afs", 4))
		{
			if (!unpack_afs(name))
				rv++;
#ifndef NO_GUI
			pb->w_pbar1->value(pb->w_pbar1->value()+1.0f);
			snprintf(l_pbar1, sizeof(l_pbar1), "%d/%d", (int) pb->w_pbar1->value(), (int) pb->w_pbar1->maximum());
			pb->w_pbar1->label(l_pbar1);
#endif
		}
	}
	closedir(indir);
	
	return rv;
}

int pack_afs(const char *fn)
{
	FILE *fp = NULL, *fo = NULL;
	struct stat st;
	uint32_t i, filenum, offset;
	fnames_s *names = NULL;
	build_list_s *blist = NULL;
	DIR *dir = NULL;
	struct dirent *de = NULL;
	char dirname[256], filename[256];
	uint8_t *buf = NULL;
	int rv = 0;
	int found = 0;
#ifndef NO_GUI
	pb->w_pbar2->value(0.0f);
	pb->w_pbar2->label("0%");
#endif		
	if (!(fp = fopen(fn, "rb")))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, не могу открыть %s\n", fn);
#else
		printf("ERROR can't open %s\n", fn);
#endif
		return -1;
	}
	
	fstat(fileno(fp), &st);
	
	if(!S_ISREG(st.st_mode) || strncasecmp(&fn[strlen(fn)-4], ".lst", 4))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, это не *.lst файл\n");
#else
		printf("ERROR is not *.lst file\n");
#endif
		fclose (fp);
		return -1;
	}

	fread(&filenum, 4, 1, fp);
	fread(&offset, 4, 1, fp);
	
	if (st.st_size > 8)
	{
		if(!(names = (fnames_s*) calloc(filenum+1, sizeof(fnames_s))))
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка, нет свободной памяти\n");
#else
			printf("ERROR no free memory\n");
#endif
			fclose (fp);
			return -1;
		}
		
		fread(names, st.st_size-8, 1, fp);
	}
	
	fclose (fp);
	fp = NULL;
	
	if (extnamelist)
	{
		uint32_t efilenum = 0, eoffset = 0;
		
		if (!(fp = fopen(extnamelist, "rb")))
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка, не могу открыть %s\n", extnamelist);
#else
			printf("ERROR can't open %s\n", extnamelist);
#endif
			return -1;
		}
		
		fstat(fileno(fp), &st);
		
		if(!S_ISREG(st.st_mode) || strncasecmp(&extnamelist[strlen(fn)-4], ".lst", 4))
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка, это не *.lst файл\n");
#else
			printf("ERROR is not *.lst file\n");
#endif
			fclose (fp);
			return -1;
		}

		fread(&efilenum, sizeof(uint32_t), 1, fp);
		fread(&eoffset, sizeof(uint32_t), 1, fp);
		
		if (st.st_size > 8 && efilenum == filenum)
		{
			if (names != NULL)
			{
				free(names);
				names = NULL;
			}
			
			if(!(names = (fnames_s*) calloc(efilenum+1, sizeof(fnames_s))))
			{
#ifndef NO_GUI
				fl_beep(FL_BEEP_ERROR);
				fl_alert("Ошибка, нет свободной памяти\n");
#else
				printf("ERROR no free memory\n");
#endif
				fclose (fp);
				return -1;
			}
			
			fread(names, st.st_size-8, 1, fp);
		}
		
		fclose (fp);
		fp = NULL;
	}
	
	strncpy(dirname, fn, strlen(fn)-4);
	dirname[strlen(fn)-4] = '\0';
	
	if (!(dir = opendir(dirname)))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, не могу открыть папку %s\n", dirname);
#else
		printf("ERROR can't open directory %s\n",dirname);
#endif
		return -1;
	}
	
	snprintf(filename, 256, "outfiles/%s", fn2fldrn(dirname));
	
	if (!(fo = fopen(filename, "wb")))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, не могу открыть %s\n", filename);
#else
		printf("ERROR can't open %s\n", filename);
#endif
		rv = -1;
		goto ext2_exit;
	}
	
	fwrite("AFS\0", sizeof(uint32_t), 1, fo);
	fwrite((void *) &filenum, sizeof(uint32_t), 1, fo);
	
	if(!(blist = (build_list_s*) calloc(filenum+1, sizeof(build_list_s))))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, нет свободной памяти\n");
#else
		printf("ERROR no free memory\n");
#endif
		rv = -1;
		goto ext2_exit;
	}
	
#ifndef NO_GUI
	int percent, oldpercent;
	percent = oldpercent = 0;
#endif
	
	while((de = readdir(dir)))
	{
		for (i = 0; i < filenum; i++)
		{
			if (!names)
				snprintf(filename, 256, "%s/no_name_%d.", dirname, i+1);
			else
				snprintf(filename,  256, "%s/%s%c", dirname, names[i].filename, have_ext(names[i].filename)?'\0':'.');
			
			if (!strncmp(fn2fldrn(filename), de->d_name, strlen(fn2fldrn(filename))))
			{
#ifndef NO_GUI
				percent = int((found/(float)filenum)*100.0);
				
				if (percent > oldpercent)
				{
					oldpercent = percent;
					snprintf(l_pbar2, sizeof(l_pbar2), "%d%%", percent);
					pb->w_pbar2->value((float) (found/(float)filenum));
					pb->w_pbar2->label(l_pbar2);
					Fl::check();
				}
#endif
				snprintf(blist[i].path ,  256, "%s/%s", dirname, de->d_name);
				
				if (stat(blist[i].path, &st))
				{
#ifndef NO_GUI
					fl_beep(FL_BEEP_ERROR);
					fl_alert("Ошибка, файла %s не существует\n", blist[i].path);
#else
					printf("ERROR file %s not found\n", blist[i].path);
#endif
					rv = -1;
					goto ext2_exit;
				}
				
				blist[i].rsize = st.st_size;
				blist[i].bsize = (st.st_size%2048) ? (st.st_size+(2048-(st.st_size%2048))) : st.st_size;
				found++;
				break;
			}
		}
	}
	
	if (found != filenum)
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, в папке %s не хватает файлов\n", dirname);
#else
		printf("ERROR in the %s folder there aren't enough files\n", dirname);
#endif
		rv = -1;
		goto ext2_exit;		
	}
	
	for (i = 0; i < filenum; i++)
	{
		blist[i].offset = offset;
		offset += blist[i].bsize;
		
		fwrite((void *) &blist[i].offset, sizeof(uint32_t), 1, fo);
		fwrite((void *) &blist[i].rsize , sizeof(uint32_t), 1, fo);
	}
	
	if(!(buf = (uint8_t*) calloc(blist[0].offset-filenum*8, 1)))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, нет свободной памяти\n");
#else
		printf("ERROR no free memory\n");
#endif
		rv = -1;
		goto ext2_exit;
	}
	
	if (!names || nofn)
		fwrite((void *) buf, (blist[0].offset-filenum*8)-8, 1, fo);
	else
	{
		fwrite((void *) buf, (blist[0].offset-filenum*8)-16, 1, fo);
		fwrite((void *) &offset, sizeof(uint32_t), 1, fo);
		uint32_t names_size = filenum * sizeof(fnames_s);
		fwrite((void *) &names_size, sizeof(uint32_t), 1, fo);
	}
	
	if(buf != NULL)
		free(buf);
/*	
#ifndef NO_GUI
	int percent, oldpercent;
	percent = oldpercent = 0;
#endif*/
	
	for (i = 0; i < filenum; i++)
	{
/*#ifndef NO_GUI
		percent = int((i/(float)filenum)*100.0);
		
		if (percent > oldpercent)
		{
			oldpercent = percent;
			snprintf(l_pbar2, sizeof(l_pbar2), "%d%%", percent);
			pb->w_pbar2->value((float) (i/(float)filenum));
			pb->w_pbar2->label(l_pbar2);
			Fl::check();
		}
#endif*/
		if ((fp = fopen(blist[i].path, "rb")) == NULL)
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка, не могу открыть %s\n", blist[i].path);
#else
			printf("ERROR can't open %s\n", blist[i].path);
#endif
			rv = -1;
			goto ext2_exit;
		}
		
		if(!(buf = (uint8_t*) calloc(blist[i].bsize, 1)))
		{
#ifndef NO_GUI
			fl_beep(FL_BEEP_ERROR);
			fl_alert("Ошибка, нет свободной памяти\n");
#else
			printf("ERROR no free memory\n");
#endif
			rv = -1;
			goto ext2_exit;
		}
		
		fread ((void *) buf, blist[i].rsize, 1, fp);
		fwrite((void *) buf, blist[i].bsize, 1, fo);
		
		if(buf != NULL)
		{
			free(buf);
			buf = NULL;
		}
		
		fclose (fp);
		fp = NULL;
	}
	
	if (names != NULL && !nofn)
	{
		fwrite((void *) names, sizeof(fnames_s), filenum, fo);
		
		uint32_t allign_size = 2048 - ((sizeof(fnames_s) * filenum) % 2048);
		
		if (allign_size)
		{
			if(!(buf = (uint8_t*) calloc(allign_size, 1)))
			{
#ifndef NO_GUI
				fl_beep(FL_BEEP_ERROR);
				fl_alert("Ошибка, нет свободной памяти\n");
#else
				printf("ERROR no free memory\n");
#endif
				rv = -1;
				goto ext2_exit;
			}
			
			fwrite((void *) buf, allign_size, 1, fo);
			
			if(buf != NULL)
				free(buf);
		}
	}
#ifndef NO_GUI
	pb->w_pbar2->value(1.0f);
	pb->w_pbar2->label("100%");
#endif
ext2_exit:
	
	if (fp != NULL)
	{
		fclose (fp);
		fp = NULL;
	}
	
	if (fo != NULL)
	{
		fclose (fo);
		fo = NULL;
	}
	
	closedir(dir);
	
	if (names != NULL)
	{
		free(names);
		names = NULL;
	}
	
	if (rv < 0 && !strncmp(filename, "outfiles/", 9))
	{
		unlink(filename);
	}
	
	
	return rv;
}

int pack_folder(const char *fn)
{
	DIR *indir;
	struct dirent *de;
	int rv = 0;
	
#ifndef NO_GUI
	int num_files = win->fbrowser1->size();
#endif
	
	if (!(indir = opendir(fn)))
	{
#ifndef NO_GUI
		fl_beep(FL_BEEP_ERROR);
		fl_alert("Ошибка, не могу открыть папку %s\n", fn);
#else
		printf("ERROR can't open directory %s\n", fn);
#endif
		return -1;
	}
	
	while(de = readdir(indir))
	{
		struct stat st;
		char name[256];
		
		snprintf(name, 256, "%s%s", fn, de->d_name);
		
		stat(name, &st);
		
		if(S_ISREG(st.st_mode) && !strncasecmp(&de->d_name[strlen(de->d_name)-4], ".lst", 4))
		{
			if(!pack_afs(name))
				rv++;
#ifndef NO_GUI
			pb->w_pbar1->value(pb->w_pbar1->value()+1.0f);
			snprintf(l_pbar1, sizeof(l_pbar1), "%d/%d", (int) pb->w_pbar1->value(), (int) pb->w_pbar1->maximum());
			pb->w_pbar1->label(l_pbar1);
#endif
		}
	}
	closedir(indir);
	
	return rv;
}

#ifdef NO_GUI
void usage()
{
	printf("Usage: AFS Tools v %s\n", VERSION);
	printf("-f input file\n");
	printf("-p Pack AFS\n");
	printf("-u Unpack AFS\n");
	printf("-l List files in AFS\n");
	printf("-e External *.lst with names\n");
	printf("-n Don't use name from AFS\n");
	printf("-h This help\n");
	
	exit(0);
}

int main(int argc, char **argv)
{
	int someopt;
	char *filename;
	int mode = 0;
	
	if (argc < 2)
		usage();
	
	someopt = getopt(argc, argv, "f:pule:hn");
	
	while (someopt > 0) 
    {
		switch (someopt) 
		{
			case 'f':
				filename = (char*) malloc(strlen(optarg) + 1);
				strcpy(filename, optarg);
				break;
			
			case 'e':
				extnamelist = (char*) malloc(strlen(optarg) + 1);
				strcpy(extnamelist, optarg);
				break;
			
			case 'p':
			case 'u':
			case 'l':
				if(mode)
				{
					printf("You can only specify one of -p, -u or -l\n");
					exit(0);
				}
				mode = someopt;
				break;
			
			case 'n':
				nofn = 1;
				break;
			
			case 'h':
				usage();
				break;
				
			default:
				break;
		}
		someopt = getopt(argc, argv, "f:pule:hn");
	}
	
	if (!filename || !mode)
		usage();
	
	if(mode == 'u' || mode == 'l')
	{
		list = (mode == 'l') ? 1 : 0;
		
		if (filename[strlen(filename)-1] == '/')
			unpack_folder(filename);
		else
			unpack_afs(filename);
	}
	else if(mode == 'p')
	{
		if (filename[strlen(filename)-1] == '/')
			pack_folder(filename);
		else
			pack_afs(filename);
	}
	
	free(filename);
	
	if (extnamelist)
		free(extnamelist);
	
	
	return 0;
}
#else
AFSToolsWindow *win;

int main(int argc, char **argv)
{
	AFSToolsWindow window;
	
	win = &window;
	
	window.show();
	
	return Fl::run();
}
#endif
