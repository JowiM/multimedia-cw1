#include "interpolate.h"

/* uncommenting this will print out a histogram of the values in the input data - only intended for debugging */
#define PRINTHIST

main(int argc, char **argv) {
  FILE *ifile;
  FILE *ofile;
  char *dbuf;
  int msPerPacket, bytesPerPacket, i, seqno, filesize, size;
  int samplesPerPacket;
  int hist[32], j;
  short *samples;

  int strategy = REPAIR_STRATEGY;

  ifile = fopen("in.wav", "r");
  if (ifile == 0) {
    fprintf(stderr, "File not found\n");
    exit(1);
  }
  ofile = fopen("out.wav", "w+");
  if (ofile == 0) {
    fprintf(stderr, "File creation failed\n");
    exit(1);
  }

  msPerPacket = MS_PER_PACKET;
  filesize = process_wav_header(ifile, ofile, msPerPacket, &bytesPerPacket);

  /* Assume 16 bit linear data */
  samplesPerPacket = bytesPerPacket/2;

  /* initialize the DCT for later usage.  If you want to run it on
     less than a whole packet, you'll need to change this */
  init_dct(samplesPerPacket);

#ifdef PRINTHIST
  for (j=0; j < 32; j++) {
    hist[j]=0;
  }
#endif

  dbuf = malloc(bytesPerPacket);
  for (i=0; i < filesize / bytesPerPacket; i++) {
    printf("%d ", i);
    fflush(stdout);
    size = fread(dbuf, 1, bytesPerPacket, ifile);
    if (size < bytesPerPacket) {
      printf("EOF\n");
      exit(0);
    }
#ifdef PRINTHIST
    samples = (unsigned short*)dbuf;
    for (j=0; j < bytesPerPacket/2; j++) {
      hist[(samples[j]/2048) + 16]++;
    }
#endif
    send_packet(seqno++, bytesPerPacket, dbuf, ofile, strategy);
  }
  
#ifdef PRINTHIST
  for (j=0; j < 32; j++) {
    printf("%d: %d\n", j-16, hist[j]);
  }
#endif
}

