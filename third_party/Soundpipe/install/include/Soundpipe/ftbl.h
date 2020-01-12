#define SP_FT_MAXLEN 0x1000000L
#define SP_FT_PHMASK 0x0FFFFFFL

typedef struct sp_ftbl{
    size_t size;
    uint32_t lobits;
    uint32_t lomask;
    SPFLOAT lodiv;
    SPFLOAT sicvt;
    SPFLOAT *tbl;
    char del;
}sp_ftbl;

int sp_ftbl_create(sp_data *sp, sp_ftbl **ft, size_t size);
int sp_ftbl_init(sp_data *sp, sp_ftbl *ft, size_t size);
int sp_ftbl_bind(sp_data *sp, sp_ftbl **ft, SPFLOAT *tbl, size_t size);
int sp_ftbl_destroy(sp_ftbl **ft);
int sp_gen_vals(sp_data *sp, sp_ftbl *ft, const char *string);
int sp_gen_sine(sp_data *sp, sp_ftbl *ft);
int sp_gen_file(sp_data *sp, sp_ftbl *ft, const char *filename);
int sp_gen_sinesum(sp_data *sp, sp_ftbl *ft, const char *argstring);
int sp_gen_line(sp_data *sp, sp_ftbl *ft, const char *argstring);
int sp_gen_xline(sp_data *sp, sp_ftbl *ft, const char *argstring);
int sp_gen_gauss(sp_data *sp, sp_ftbl *ft, SPFLOAT scale, uint32_t seed);
int sp_ftbl_loadfile(sp_data *sp, sp_ftbl **ft, const char *filename);
int sp_ftbl_loadspa(sp_data *sp, sp_ftbl **ft, const char *filename);
int sp_gen_composite(sp_data *sp, sp_ftbl *ft, const char *argstring);
int sp_gen_rand(sp_data *sp, sp_ftbl *ft, const char *argstring);
int sp_gen_triangle(sp_data *sp, sp_ftbl *ft);
