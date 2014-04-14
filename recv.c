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

void print_raw_packetbuf(short* prev_samples, int len){
  printf("LETS SEE! - ");
  int i;
  for(i = 0; i < len; i++) {
    printf("%02x ", *(char *)(prev_samples + i));
  }
  printf("\n");
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
      case SILENCE: {
	         /* create a packet containing silence */
	         missing_seqno = prev_seqno + 1;
	         while (missing_seqno < seqno) {
	           write_silence(ofile, num_samples);
	           missing_seqno++;
	         }
	         break;
        }
      case REPEAT_PREV:{
          print_raw_packetbuf(prev_samples, num_samples);
          print_raw_packetbuf(prev_samples+(num_samples/2), num_samples/2);
          printf("FIX THIS!! - %02x - %02x - numb samples - %d \n", *prev_samples, *prev_samples+(num_samples/2), num_samples/2);
	         /* repeat the previous packet */
          fwrite(prev_samples, 2, num_samples, ofile);
        	missing_seqno = prev_seqno + 2;
          printf("seqno - %d prev_seqno - %d - missing_seqno - %d \n", seqno, prev_seqno, missing_seqno);
        	while (missing_seqno < seqno) {
            printf("loop!! \n");
        	  /* repeating the same packet more than once sounds bad */
        	  write_silence(ofile, num_samples);
        	  missing_seqno++;
        	}
        	break;
        }
      case REPEAT_PREV_NEXT:{
        	int pkt_distance = seqno - prev_seqno;

          //Just one packet lost
          //one packet loss
          if(pkt_distance == 2){
            printf("ONE PACKET LOSS \n");
            //Write half of previous
            fwrite(prev_samples+(num_samples/2), 2, num_samples/2, ofile);
            //Write half of next
            fwrite(samples, 1, num_samples, ofile);
            print_raw_packetbuf(prev_samples+(num_samples/2), num_samples/2);
            print_raw_packetbuf(samples, num_samples);
          }

          //two packet loss
          if(pkt_distance == 3){
            printf("TWO PACKET LOSS \n");
            fwrite(prev_samples, 2, num_samples, ofile);
            fwrite(samples, 1, num_samples*2, ofile);
          }

          //more then 3 packet loss
          if(pkt_distance >= 4){
            printf("THREE PACKET LOSS \n");
            fwrite(prev_samples, 2, num_samples, ofile);
            missing_seqno = prev_seqno + 2;
            while(missing_seqno < seqno-1){
              printf("Silence!! \n");
              write_silence(ofile, num_samples);
              missing_seqno++;
            }
            //@todo check why 1byte used here instead of 2.
            fwrite(samples, 1, num_samples*2, ofile);
          }

          break;
        }
      case INTERPOLATE:{
        short *dct_sample;
        int *dct_result, *result;
        int pkt_size = num_samples*2;
        int pkt_distance = seqno - prev_seqno;
        pkt_distance -= 1;
        printf("PKT DISTANCE %d \n", pkt_distance);
        int i = 0, j = 0, tmp = 0, sample_pos = 0, numb_interpolation = 0, t_packets = 0;
        short* idct_val = 0;

        result = malloc(pkt_size*sizeof(int));
        dct_sample = malloc(pkt_size*sizeof(short));
        memcpy(dct_sample, prev_samples, num_samples*sizeof(int));
        memcpy((dct_sample+num_samples*sizeof(int)), samples, num_samples*sizeof(int));

        dct_result = dct(prev_samples);
        t_packets = pkt_size*numb_interpolation;
        numb_interpolation = pkt_distance;
        if(pkt_distance > 3){
          numb_interpolation = 3;
        }
        printf("INTERPOLATED PACKETS: %d \n", numb_interpolation);
        for(i = 0; i < numb_interpolation; i++){
          printf("PKT - %d \n", i);
          for(j = 0; j < pkt_size; j++){
            if(tmp == numb_interpolation){
              tmp = 0;
              sample_pos += 1;
              printf("SAMPLE POS: %d - result: %d \n", sample_pos, dct_result[sample_pos]);
            }
            result[j] = dct_result[sample_pos];
            //(int)(dct_result[sample_pos]+((dct_result[sample_pos+1]-dct_result[sample_pos])/pkt_size)*(pkt_size - pkt_size-sample_pos));
            printf("LEts see %d \n",result[j]);
            tmp += 1;
          }
          idct_val = idct(dct_result);
          fwrite(idct_val, 2, num_samples, ofile);
          //clean memory
          memset(result, 0, num_samples*sizeof(int));
        }

        if(pkt_distance > 3){
          printf("GREATER the 3 \n");
          /*while(missing_seqno < seqno-1){
              printf("Silence!! \n");
              write_silence(ofile, num_samples);
              missing_seqno++;
            }*/
        }
      
        free(dct_sample);
        free(result);
        free(dct_result);
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
