void sp_fft_init(sp_fft *fft, int M)
{
    SPFLOAT *utbl;
    int16_t *BRLow;
    int16_t *BRLowCpx;
    int i;

    /* init cos table */
    utbl = (SPFLOAT*) malloc((POW2(M) / 4 + 1) * sizeof(SPFLOAT));
    fftCosInit(M, utbl);

    BRLowCpx =
      (int16_t*) malloc(POW2(M / 2 - 1) * sizeof(int16_t));
    fftBRInit(M, BRLowCpx);

    /* init bit reversed table for real FFT */
     BRLow =
      (int16_t*) malloc(POW2((M - 1) / 2 - 1) * sizeof(int16_t));
    fftBRInit(M - 1, BRLow);

    fft->BRLow = BRLow;
    fft->BRLowCpx = BRLowCpx;
    fft->utbl = utbl;
}

void sp_fftr(sp_fft *fft, SPFLOAT *buf, int FFTsize)
{
    SPFLOAT *Utbl;
    int16_t *BRLow;
    int   M = log2(FFTsize);
    rffts1(buf, M, fft->utbl, fft->BRLow);
}

void sp_fft_cpx(sp_fft *fft, SPFLOAT *buf, int FFTsize)
{
    SPFLOAT *Utbl;
    int16_t *BRLow;
    int   M = log2(FFTsize);
    ffts1(buf, M, fft->utbl, fft->BRLowCpx);
}



void sp_ifftr(sp_fft *fft, SPFLOAT *buf, int FFTsize)
{
    SPFLOAT *Utbl;
    int16_t *BRLow;
    int   M = log2(FFTsize);
    riffts1(buf, M, fft->utbl, fft->BRLow);
}

void sp_fft_destroy(sp_fft *fft)
{
    free(fft->utbl);
    free(fft->BRLow);
    free(fft->BRLowCpx);
}
