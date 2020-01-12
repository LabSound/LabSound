typedef struct {
    SPFLOAT aOut[1];
    SPFLOAT aIn;
    SPFLOAT iPartLen;
    SPFLOAT iSkipSamples;
    SPFLOAT iTotLen;
    int initDone;
    int nChannels;
    int cnt;
    int nPartitions;
    int partSize;
    int rbCnt;
    SPFLOAT *tmpBuf;
    SPFLOAT *ringBuf;
    SPFLOAT *IR_Data[1];
    SPFLOAT *outBuffers[1];
    sp_auxdata auxData;
    sp_ftbl *ftbl;
    sp_fft fft;
} sp_conv;

int sp_conv_create(sp_conv **p);
int sp_conv_destroy(sp_conv **p);
int sp_conv_init(sp_data *sp, sp_conv *p, sp_ftbl *ft, SPFLOAT iPartLen);
int sp_conv_compute(sp_data *sp, sp_conv *p, SPFLOAT *in, SPFLOAT *out);
