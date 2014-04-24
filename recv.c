#include "interpolate.h"
#include "assert.h"


void linearInterpolateBuffer(float* previousFrame, int numChannels, float* input, int inNumFrames, float* output, int outNumFrames)
{
  int i, j, index;
  double distance, prevValue, nextValue;

  for(i=0; i<outNumFrames; i++)
    {
      for(j=0; j<numChannels; j++)
        {
          distance = i * (inNumFrames / (double)outNumFrames);
          index = ((int)distance) * numChannels + j;
          nextValue = input[index];
          prevValue = distance < 1 ? previousFrame[j] : input[index-numChannels];
          distance -= (int)distance;
          output[i*numChannels+j] = (nextValue-prevValue) * distance + prevValue;
        }
    }
  for(j=0; j<numChannels; j++)
    previousFrame[j] = input[(inNumFrames - 1) * numChannels + j];
}

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
          int pkt_distance = (seqno - prev_seqno)-1;

          //Just one packet lost
          //one packet loss
          if(pkt_distance == 1){
            printf("ONE PACKET LOSS \n");
            //Write half of previous
            fwrite(prev_samples+(num_samples/2), 2, num_samples/2, ofile);
            //Write half of next
            fwrite(samples, 1, num_samples, ofile);
            print_raw_packetbuf(prev_samples+(num_samples/2), num_samples/2);
            print_raw_packetbuf(samples, num_samples);
          }

          //two packet loss
          if(pkt_distance == 2){
            printf("TWO PACKET LOSS \n");
            fwrite(prev_samples, 2, num_samples, ofile);
            fwrite(samples, 1, num_samples*2, ofile);
          }

          //more then 3 packet loss
          if(pkt_distance >= 3){
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
        int *dct_result, *new_packet;
        short *idct_result;
        int pkt_size = num_samples*2;
        int i, j, distance, tmp_distance, index, next_val, prev_val, numb_interpolation;
        double difference;

        distance = (seqno - prev_seqno) - 1;
        printf("DISTANCE: %d \n", distance);
        tmp_distance = distance;

        new_packet = malloc(pkt_size*sizeof(int));

        dct_result = dct(prev_samples);
        numb_interpolation = distance;
        if(distance > 3){
          numb_interpolation = 3;
        }

        index = 0;
        for(i = 0; i < numb_interpolation; i++){
          printf("Dis %d \n", i);
          for(j = 0; j < pkt_size; j++){
            prev_val = dct_result[index];

            //If 1 no calculations is done :) & copy correct packet
            if(tmp_distance == numb_interpolation){
              new_packet[j] = prev_val;
            } else {
              next_val = dct_result[index+1];
              difference = 1.0/numb_interpolation;
              if(difference > 200 || difference < -200){
                new_packet[j] = prev_val;
              } else {
                new_packet[j] = (int)(((next_val - prev_val)*difference) + prev_val);
              }
              printf("SAVED: %d \n", new_packet[j]);
            }

            tmp_distance -= 1;
            if(tmp_distance < 1){
              tmp_distance = numb_interpolation;
              index += 1;
            }
          }

          idct_result = idct(new_packet);
          printf("WRITING \n");
          fwrite(idct_result, 1, pkt_size, ofile);

          //clean memory
          memset(new_packet, 0, pkt_size);
          free(idct_result);
        }

        free(new_packet);
        free(dct_result);

        if(distance > 3){
          tmp_distance = prev_seqno + distance;
          while(tmp_distance < seqno-1){
              printf("Silence!! \n");
              write_silence(ofile, num_samples);
              tmp_distance++;
          }
        }
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
