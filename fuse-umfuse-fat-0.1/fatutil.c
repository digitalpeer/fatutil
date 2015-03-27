/*
    fatutil: fat32 filesystem image manipulation program
	Copyright (C) 2015 Joshua Henderson <joshua.henderson@linux.com>

	FUSEFAT: Copyright (C) 2006-2007  Paolo Angelelli <angelell@cs.unibo.it>
		    Various acknowledgments to Renzo Davoli <renzo@cs.unibo.it>,
					       Ludovico Gardenghi <gardengl@cs.unibo.it>
		
	FUSE:    Copyright (C) 2001-2006  Miklos Szeredi <miklos@szeredi.hu>
	
    This program can be distributed under the terms of the GNU GPL.
*/

#include <config.h>
#include "libfat.h"

static const char* human_size(unsigned int size)
{
   static char buffer[255];
   if (size < 1024)
         sprintf(buffer, "%d", size);
   else if (size < 1024 * 1024)
      sprintf(buffer, "%dK", size / 1024);
   else
      sprintf(buffer, "%dM", size / 1024 / 1024);
   return buffer;
}

static int fusefat_getattr(Volume_t* V, const char *path, struct stat *stbuf) {
   File_t F;
   
   if (fat_open(path, &F, V, O_RDONLY))
      return -ENOENT; 
   
   if (fat_stat(&F, stbuf))
      return -1; 
   
   return 0;
}

static File_t* fusefat_open(Volume_t* V, const char *path) {
   File_t *F;

   F = malloc(sizeof(File_t));

   if (fat_open(path, F, V, O_RDWR)) { 
      free(F); 
      return 0;
   }

   return F;
}

static int fusefat_readdir(Volume_t* V, const char *path) {
    struct dirent de;
    File_t F;

    if (fat_open(path, &F, V, O_RDONLY))
       return -ENOENT;

    while (fat_readdir(&F, &de) == 0)
    {
       char filepath[1024];
       struct stat stbuf;

       if (strlen(path) == 1) {
	  strcpy(filepath, path);
	  strcat(filepath, de.d_name);
       }
       else
	  strcpy(filepath, path);

       if (fusefat_getattr(V, filepath, &stbuf))
	  return -ENOENT;

       if (de.d_type == DT_DIR)
	  printf("drwxr-xr-x");
       else
	  printf("-rwxr-xr-x");
	
       printf(" %s %s\n", human_size(stbuf.st_size), de.d_name);
    }

    return 0;
}

static int fusefat_mkdir(Volume_t* V, const char *path) {
   char dirname[4096];
   char filename[1024];
   File_t Parent;

   fat_dirname(path, dirname);
   fat_filename(path, filename);
   
   if (fat_open(dirname, &Parent, V, O_RDWR))
      return -ENOENT; 

   if (fat_mkdir(V, &Parent, filename , NULL, S_IFDIR))
      return -1;

   return 0;
}

static int fusefat_unlink(Volume_t* V, const char *path) {
   File_t F;
   
   if (fat_open(path, &F, V, O_RDWR))
      return -ENOENT;
   
   if (fat_delete(&F, 0))
      return -1;
   
   return 0;
}

static int fusefat_rmdir(Volume_t* V, const char *path) {
   File_t F;
   
   if (fat_open(path, &F, V, O_RDWR))
      return -ENOENT; 

   if (fat_rmdir(&F))
      return -1; 
   
   return 0;
}

static int fusefat_mknod(Volume_t* V, const char *path) {
   char dirname[4096];
   char filename[1024];
   File_t Parent;
   
   fat_dirname(path, dirname);
   fat_filename(path, filename);

   if (fat_open(dirname, &Parent, V, O_RDWR))
      return -ENOENT; 
   
   if (fat_create(V, &Parent, filename , NULL, S_IFREG, 0))
      return -1; 

   return 0;
}

static int fusefat_read(Volume_t* V, File_t* F, char *buf, size_t size, off_t offset) {
   int res, mode;
   DWORD fsize;

   fsize = EFD(F->DirEntry->DIR_FileSize);
   if ((size + (int) offset) > fsize)
      size = (fsize - offset);
   
   mode = F->Mode;
   F->Mode = O_RDONLY;
	
   if (fat_seek(F, offset, SEEK_SET) != offset)
      return -1; 

   if (fat_iseoc(V, F->CurClus) || fat_isfree(V,F->CurClus)) 
      return -1; 
	
   if ((res = fat_read_data(V, &(F->CurClus), &(F->CurOff), buf, size)) <= 0)
      return -1;

   F->CurAbsOff += res;
   F->Mode = mode;
   
   return res;
}

static int fusefat_write(Volume_t* V, File_t* F, char *buf, size_t size, off_t offset) {
	
    if (fat_seek(F, offset, SEEK_SET) < 0) { 
       fat_update_file(F);
       return -1; 
    }

    if (fat_write_data(V, F, &(F->CurClus), &(F->CurOff), buf, size) != size) { 
       fat_update_file(F); 
       return -1; 
    }

    if (fat_update_file(F) != 0)
       return -1; 
    
    return size;
}

static int fusefat_statvfs(Volume_t* V, struct statvfs *stbuf) {
   fat_statvfs(V, stbuf);
   return 0;
}

static void usage(const char* base)
{
   static const char* help =
      "fatutil 1.0\n"
      "usage: %s FATIMG CMD [OPTIONS]\n\n"
      "Where CMD [OPTIONS] is one of the following:\n"
      "  stat\n"
      "  ls DIR\n"
      "  rmdir DIR\n"
      "  unlink FILE\n"
      "  mkdir DIR\n"
      "  write SOURCE_FILE FAT_DEST_FILE\n"
      "  read FAT_SOURCE_FILE DEST_FILE\n"
      "  \n"
      "FAT_SOURCE_FILE and FAT_DEST_FILE can be - for stdin and stdout respectively.\n\n";

   fprintf(stderr, help, base);
}

int main(int argc, char *argv[])
{
   Volume_t volume;
   int res;

   if (argc < 3) {
      usage(argv[0]);
      return -1;
   }

   if ((res = fat_partition_init(&volume, argv[1], FAT_WRITE_ACCESS_FLAG)) < 0) {
      fprintf(stderr,"could not initialize fat image %s\n", argv[1]); 
      return -1;		
   }

   if (strcmp(argv[2],"write") == 0)
   {
      char* source;
      FILE* in;
      File_t* out;
      off_t offset = 0;

      if (argc != 5) {
	 fprintf(stderr, "error: write SOURCE_FILE FAT_DEST_FILE\n");
	 return -1;
      }

      source = argv[3];
      if (strcmp(source,"-") == 0)
	 in = stdin;
      else
	 in = fopen(source, "r");

      if (!in) {
	 fprintf(stderr, "error: could not open %s\n", source);
	 return -1;
      }

      fusefat_mknod(&volume, argv[4]);

      out = fusefat_open(&volume, argv[4]);
      if (!out) {
	 fprintf(stderr, "error: could not open %s\n", argv[4]);
	 return -1;
      }

      while (1)
      {
	 char buffer[1024];
	 size_t size = fread(buffer, 1, sizeof(buffer), in);
	 if (size) {
	    if (fusefat_write(&volume, out, buffer, size, offset) != size)
	       fprintf(stderr, "error: could not write to %s\n", argv[4]);
	 }
	 offset += size;
	 if (size < sizeof(buffer))
	    break;
      }

      if (strcmp(source,"-") != 0)
	 fclose(in);

      free(out);
   }
   else if (strcmp(argv[2],"read") == 0)
   {
      File_t* in;
      char* dest;
      FILE* out;
      off_t offset = 0;

      if (argc != 5) {
	 fprintf(stderr, "error: read FAT_SOURCE_FILE DEST_FILE\n");
	 return -1;
      }

      in = fusefat_open(&volume, argv[3]);

      dest = argv[4];
      if (strcmp(dest,"-") == 0)
	 out = stdout;
      else
	 out = fopen(dest,"w");

      if (in && out)
      {
	 struct stat stbuf;

	 if (fusefat_getattr(&volume, argv[3], &stbuf)) {
	    fprintf(stderr, "error: could not stat %s\n", argv[3]);
	    return -1;
	 }

	 while (stbuf.st_size)
	 {
	    char buffer[1024];
	    size_t size = stbuf.st_size > sizeof(buffer) ? sizeof(buffer) : stbuf.st_size;

	    fusefat_read(&volume, in, buffer, size, offset);
	    fwrite(buffer, 1, size, out);

	    offset += size;
	    stbuf.st_size -= size;
	 }
      }

      if (strcmp(dest,"-") != 0)
	 fclose(out);
      free(in);
   }
   else if (strcmp(argv[2],"mkdir") == 0)
   {
      if (argc != 4) {
	 fprintf(stderr, "error: mkdir DIR\n");
	 return -1;
      }

      if (fusefat_mkdir(&volume, argv[3])) {
	 fprintf(stderr, "error: mkdir failed\n");
	 return -1;
      }
   }
   else if (strcmp(argv[2],"unlink") == 0)
   {
      if (argc != 4) {
	 fprintf(stderr, "error: unlink FILE\n");
	 return -1;
      }
      
      if (fusefat_unlink(&volume, argv[3])) {
	 fprintf(stderr, "error: unlink failed\n");
	 return -1;
      }
   }
   else if (strcmp(argv[2],"rmdir") == 0)
   {
      if (argc != 4) {
	 fprintf(stderr, "error: rmdir DIR\n");
	 return -1;
      }

      fusefat_rmdir(&volume, argv[3]);
   }
   else if (strcmp(argv[2],"ls") == 0)
   {
      if (argc != 4) {
	 fprintf(stderr, "error: ls DIR\n");
	 return -1;
      }

      if (fusefat_readdir(&volume, argv[3]))
	 fprintf(stderr, "error: readdir failed\n");
   }   
   else if (strcmp(argv[2],"stat") == 0)
   {
      struct statvfs stbuf;
      if (fusefat_statvfs(&volume, &stbuf)) {
	 fprintf(stderr, "error: stat failed\n");
	 return -1;
      }

      printf("ID: %ld\n", stbuf.f_fsid);
      printf("size: %s\n", human_size(stbuf.f_blocks * stbuf.f_frsize));
      printf("free: %s\n", human_size(stbuf.f_bfree * stbuf.f_bsize));
   }
   else
   {
      fprintf(stderr,"error: unknown command %s\n", argv[2]);
      return -1;
   }

   res = fat_partition_finalize(&volume);
   
   return res;
}
