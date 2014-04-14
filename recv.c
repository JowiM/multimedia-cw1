#include "interpolate.h"
#include "assert.h"

/* write a packet's worth of silence */
void write_silence(FILE *ofile, int num_samples) {
  short missing_pkt[num_samples];
  int i;
  for (i=0; i < num_samples; i++) {
    missing_pkt[i]=0;
  }
  fwrite(missing_pkt, 1, num_samples*2, ofile);
}

/* simulate the reception of a packet, and apply a loss repair strategy */
void recv_packet(int seqno, int len, char *data, FILE *ofile, int strategy) {
  static int prev_seqno = -1;
  static short* prev_samples = 0;

  /* interpret the data as signed 16 bit integers */
  short *samples = (short*)data; 
  int num_samples = len/2;

  printf("recv_packet: seqno=%d\n", seqno);

  if (prev_seqno != -1 && (prev_seqno+1 != seqno)) {
    int i, missing_seqno;
    /* there was missing data */

    printf("s=%d\n", strategy);
    switch(strategy) {
    case SILENCE: 
      {
	/* create a packet containing silence */
	missing_seqno = prev_seqno + 1;
	while (missing_seqno < seqno) {
	  write_silence(ofile, num_samples);
	  missing_seqno++;
	}
	break;
      }
    case REPEAT_PREV: 
      {
	/* repeat the previous packet */
	fwrite(prev_samples, 2, num_samples, ofile);
	missing_seqno = prev_seqno + 2;
	while (missing_seqno < seqno) {
	  /* repeating the same packet more than once sounds bad */
	  write_silence(ofile, num_samples);
	  missing_seqno++;
	}
	break;
      }
    case REPEAT_PREV_NEXT: 
      {
	/****************************************************************/
	/**                     your code goes here                    **/
	/****************************************************************/
	break;
      }
    case INTERPOLATE:
      {
	/****************************************************************/
	/**                     your code goes here                    **/
	/****************************************************************/
	break;
      }
    default:
      abort();
    }
  }

  fwrite(samples, 1, num_samples*2, ofile);
  
  /* hold onto the last received packet - we may need it */
  if (prev_samples != 0)
    free(prev_samples);
  prev_samples = samples;
  prev_seqno = seqno;
};
