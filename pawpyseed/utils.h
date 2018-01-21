#ifndef UTILS_H
#define UTILS_H
#include <complex.h>
#include <math.h>

typedef struct band {
	int n;
	int num_waves;
	double occ;
	double N;
	double complex energy;
	float complex* Cs;
} band_t;

typedef struct kpoint {
	short int up;
	int* Gs;
	double* k;
	double weight;
	int num_bands;
	band_t** bands;
} kpoint_t;

typedef struct pswf {
	int* G_bounds;
	kpoint_t** kpts;
	int nspin;
	int nband;
	int nwk;
	double* lattice;
	double* reclattice;
} pswf_t;

typedef struct proj_ae_ps {
	int l;
	double* proj;
	double** proj_spline;
	double* aewave;
	double* pswave;
} funcset_t;

typedef struct projgrid {
	double complex* values;
} projgrid_t;

typedef struct real_proj {
	int l;
	int m;
	int func_num;
	double complex* values;
} real_proj_t;

typedef struct real_proj_site {
	int index;
	int elem;
	int num_projs;
	int total_projs;
	int num_indices;
	double rmax;
	double* coord;
	int* indices;
	real_proj_t* projs;
} real_proj_site_t;

typedef struct pseudopot {
	int num_projs;
	int total_projs;
	funcset_t* funcs;
	double rmax;
	double* pspw_overlap_matrix;
	double* aepw_overlap_matrix;
	double* diff_overlap_matrix;
	int proj_gridsize;
	int wave_gridsize;
	int num_cart_gridpts;
	double* wave_grid;
	double* proj_grid;
} ppot_t;

int min(int a, int b);

void vcross(double* res, double* top, double* bottom);

double dot(double* x1, double* x2);

double mag(double* x1);

double determinant(double* m);

double dist_from_frac(double* coords1, double* coords2, double* lattice);

void frac_to_cartesian(double* coord, double* lattice);

void cartesian_to_frac(double* coord, double* reclattice);

void min_cart_path(double* coord, double* center, double* lattice, double* path, double* r);

double complex trilinear_interpolate(double complex* c, double* frac, int* fftg);

void free_kpoint(kpoint_t* kpt);

void free_ppot(ppot_t* pp);

void free_real_proj(real_proj_t* proj);

void free_real_proj_site(real_proj_site_t* site);

void free_pswf(pswf_t* wf);

void free_ptr(void* ptr);

void free_real_proj_site_list(real_proj_site_t* sites, int length);

void free_ppot_list(ppot_t* pps, int length);

double* get_occs(pswf_t* wf);

int get_nband(pswf_t* wf);

int get_nwk(pswf_t* wf);

int get_nspin(pswf_t* wf);

double legendre(int l, int m, double x);

double fac(int n);

double complex Ylm(int l, int m, double theta, double phi);

double complex Ylm2(int l, int m, double costheta, double phi);

double complex proj_value(funcset_t funcs, double* x, int m, double rmax,
	double* ion_pos, double* pos, double* lattice);

double** spline_coeff(double* x, double* y, int N);

void frac_from_index(int index, double* coord, int* fftg);

void ALLOCATION_FAILED();

#endif
