
#ifndef H264_H_
#define H264_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PACKET_BUFFER_END      (unsigned int)0x00000000
#define MAX_RTP_PKT_LENGTH     1360
#define H264                    96


#define SSRC           100
#define DEST_IP_STR   "127.0.0.1"
#define DEST_PORT     12500
#define BASE_PORT     9400


typedef struct 
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /**//* expect 0 */
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */
    unsigned char padding:1;        /**//* expect 0 */
    unsigned char version:2;        /**//* expect 2 */
    /**//* byte 1 */
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */
    unsigned char marker:1;        /**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short seq_no;            
    /**//* bytes 4-7 */
    unsigned  long timestamp;        
    /**//* bytes 8-11 */
    unsigned long ssrc;            /**//* stream number is used here. */
} RTP_FIXED_HEADER;

typedef struct {
    //byte 0
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;    
         
} NALU_HEADER; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;    
            
             
} FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER; /**//* 1 BYTES */
typedef struct
{
  int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  long len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  long actual_len;            //! Nal Unit Buffer size
  int max_size;
  unsigned char forbidden_bit:1;            //! should be always FALSE
  unsigned char  nal_NRI:2;        //! NALU_PRIORITY_xxxx
  unsigned char nal_unit_type:5;            //! NALU_TYPE_xxxx
  unsigned char *buf;                    //! contains the first byte followed by the EBSP
} NALU_t;






//static bool flag = true;
static int info2=0, info3=0;
RTP_FIXED_HEADER        *rtp_hdr;
BYTE *bits = NULL;                //!< the bit stream file
NALU_HEADER		*nalu_hdr;
FU_INDICATOR	*fu_ind;
FU_HEADER		*fu_hdr;




//查找开始字符0x000001
static int FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //判断是否为0x000001,如果是返回1
	else return 1;
}

//查找开始字符0x00000001
static int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//判断是否为0x00000001,如果是返回1
	else return 1;
}


void OpenBitstreamFile (byte **fn,long s)
{
	bits = *fn;
}


//为NALU_t结构体分配内存空间
NALU_t *AllocNALU()
{
	NALU_t *n;
	if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
	{
		printf("AllocNALU: n");
		exit(0);
	}

	n->max_size=614400;

	if ((n->buf = (unsigned char*)calloc(614400, sizeof (char))) == NULL) //默认80000字节的大小，太吓人了
	{
		free (n);
		printf ("AllocNALU: n->buf");
		exit(0);
	}

	return n;
}


//释放
void FreeNALU(NALU_t *n)
{
	if (n)
	{
		if (n->buf)
		{
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}
}

#endif
