typedef struct {
    int mti;
    /* do not change value 624 */
    uint32_t mt[624];
} sp_randmt;

void sp_randmt_seed(sp_randmt *p,
    const uint32_t *initKey, uint32_t keyLength);

uint32_t sp_randmt_compute(sp_randmt *p);
