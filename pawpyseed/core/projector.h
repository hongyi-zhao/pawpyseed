/** \file
This file contains the functions needed to perform overlap operator evaulations
between bands of different wavefunctions. It uses a similar algorithm to
VASP for the onto_projector and projector_values evaluation.
*/

#ifndef PROJECTOR_H
#define PROJECTOR_H
#include "linalg.h"

/**
Returns a point to a list of ppot_t objects, one for each element in a POTCAR
file. Called as a helper function by Wavefunction.make_c_projectors
*/
ppot_t* get_projector_list(int num_els, int* labels, int* ls, double* proj_grids, double* wave_grids,
	double* projectors, double* aewaves, double* pswaves, double* rmaxs);

/**
Finds the coordinates on the FFT grid that fall within each projection sphere
and stores the values of the projectors at that those points, as well
as the coordinates of those points, in real_proj_site_t objects,
one for each site in the structure.
*/
real_proj_site_t* projector_values(int num_sites, int* labels, double* coords,
	double* lattice, double* reclattice, ppot_t* pps, int* fftg);

/**
Helper function for onto_projector, which performs the FFT of the wavefunction
into real space and calculates from <p_i|psit_nk> from the grid points found
in projector_values.
*/
void onto_projector_helper(band_t* band, fft_complex* x, real_proj_site_t* sites,
	int num_sites, int* labels, double* lattice, double* reclattice, double* kpt, ppot_t* pps, int* fftg);

/**
Calculates <p_i|psit_nk> for all i={R,epsilon,l,m} in the structure for one band
*/
void onto_projector(kpoint_t* kpt, int band_num, real_proj_site_t* sites, int num_sites, int* labels,
	int* G_bounds, double* lattice, double* reclattice, ppot_t* pps, int* fftg);

/**
Calculates the maximum number of grid points that can be contained within the projector
sphere of pp_ptr given the lattice and fftg (FFT grid dimensions, 3D vector of int),
and stores this value to pp_ptr.
*/
void add_num_cart_gridpts(ppot_t* pp_ptr, double* lattice, int* fftg);

/**
Calculates <phi_i|phi_j>, <phit_i|phit_j>, and <(phi_i-phit_i)|(phi_j-phit_j)>
for all onsite i and j for the element represented by pp_ptr.
*/
void make_pwave_overlap_matrices(ppot_t* pp_ptr);

/**
Evaluates <p_i|psit_nk> for all bands and kpoints of wf,
and calls generate_rayleigh_expansion_terms to project
all plane-waves onto the differences of partial
waves |phi_i-phit_i>.
*/
void setup_projections(pswf_t* wf, ppot_t* pps, int num_elems,
	int num_sites, int* fftg, int* labels, double* coords);

/**
Same as setup_projections, but copies the partial wave expansion terms
from a pswf_t objected pointed to by wf_R which has the same
basis set as wf
*/
void setup_projections_copy_rayleigh(pswf_t* wf, pswf_t* wf_R, ppot_t* pps, int num_elems,
		int num_sites, int* fftg, int* labels, double* coords);

/**
Calculates three overlap terms for when bands have different
structures:
<(phi1_i-phit1_i)|psit2_n2k>
<(phi2_i-phit2_i)|psit1_n1k>
<(phi1_i-phit1_i)|(phi2_i-phit2_i)>
*/
void overlap_setup(pswf_t* wf_R, pswf_t* wf_S, ppot_t* pps,
        int* labels_R, int* labels_S, double* coords_R, double* coords_S,
        int* N_R, int* N_S, int* N_RS_R, int* N_RS_S, int num_N_R, int num_N_S, int num_N_RS);

/**
Calculates the components of the overlap operator in the augmentation
regions of each ion in the lattice.
*/
double* compensation_terms(int BAND_NUM, pswf_t* wf_proj, pswf_t* wf_ref, ppot_t* pps,
	int num_elems, int num_M, int num_N_R, int num_N_S, int num_N_RS,
	int* M_R, int* M_S, int* N_R, int* N_S, int* N_RS_R, int* N_RS_S,
	int* proj_labels, double* proj_coords, int* ref_labels, double* ref_coords,
	int* fft_grid);

/**
###DEPRECATED###
O(N^2) spherical Bessel transform
*/
double* besselt(double* r, double* k, double* f, double encut, int N, int l);

#endif
