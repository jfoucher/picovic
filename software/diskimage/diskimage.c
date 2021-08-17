#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "ff.h"
#include "diskimage.h"

typedef struct errormessage {
  signed int number;
  char *string;
} ErrorMessage;


ErrorMessage error_msg[] = {
  /* non-errors */
  { 0, "ok" },
  { 1, "files scratched" },
  { 2, "partition selected" },
  /* errors */
  { 20, "read error (block header not found)" },
  { 21, "read error (drive not ready)" },
  { 22, "read error (data block not found)" },
  { 23, "read error (crc error in data block)" },
  { 24, "read error (byte sector header)" },
  { 25, "write error (write-verify error)" },
  { 26, "write protect on" },
  { 27, "read error (crc error in header)" },
  { 30, "syntax error (general syntax)" },
  { 31, "syntax error (invalid command)" },
  { 32, "syntax error (long line)" },
  { 33, "syntax error (invalid file name)" },
  { 34, "syntax error (no file given)" },
  { 39, "syntax error (invalid command)" },
  { 50, "record not present" },
  { 51, "overflow in record" },
  { 52, "file too large" },
  { 60, "write file open" },
  { 61, "file not open" },
  { 62, "file not found" },
  { 63, "file exists" },
  { 64, "file type mismatch" },
  { 65, "no block" },
  { 66, "illegal track and sector" },
  { 67, "illegal system t or s" },
  { 70, "no channel" },
  { 71, "directory error" },
  { 72, "disk full" },
  { 73, "dos mismatch" },
  { 74, "drive not ready" },
  { 75, "format error" },
  { 76, "controller error" },
  { 77, "selected partition illegal" },
  { -1, NULL }
};

void ptoa(unsigned char *s) {
  unsigned char c;

  while ((c = *s)) {
    c &= 0x7f;
    if (c >= 'A' && c <= 'Z') {
      c += 32;
    } else if (c >= 'a' && c <= 'z') {
      c -= 32;
    } else if (c == 0x7f) {
      c = 0x3f;
    }
    *s++ = c;
  }
}

void atop(unsigned char *s) {
  unsigned char c;

  while ((c = *s)) {
    c &= 0x7f;
    if (c >= 'A' && c <= 'Z') {
      c += 32;
    } else if (c >= 'a' && c <= 'z') {
      c -= 32;
    }
    *s++ = c;
  }
}

/** Read byte from image */
unsigned char read_from_image(DiskImage *di, int offset) {
  unsigned char buff[1];
  UINT br;
  f_lseek((FIL *)(di->image), offset);

  FRESULT fr = f_read((FIL *)(di->image), buff, 1, &br);
  if (fr != FR_OK) {
      printf("read_from_image f_read error: %s (%d) offset: %ld\n", FRESULT_str(fr), fr, offset);
  }

  return buff[0];
}

/** Write byte to image */
void write_to_image(DiskImage *di, int offset, unsigned char data) {
  unsigned char buff[1];
  UINT * bw;

  f_lseek(di->image, offset);
  FRESULT r = f_write (di->image, &data, 1, bw);
  if (r != FR_OK) {
      printf("could not write");
  }
}

/* convert to rawname */
int di_rawname_from_name(unsigned char *rawname, unsigned char *name) {
  int i;

  memset(rawname, 0xa0, 16);
  for (i = 0; i < 16 && name[i]; ++i) {
    rawname[i] = name[i];
  }
  return(i);
}


/* convert from rawname */
int di_name_from_rawname(unsigned char *name, unsigned char *rawname) {
  int i;

  for (i = 0; i < 16 && rawname[i] != 0xa0; ++i) {
    name[i] = rawname[i];
  }
  name[i] = 0;
  return(i);
}


/* return status string */
int di_status(DiskImage *di, char *status) {
  ErrorMessage *err = error_msg;

  /* special case for power up */
  if (di->status == 254) {
    switch (di->type) {
    case D64:
      sprintf(status, "73,cbm dos v2.6 1541,00,00");
      break;
    case D71:
      sprintf(status, "73,cbm dos v3.0 1571,00,00");
      break;
    case D81:
      sprintf(status, "73,copyright cbm dos v10 1581,00,00");
      break;
    }
    return(73);
  }

  while (err->number >= 0) {
    if (di->status == err->number) {
      sprintf(status, "%02d,%s,%02d,%02d", di->status, err->string, di->statusts.track, di->statusts.sector);
      return(di->status);
    }
    err->number++;
  }
  sprintf(status, "%02d,unknown error,%02d,%02d", di->status, di->statusts.track, di->statusts.sector);
  return(di->status);
}


int set_status(DiskImage *di, int status, int track, int sector) {
  di->status = status;
  di->statusts.track = track;
  di->statusts.sector = sector;
  return(status);
}


/* return write interleave */
int interleave(ImageType type) {
  switch (type) {
  case D64:
    return(10);
    break;
  case D71:
    return(6);
    break;
  default:
    return(1);
    break;
  }
}


/* return number of tracks for image type */
int di_tracks(ImageType type) {
  switch (type) {
  case D64:
    return(35);
    break;
  case D71:
    return(70);
    break;
  case D81:
    return(80);
    break;
  }
  return(0);
}


/* return disk geometry for track */
int di_sectors_per_track(ImageType type, int track) {
  switch (type) {
  case D71:
    if (track > 35) {
      track -= 35;
    }
    // fall through
  case D64:
    if (track < 18) {
      return(21);
    } else if (track < 25) {
      return(19);
    } else if (track < 31) {
      return(18);
    } else {
      return(17);
    }
    break;
  case D81:
    return(40);
    break;
  }
  return(0);
}

/* convert track, sector to blocknum */
int get_block_num(ImageType type, TrackSector ts) {
  int block;

  switch (type) {
  case D64:
    if (ts.track < 18) {
      block = (ts.track - 1) * 21;
    } else if (ts.track < 25) {
      block = (ts.track - 18) * 19 + 17 * 21;
    } else if (ts.track < 31) {
      block = (ts.track - 25) * 18 + 17 * 21 + 7 * 19;
    } else {
      block = (ts.track - 31) * 17 + 17 * 21 + 7 * 19 + 6 * 18;
    }
    return(block + ts.sector);
    break;
  case D71:
    if (ts.track > 35) {
      block = 683;
      ts.track -= 35;
    } else {
      block = 0;
    }
    if (ts.track < 18) {
      block += (ts.track - 1) * 21;
    } else if (ts.track < 25) {
      block += (ts.track - 18) * 19 + 17 * 21;
    } else if (ts.track < 31) {
      block += (ts.track - 25) * 18 + 17 * 21 + 7 * 19;
    } else {
      block += (ts.track - 31) * 17 + 17 * 21 + 7 * 19 + 6 * 18;
    }
    return(block + ts.sector);
    break;
  case D81:
    return((ts.track - 1) * 40 + ts.sector);
    break;
  }
  return(0);
}


/* get a pointer to block data */
int get_ts_addr(DiskImage *di, TrackSector ts) {
  return get_block_num(di->type, ts) * 256;
}


/* return a pointer to the next block in the chain */
TrackSector next_ts_in_chain(DiskImage *di, TrackSector ts) {
  int p;
  TrackSector newts;

  p = get_ts_addr(di, ts);

  newts.track = read_from_image(di, p);
  newts.sector = read_from_image(di, p+1);
  if (newts.track > di_tracks(di->type)) {
    newts.track = 0;
    newts.sector = 0;
  } else if (newts.sector > di_sectors_per_track(di->type, newts.track)) {
    newts.track = 0;
    newts.sector = 0;
  }

  // printf("next_ts_in_chain track %d, sector %d", newts.track, newts.sector);

  return(newts);
}


/* return a pointer to the disk title */
int di_title(DiskImage *di) {
  switch (di->type) {
  default:
  case D64:
  case D71:
    return(get_ts_addr(di, di->dir) + 144);
    break;
  case D81:
    return(get_ts_addr(di, di->dir) + 4);
    break;
  }
}


/* return number of free blocks in track */
int di_track_blocks_free(DiskImage *di, int track) {
  int bam;


  switch (di->type) {
  default:
  case D64:
    bam = get_ts_addr(di, di->bam);
    break;
  case D71:
    bam = get_ts_addr(di, di->bam);
    if (track >= 36) {
      return(read_from_image(di, bam + track + 185));
    }
    break;
  case D81:
    if (track <= 40) {
      bam = get_ts_addr(di, di->bam);
    } else {
      bam = get_ts_addr(di, di->bam2);
      track -= 40;
    }

    return(read_from_image(di, bam + track * 6 + 10));

    //return(bam[track * 6 + 10]);
    break;
  }
  return(read_from_image(di, bam + track * 4));
  //return(bam[track * 4]);
}




/* count number of free blocks */
int blocks_free(DiskImage *di) {
  int track;
  int blocks = 0;
  int ntracks = di_tracks(di->type);

  for (track = 1; track <= ntracks; ++track) {
    if (track != di->dir.track) {
      blocks += di_track_blocks_free(di, track);
    }
  }
  return(blocks);
}


/* check if track, sector is free in BAM */
int di_is_ts_free(DiskImage *di, TrackSector ts) {
  unsigned char mask;
  int bam;

  switch (di->type) {
  case D64:
    bam = get_ts_addr(di, di->bam);
    if (read_from_image(di, bam + ts.track * 4)) {
      mask = 1<<(ts.sector & 7);
      return(read_from_image(di, bam + ts.track * 4 + ts.sector / 8 + 1) & mask ? 1 : 0);
    } else {
      return(0);
    }
    break;
  case D71:
    mask = 1<<(ts.sector & 7);
    if (ts.track < 36) {
      bam = get_ts_addr(di, di->bam);
      return(read_from_image(di, bam + ts.track * 4 + ts.sector / 8 + 1) & mask ? 1 : 0);
    } else {
      bam = get_ts_addr(di, di->bam2);
      return(read_from_image(di, bam + (ts.track - 35) * 3 + ts.sector / 8 - 3) & mask ? 1 : 0);
    }
    break;
  case D81:
    mask = 1<<(ts.sector & 7);
    if (ts.track < 41) {
      bam = get_ts_addr(di, di->bam);
    } else {
      bam = get_ts_addr(di, di->bam2);
      ts.track -= 40;
    }
    return(read_from_image(di, bam + ts.track * 6 + ts.sector / 8 + 11) & mask ? 1 : 0);
    // return(bam[ts.track * 6 + ts.sector / 8 + 11] & mask ? 1 : 0);
    break;
  }
  return(0);
}


/* allocate track, sector in BAM */
void di_alloc_ts(DiskImage *di, TrackSector ts) {
  unsigned char mask;
  int bam;

  di->modified = 1;
  switch (di->type) {
  case D64:
    bam = get_ts_addr(di, di->bam);

    write_to_image(di, bam + ts.track * 4, read_from_image(di, bam + ts.track * 4) - 1);
    //bam[ts.track * 4] -= 1;
    mask = 1<<(ts.sector & 7);
    write_to_image(di, bam + ts.track * 4 + ts.sector / 8 + 1, read_from_image(di, bam + ts.track * 4 + ts.sector / 8 + 1) & ~mask);
    // bam[ts.track * 4 + ts.sector / 8 + 1] &= ~mask;
    break;
  case D71:
    mask = 1<<(ts.sector & 7);
    if (ts.track < 36) {
      bam = get_ts_addr(di, di->bam);
      write_to_image(di, bam + ts.track * 4, read_from_image(di, bam + ts.track * 4) - 1);
      //bam[ts.track * 4] -= 1;
      write_to_image(di, bam + ts.track * 4 + ts.sector / 8 + 1, read_from_image(di, bam + ts.track * 4 + ts.sector / 8 + 1) & ~mask);
      //bam[ts.track * 4 + ts.sector / 8 + 1] &= ~mask;
    } else {
      bam = get_ts_addr(di, di->bam);
      write_to_image(di, bam + ts.track + 185, read_from_image(di, bam + ts.track + 185) - 1);
      // bam[ts.track + 185] -= 1;
      bam = get_ts_addr(di, di->bam2);
      write_to_image(di, bam + (ts.track - 35) * 3 + ts.sector / 8 - 3, read_from_image(di, bam + (ts.track - 35) * 3 + ts.sector / 8 - 3) & ~mask);
      // bam[(ts.track - 35) * 3 + ts.sector / 8 - 3] &= ~mask;
    }
    break;
  case D81:
    if (ts.track < 41) {
      bam = get_ts_addr(di, di->bam);
    } else {
      bam = get_ts_addr(di, di->bam2);
      ts.track -= 40;
    }
    write_to_image(di, bam + ts.track * 6 + 10, read_from_image(di, bam + ts.track * 6 + 10) - 1);
    // bam[ts.track * 6 + 10] -= 1;
    mask = 1<<(ts.sector & 7);
    write_to_image(di, bam + ts.track * 6 + ts.sector / 8 + 11, read_from_image(di, bam + ts.track * 6 + ts.sector / 8 + 11) & ~mask);
    // bam[ts.track * 6 + ts.sector / 8 + 11] &= ~mask;
    break;
  }
}


/* allocate next available block */
TrackSector alloc_next_ts(DiskImage *di, TrackSector prevts) {
  int bam;
  int spt, s1, s2, t1, t2, bpt, boff, res1, res2;
  TrackSector ts;

  switch (di->type) {
  default:
  case D64:
    s1 = 1;
    t1 = 35;
    s2 = 1;
    t2 = 35;
    res1 = 18;
    res2 = 0;
    bpt = 4;
    boff = 0;
    break;
  case D71:
    s1 = 1;
    t1 = 35;
    s2 = 36;
    t2 = 70;
    res1 = 18;
    res2 = 53;
    bpt = 4;
    boff = 0;
    break;
  case D81:
    s1 = 1;
    t1 = 40;
    s2 = 41;
    t2 = 80;
    res1 = 40;
    res2 = 0;
    bpt = 6;
    boff = 10;
    break;
  }

  bam = get_ts_addr(di, di->bam);
  for (ts.track = s1; ts.track <= t1; ++ts.track) {
    if (ts.track != res1) {
      if  (read_from_image(di, bam + ts.track * bpt + boff) /*bam[ts.track * bpt + boff]*/) {
        spt = di_sectors_per_track(di->type, ts.track);
        ts.sector = (prevts.sector + interleave(di->type)) % spt;
        for (; ; ts.sector = (ts.sector + 1) % spt) {
          if (di_is_ts_free(di, ts)) {
            di_alloc_ts(di, ts);
            return(ts);
          }
        }
      }
    }
  }

  if (di->type == D71 || di->type == D81) {
    bam = get_ts_addr(di, di->bam2);
    for (ts.track = s2; ts.track <= t2; ++ts.track) {
      if (ts.track != res2) {
        if  (read_from_image(di, bam + (ts.track - t1) * bpt + boff) /*bam[(ts.track - t1) * bpt + boff]*/) {
          spt = di_sectors_per_track(di->type, ts.track);
          ts.sector = (prevts.sector + interleave(di->type)) % spt;
          for (; ; ts.sector = (ts.sector + 1) % spt) {
            if (di_is_ts_free(di, ts)) {
              di_alloc_ts(di, ts);
              return(ts);
            }
          }
        }
      }
    }
  }

  ts.track = 0;
  ts.sector = 0;
  return(ts);
}


/* allocate next available directory block */
TrackSector alloc_next_dir_ts(DiskImage *di) {
  int p;
  int spt;
  TrackSector ts, lastts;

  if (di_track_blocks_free(di, di->bam.track)) {
    ts.track = di->bam.track;
    ts.sector = 0;
    while (ts.track) {
      lastts = ts;
      ts = next_ts_in_chain(di, ts);
    }
    ts.track = lastts.track;
    ts.sector = lastts.sector + 3;
    spt = di_sectors_per_track(di->type, ts.track);
    for (; ; ts.sector = (ts.sector + 1) % spt) {
      if (di_is_ts_free(di, ts)) {
        di_alloc_ts(di, ts);
        p = get_ts_addr(di, lastts);
        write_to_image(di, p, ts.track);
        write_to_image(di, p+1, ts.sector);
        // p[0] = ts.track;
        // p[1] = ts.sector;
        p = get_ts_addr(di, ts);
        for (int nn = 0; nn < 256; nn++) {
          write_to_image(di, p+nn, 0);
        }
        // memset(p, 0, 256);
        // p[1] = 0xff;
        write_to_image(di, p+1, 0xFF);
        di->modified = 1;
        return(ts);
      }
    }
  } else {
    ts.track = 0;
    ts.sector = 0;
    return(ts);
  }
}


/* free a block in the BAM */
void di_free_ts(DiskImage *di, TrackSector ts) {
  unsigned char mask;
  int bam;

  di->modified = 1;
  switch (di->type) {
  case D64:
    mask = 1<<(ts.sector & 7);
    bam = get_ts_addr(di, di->bam);
    write_to_image(di, bam+ts.track * 4 + ts.sector / 8 + 1, read_from_image(di, bam+ts.track * 4 + ts.sector / 8 + 1) | mask);

    write_to_image(di, bam+ts.track * 4, read_from_image(di, bam+ts.track * 4) + 1);

    // bam[ts.track * 4 + ts.sector / 8 + 1] |= mask;
    // bam[ts.track * 4] += 1;
    break;
  case D71:
    mask = 1<<(ts.sector & 7);
    if (ts.track < 36) {
      bam = get_ts_addr(di, di->bam);
      write_to_image(di, bam+ts.track * 4 + ts.sector / 8 + 1, read_from_image(di, bam+ts.track * 4 + ts.sector / 8 + 1) | mask);

      write_to_image(di, bam+ts.track * 4, read_from_image(di, bam+ts.track * 4) + 1);
    } else {
      bam = get_ts_addr(di, di->bam);
      write_to_image(di, bam+ts.track + 185, read_from_image(di, bam+ts.track + 185) + 1);
      // bam[ts.track + 185] += 1;
      bam = get_ts_addr(di, di->bam2);
      write_to_image(di, bam+(ts.track - 35) * 3 + ts.sector / 8 - 3, read_from_image(di, bam+(ts.track - 35) * 3 + ts.sector / 8 - 3) | mask);
      // bam[(ts.track - 35) * 3 + ts.sector / 8 - 3] |= mask;
    }
    break;
  case D81:
    if (ts.track < 41) {
      bam = get_ts_addr(di, di->bam);
    } else {
      bam = get_ts_addr(di, di->bam2);
      ts.track -= 40;
    }
    mask = 1<<(ts.sector & 7);
    write_to_image(di, bam+ts.track * 6 + ts.sector / 8 + 11, read_from_image(di, bam+ts.track * 6 + ts.sector / 8 + 11) | mask);
    // bam[ts.track * 6 + ts.sector / 8 + 11] |= mask;
    write_to_image(di, bam+ts.track * 6 + 10, read_from_image(di, bam+ts.track * 6 + 10) + 1);
    // bam[ts.track * 6 + 10] += 1;
    break;
  default:
    break;
  }
}


/* free a chain of blocks */
void free_chain(DiskImage *di, TrackSector ts) {
  while (ts.track) {
    di_free_ts(di, ts);
    ts = next_ts_in_chain(di, ts);
  }
}


DiskImage *di_load_image(const TCHAR *name) {
  FIL *file;
  UINT filesize;

  DiskImage *di;

  if ((file = malloc(sizeof(*file))) == NULL) {
    puts("file malloc failed");

    return(NULL);
  }


  // printf("loading image from %s\n", name);

  FRESULT fr = f_open(file, name, FA_OPEN_EXISTING | FA_READ);

  /* open image */
  if (FR_OK != fr && FR_EXIST != fr) {
    panic("f_open(%s) error: %s (%d)\n", name, FRESULT_str(fr), fr);
  }


  filesize = (file)->obj.objsize;
  f_rewind(&file);

  if ((di = malloc(sizeof(*di))) == NULL) {
    puts("malloc failed");
    f_close(file);
    free(file);
    return(NULL);
  }

  /* check image type */
  switch (filesize) {
  case 174848: // standard D64
  case 175531: // D64 with error info (which we just ignore)
    di->type = D64;
    di->bam.track = 18;
    di->bam.sector = 0;
    di->dir = di->bam;
    break;
  case 349696:
    di->type = D71;
    di->bam.track = 18;
    di->bam.sector = 0;
    di->bam2.track = 53;
    di->bam2.sector = 0;
    di->dir = di->bam;
    break;
  case 819200:
    di->type = D81;
    di->bam.track = 40;
    di->bam.sector = 1;
    di->bam2.track = 40;
    di->bam2.sector = 2;
    di->dir.track = 40;
    di->dir.sector = 0;
    break;
  default:
    puts("unknown type");
    free(di);
    free(file);
    f_close(file);
    return(NULL);
  }

  di->size = filesize;



  /* allocate buffer for image */
  // if ((di->image = malloc(filesize)) == NULL) {
  //   puts("image malloc failed");
  //   free(di);
  //   f_close(&file);
  //   return(NULL);
  // }

  /* read file into buffer */
  // read = 0;

  // while (read < filesize) {
  //   if (f_read(&file, di->image, 1, &l) == FR_OK) {
  //     read += l;
  //   } else {
  //     puts("fread failed");
  //     free(di->image);
  //     free(di);
  //     f_close(&file);
  //     return(NULL);
  //   }
  // }

  di->image = file;

  di->filename = malloc(strlen(name) + 1);
  strcpy(di->filename, name);

  
  di->openfiles = 0;
  di->blocksfree = blocks_free(di);
  di->modified = 0;
  //f_close(&file);
  set_status(di, 254, 0, 0);
  return(di);
}


DiskImage *di_create_image(char *name, int size) {
  DiskImage *di;

  if ((di = malloc(sizeof(*di))) == NULL) {
    //puts("malloc failed");
    return(NULL);
  }

  /* check image type */
  switch (size) {
  case 174848: // standard D64
  case 175531: // D64 with error info (which we just ignore)
    di->type = D64;
    di->bam.track = 18;
    di->bam.sector = 0;
    di->dir = di->bam;
    break;
  case 349696:
    di->type = D71;
    di->bam.track = 18;
    di->bam.sector = 0;
    di->bam2.track = 53;
    di->bam2.sector = 0;
    di->dir = di->bam;
    break;
  case 819200:
    di->type = D81;
    di->bam.track = 40;
    di->bam.sector = 1;
    di->bam2.track = 40;
    di->bam2.sector = 2;
    di->dir.track = 40;
    di->dir.sector = 0;
    break;
  default:
    //puts("unknown type");
    free(di);
    return(NULL);
  }

  di->size = size;

  /* allocate buffer for image */
  if ((di->image = malloc(size)) == NULL) {
    puts("image malloc failed");
    free(di);
    return(NULL);
  }
  memset(di->image, 0, size);

  di->filename = malloc(strlen(name) + 1);
  strcpy(di->filename, name);
  di->openfiles = 0;
  di->blocksfree = blocks_free(di);
  di->modified = 1;
  set_status(di, 254, 0, 0);
  return(di);
}


void di_sync(DiskImage *di) {
    // No need to sync because zvery change is written to disk
    di->modified = 0;
}


void di_free_image(DiskImage *di) {
  // printf("free image");
  if (di->modified) {
    di_sync(di);
  }
  if (di->filename) {
    free(di->filename);
  }
  f_close(di->image);
  free(di->image);
  free(di);
}


int match_pattern(unsigned char *rawpattern, unsigned char *rawname) {
  int i;

  for (i = 0; i < 16; ++i) {
    if (rawpattern[i] == '*') {
      return(1);
    }
    if (rawname[i] == 0xa0) {
      if (rawpattern[i] == 0xa0) {
	return(1);
      } else {
	return(0);
      }
    } else {
      if (rawpattern[i] == '?' || rawpattern[i] == rawname[i]) {
      } else {
	return(0);
      }
    }
  }
  return(1);
}


RawDirEntry *find_file_entry(DiskImage *di, unsigned char *rawpattern, FileType type) {
  int buffer;
  TrackSector ts;
  RawDirEntry *rde;
  int offset;

  int s = (sizeof(RawDirEntry));

  unsigned char * buff;
  UINT br;


  if ((buff = malloc(s)) == NULL) {
    printf("could not alloc buff");
    return NULL;
  }
  ts = next_ts_in_chain(di, di->bam);
  while (ts.track) {
    buffer = get_ts_addr(di, ts);
    for (offset = 0; offset < 256; offset += 32) {
      f_lseek(di->image, buffer + offset);
      FRESULT r = f_read (di->image, buff, s, &br);
      if (r != FR_OK) {
        return NULL;
      }

      rde = (RawDirEntry *)(buff);
      if ((rde->type & ~0x40) == (type | 0x80)) {
        if (match_pattern(rawpattern, rde->rawname)) {
          return(rde);
        }
      }
    }
    ts = next_ts_in_chain(di, ts);
  }
  return(NULL);
}


RawDirEntry *alloc_file_entry(DiskImage *di, unsigned char *rawname, FileType type) {
  int buffer;
  TrackSector ts, lastts;
  RawDirEntry *rde;
  int offset;
  int s = (sizeof(RawDirEntry));

  unsigned char buff[s];
  UINT br;

  /* check if file already exists */
  ts = next_ts_in_chain(di, di->bam);
  while (ts.track) {
    buffer = get_ts_addr(di, ts);
    for (offset = 0; offset < 256; offset += 32) {
      f_lseek(di->image, buffer + offset);
      FRESULT r = f_read (di->image, buff, s, &br);
      if (r != FR_OK) {
        return NULL;
      }
      rde = (RawDirEntry *)(&buff);
      if (rde->type) {
        if (strncmp(rawname, rde->rawname, 16) == 0) {
          set_status(di, 63, 0, 0);
          puts("file exists");
          return(NULL);
        }
      }
    }
    ts = next_ts_in_chain(di, ts);
  }

  /* allocate empty slot */
  ts = next_ts_in_chain(di, di->bam);
  while (ts.track) {
    buffer = get_ts_addr(di, ts);
    for (offset = 0; offset < 256; offset += 32) {
      f_lseek(di->image, buffer + offset);
      FRESULT r = f_read (di->image, buff, s, &br);
      if (r != FR_OK) {
        return NULL;
      }
      rde = (RawDirEntry *)(&buff);

      if (rde->type == 0) {
        memset((unsigned char *)rde + 2, 0, 30);
        memcpy(rde->rawname, rawname, 16);
        rde->type = type;
        unsigned char * ptr = (unsigned char *)(rde) + 2;
        for(int nn=0;nn < 30; nn++) {
          write_to_image(di, buffer + offset + 2 + nn, *ptr);
          ptr++;
        }
        
        
        di->modified = 1;
        return(rde);
      }
    }
    lastts = ts;
    ts = next_ts_in_chain(di, ts);
  }

  /* allocate new dir block */
  ts = alloc_next_dir_ts(di);
  if (ts.track) {
    rde = (RawDirEntry *)get_ts_addr(di, ts);
    memset((unsigned char *)rde + 2, 0, 30);
    memcpy(rde->rawname, rawname, 16);
    rde->type = type;
    di->modified = 1;
    return(rde);
  } else {
    set_status(di, 72, 0, 0);
    //puts("directory full");
    return(NULL);
  }
}


/* open a file */
ImageFile *di_open(DiskImage *di, unsigned char *name, FileType type, char *mode) {
  ImageFile *imgfile;
  RawDirEntry *rde;
  int p;

  char newname[17];
  unsigned char rawname[17];
  strncpy(newname, name, 16);

  newname[16] = 0;
  atop(newname);

  di_rawname_from_name(rawname, newname);

  set_status(di, 255, 0, 0);

  if (strcmp("rb", mode) == 0) {
    if ((imgfile = malloc(sizeof(*imgfile))) == NULL) {
      puts("malloc failed");
      return(NULL);
    }
    if (strcmp("$", name) == 0) {
      imgfile->mode = 'r';
      imgfile->ts = di->dir;
      p = get_ts_addr(di, di->dir);

      if ((di->type == D64) || (di->type == D71)) {
        imgfile->nextts.track = 18; // 1541/71 ignores bam t/s link
        imgfile->nextts.sector = 1;
      } else {
        imgfile->nextts.track = read_from_image(di, p);
        imgfile->nextts.sector = read_from_image(di, p + 1);
      }
      imgfile->buflen = 254;
      unsigned char * file_contents;
      if ((file_contents = malloc(imgfile->buflen)) == NULL) {
        printf("file buffer alloc fail");
        return(NULL);
      }

      UINT br;
      f_read(di->image, file_contents, imgfile->buflen, &br);

      imgfile->buffer = file_contents;
      rde = NULL;
    } else {
      if ((rde = find_file_entry(di, rawname, type)) == NULL) {
        set_status(di, 62, 0, 0);
        puts("find_file_entry failed");
        free(imgfile);
        return(NULL);
      }
      imgfile->mode = 'r';
      imgfile->ts = rde->startts;
      if (imgfile->ts.track > di_tracks(di->type)) {
	      return(NULL);
      }
      p = get_ts_addr(di, rde->startts);
      
      imgfile->nextts.track = read_from_image(di, p);
      imgfile->nextts.sector = read_from_image(di, p+1);
      if (imgfile->nextts.track == 0) {
	      imgfile->buflen = imgfile->nextts.sector - 1;
      } else {
	      imgfile->buflen = 254;
      }
      unsigned char * file_contents;
      if ((file_contents = malloc(imgfile->buflen)) == NULL) {
        // printf("file buffer alloc fail");
        return(NULL);
      }
      UINT br;
      f_read(di->image, file_contents, imgfile->buflen, &br);
      imgfile->buffer = file_contents;
    }
  } else if (strcmp("wb", mode) == 0) {
    if ((rde = alloc_file_entry(di, rawname, type)) == NULL) {
      puts("alloc_file_entry failed");
      return(NULL);
    }
    if ((imgfile = malloc(sizeof(*imgfile))) == NULL) {
      puts("malloc failed");
      return(NULL);
    }
    imgfile->mode = 'w';
    imgfile->ts.track = 0;
    imgfile->ts.sector = 0;
    if ((imgfile->buffer = malloc(254)) == NULL) {
      free(imgfile);
      puts("malloc failed imgfile");
      return(NULL);
    }
    imgfile->buflen = 254;
    di->modified = 1;

  } else {
    return(NULL);
  }

  imgfile->diskimage = di;
  imgfile->rawdirentry = rde;
  imgfile->position = 0;
  imgfile->bufptr = 0;

  ++(di->openfiles);
  set_status(di, 0, 0, 0);
  return(imgfile);
}


int di_read(ImageFile *imgfile, unsigned char *buffer, int len) {
  int p;
  int bytesleft;
  int counter = 0;

  while (len) {
    // printf("loop 1 length is %d\n", len);
    bytesleft = imgfile->buflen - imgfile->bufptr;
    // printf("loop 1 bytesleft is %d\n", bytesleft);
    if (bytesleft == 0) {
      if (imgfile->nextts.track == 0) {
	      return(counter);
      }
      if (((imgfile->diskimage->type == D64) || (imgfile->diskimage->type == D71)) && imgfile->ts.track == 18 && imgfile->ts.sector == 0) {
        imgfile->ts.track = 18;
        imgfile->ts.sector = 1;
      } else {
	      imgfile->ts = next_ts_in_chain(imgfile->diskimage, imgfile->ts);
      }
      if (imgfile->ts.track == 0) {
	      return(counter);
      }
      p = get_ts_addr(imgfile->diskimage, imgfile->ts);

      // printf("p is %ld\n", p);
      // imgfile->buffer = p + 2;
      imgfile->nextts.track = read_from_image(imgfile->diskimage, p);
      imgfile->nextts.sector = read_from_image(imgfile->diskimage, p + 1);
      // printf("read TS\n");
      if (imgfile->nextts.track == 0) {
	      imgfile->buflen = imgfile->nextts.sector - 1;
      } else {
	      imgfile->buflen = 254;
      }
      imgfile->bufptr = 0;

      unsigned char * file_contents;
      if ((file_contents = malloc(imgfile->buflen)) == NULL) {
        // printf("file buffer alloc fail\n");
        return(0);
      }
      // printf("file buffer alloc ok\n");
      for (int nn = 0; nn < imgfile->buflen; nn++) {
        file_contents[nn] = read_from_image(imgfile->diskimage, p + 2 + nn);
      }
      // printf("setting file buffer to content\n");
      imgfile->buffer = file_contents;
    } else {
      if (len >= bytesleft) {
        while (bytesleft) {
          // printf("loop 2 bytesleft is %d\n", bytesleft);
          unsigned char res = imgfile->buffer[imgfile->bufptr++];
          // printf("byte read from buffer is %d\n", res);
          *buffer++ = res;
          --len;
          --bytesleft;
          ++counter;
          ++(imgfile->position);
        }
      } else {
        while (len) {
          // printf("loop 2 len is %d\n", len);
          unsigned char res = imgfile->buffer[imgfile->bufptr++];
          // printf("byte read from file is %d", res);
          *buffer++ = res;          
          --len;
          --bytesleft;
          ++counter;
          ++(imgfile->position);
        }
      }
    }
  }
  return(counter);
}


int di_write(ImageFile *imgfile, unsigned char *buffer, int len) {
  int p;
  int bytesleft;
  int counter = 0;

  while (len) {
    bytesleft = imgfile->buflen - imgfile->bufptr;
    if (bytesleft == 0) {
      if (imgfile->diskimage->blocksfree == 0) {
        set_status(imgfile->diskimage, 72, 0, 0);
        return(counter);
      }
      imgfile->nextts = alloc_next_ts(imgfile->diskimage, imgfile->ts);
      if (imgfile->ts.track == 0) {
        imgfile->rawdirentry->startts = imgfile->nextts;
      } else {
        p = get_ts_addr(imgfile->diskimage, imgfile->ts);
        write_to_image(imgfile->diskimage, p, imgfile->nextts.track);
        // p[0] = imgfile->nextts.track;
        write_to_image(imgfile->diskimage, p + 1, imgfile->nextts.track);
        // p[1] = imgfile->nextts.sector;
      }
      imgfile->ts = imgfile->nextts;
      p = get_ts_addr(imgfile->diskimage, imgfile->ts);
      write_to_image(imgfile->diskimage, p, 0);
      // p[0] = 0;
      write_to_image(imgfile->diskimage, p + 1, 0xFF);
      // p[1] = 0xff;

      //memcpy(p + 2, imgfile->buffer, 254);
      imgfile->bufptr = 0;
      if (++(imgfile->rawdirentry->sizelo) == 0) {
	      ++(imgfile->rawdirentry->sizehi);
      }
      --(imgfile->diskimage->blocksfree);
    } else {
      if (len >= bytesleft) {
        while (bytesleft) {
          write_to_image(imgfile->diskimage, (int)(imgfile->buffer + imgfile->bufptr++), *buffer++);
          // imgfile->buffer[imgfile->bufptr++] = *buffer++;
          --len;
          --bytesleft;
          ++counter;
          ++(imgfile->position);
        }
      } else {
        while (len) {
          write_to_image(imgfile->diskimage, (int)(imgfile->buffer + imgfile->bufptr++), *buffer++);
          // imgfile->buffer[imgfile->bufptr++] = *buffer++;
          --len;
          --bytesleft;
          ++counter;
          ++(imgfile->position);
        }
      }
    }
  }
  return(counter);
}


void di_close(ImageFile *imgfile) {
  int p;

  if (imgfile->mode == 'w') {
    if (imgfile->bufptr) {
      if (imgfile->diskimage->blocksfree) {
	imgfile->nextts = alloc_next_ts(imgfile->diskimage, imgfile->ts);
	if (imgfile->ts.track == 0) {
	  imgfile->rawdirentry->startts = imgfile->nextts;
	} else {
	  p = get_ts_addr(imgfile->diskimage, imgfile->ts);
    write_to_image(imgfile->diskimage, p, imgfile->nextts.track);
	  // p[0] = imgfile->nextts.track;
    write_to_image(imgfile->diskimage, p + 1, imgfile->nextts.sector);
	  // p[1] = imgfile->nextts.sector;
	}
	imgfile->ts = imgfile->nextts;
	p = get_ts_addr(imgfile->diskimage, imgfile->ts);
  write_to_image(imgfile->diskimage, p, 0);
	// p[0] = 0;
  write_to_image(imgfile->diskimage, p, 0xFF);
	// p[1] = 0xff;
	//memcpy(p + 2, imgfile->buffer, 254);
	imgfile->bufptr = 0;
	if (++(imgfile->rawdirentry->sizelo) == 0) {
	  ++(imgfile->rawdirentry->sizehi);
	}
	--(imgfile->diskimage->blocksfree);
	imgfile->rawdirentry->type |= 0x80;
      }
    } else {
      imgfile->rawdirentry->type |= 0x80;
    }
    free(imgfile->buffer);
  }
  --(imgfile->diskimage->openfiles);
  free(imgfile);
}


int di_format(DiskImage *di, unsigned char *rawname, unsigned char *rawid) {
  unsigned char *p;
  TrackSector ts;

  di->modified = 1;

  switch (di->type) {

  case D64:
    /* erase disk */
    if (rawid) {
      memset(di->image, 0, 174848);
    }

    /* get ptr to bam */
    p = get_ts_addr(di, di->bam);

    /* setup header */
    p[0] = 18;
    p[1] = 1;
    p[2] = 'A';
    p[3] = 0;

    /* clear bam */
    memset(p + 4, 0, 0x8c);

    /* free blocks */
    for (ts.track = 1; ts.track <= di_tracks(di->type); ++ts.track) {
      for (ts.sector = 0; ts.sector < di_sectors_per_track(di->type, ts.track); ++ts.sector) {
	di_free_ts(di, ts);
      }
    }

    /* allocate bam and dir */
    ts.track = 18;
    ts.sector = 0;
    di_alloc_ts(di, ts);
    ts.sector = 1;
    di_alloc_ts(di, ts);

    /* copy name */
    memcpy(p + 0x90, rawname, 16);

    /* set id */
    memset(p + 0xa0, 0xa0, 2);
    if (rawid) {
      memcpy(p + 0xa2, rawid, 2);
    }
    memset(p + 0xa4, 0xa0, 7);
    p[0xa5] = '2';
    p[0xa6] = 'A';

    /* clear unused bytes */
    memset(p + 0xab, 0, 0x55);

    /* clear first dir block */
    memset(p + 256, 0, 256);
    p[257] = 0xff;
    break;

  case D71:
    /* erase disk */
    if (rawid) {
      memset(di->image, 0, 349696);
    }

    /* get ptr to bam2 */
    p = get_ts_addr(di, di->bam2);

    /* clear bam2 */
    memset(p, 0, 256);

    /* get ptr to bam */
    p = get_ts_addr(di, di->bam);

    /* setup header */
    p[0] = 18;
    p[1] = 1;
    p[2] = 'A';
    p[3] = 0x80;

    /* clear bam */
    memset(p + 4, 0, 0x8c);

    /* clear bam2 counters */
    memset(p + 0xdd, 0, 0x23);

    /* free blocks */
    for (ts.track = 1; ts.track <= di_tracks(di->type); ++ts.track) {
      if (ts.track != 53) {
	for (ts.sector = 0; ts.sector < di_sectors_per_track(di->type, ts.track); ++ts.sector) {
	  di_free_ts(di, ts);
	}
      }
    }

    /* allocate bam and dir */
    ts.track = 18;
    ts.sector = 0;
    di_alloc_ts(di, ts);
    ts.sector = 1;
    di_alloc_ts(di, ts);

    /* copy name */
    memcpy(p + 0x90, rawname, 16);

    /* set id */
    memset(p + 0xa0, 0xa0, 2);
    if (rawid) {
      memcpy(p + 0xa2, rawid, 2);
    }
    memset(p + 0xa4, 0xa0, 7);
    p[0xa5] = '2';
    p[0xa6] = 'A';

    /* clear unused bytes */
    memset(p + 0xab, 0, 0x32);

    /* clear first dir block */
    memset(p + 256, 0, 256);
    p[257] = 0xff;
    break;

  case D81:
    /* erase disk */
    if (rawid) {
      memset(di->image, 0, 819200);
    }

    /* get ptr to bam */
    p = get_ts_addr(di, di->bam);

    /* setup header */
    p[0] = 0x28;
    p[1] = 0x02;
    p[2] = 0x44;
    p[3] = 0xbb;
    p[6] = 0xc0;

    /* set id */
    if (rawid) {
      memcpy(p + 4, rawid, 2);
    }

    /* clear bam */
    memset(p + 7, 0, 0xfa);

    /* get ptr to bam2 */
    p = get_ts_addr(di, di->bam2);

    /* setup header */
    p[0] = 0x00;
    p[1] = 0xff;
    p[2] = 0x44;
    p[3] = 0xbb;
    p[6] = 0xc0;

    /* set id */
    if (rawid) {
      memcpy(p + 4, rawid, 2);
    }

    /* clear bam2 */
    memset(p + 7, 0, 0xfa);

    /* free blocks */
    for (ts.track = 1; ts.track <= di_tracks(di->type); ++ts.track) {
      for (ts.sector = 0; ts.sector < di_sectors_per_track(di->type, ts.track); ++ts.sector) {
	di_free_ts(di, ts);
      }
    }

    /* allocate bam and dir */
    ts.track = 40;
    ts.sector = 0;
    di_alloc_ts(di, ts);
    ts.sector = 1;
    di_alloc_ts(di, ts);
    ts.sector = 2;
    di_alloc_ts(di, ts);
    ts.sector = 3;
    di_alloc_ts(di, ts);

    /* get ptr to dir */
    p = get_ts_addr(di, di->dir);

    /* copy name */
    memcpy(p + 4, rawname, 16);

    /* set id */
    memset(p + 0x14, 0xa0, 2);
    if (rawid) {
      memcpy(p + 0x16, rawid, 2);
    }
    memset(p + 0x18, 0xa0, 5);
    p[0x19] = '3';
    p[0x1a] = 'D';

    /* clear unused bytes */
    memset(p + 0x1d, 0, 0xe3);

    /* clear first dir block */
    memset(p + 768, 0, 256);
    p[769] = 0xff;
    break;

  }

  di->blocksfree = blocks_free(di);

  return(set_status(di, 0, 0, 0));
}


int di_delete(DiskImage *di, unsigned char *rawpattern, FileType type) {
  RawDirEntry *rde;
  int delcount = 0;

  switch (type) {

  case T_SEQ:
  case T_PRG:
  case T_USR:
    while ((rde = find_file_entry(di, rawpattern, type))) {
      free_chain(di, rde->startts);
      rde->type = 0;
      ++delcount;
    }
    if (delcount) {
      return(set_status(di, 1, delcount, 0));
    } else {
      return(set_status(di, 62, 0, 0));
    }
    break;

  default:
    return(set_status(di, 64, 0, 0));
    break;

  }
}


int di_rename(DiskImage *di, unsigned char *oldrawname, unsigned char *newrawname, FileType type) {
  RawDirEntry *rde;

  if ((rde = find_file_entry(di, oldrawname, type))) {
    memcpy(rde->rawname, newrawname, 16);
    return(set_status(di, 0, 0, 0));
  } else {
    return(set_status(di, 62, 0, 0));
  }
}
