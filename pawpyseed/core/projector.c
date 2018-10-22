#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include "utils.h"
#include "projector.h"
#include <mkl.h>
#include <mkl_types.h>
#include "linalg.h"
#include "quadrature.h"
#include "radial.h"
#include "sbt.h"

#define c 0.262465831
#define PI 3.14159265358979323846
#define DENSE_GRID_SCALE 1

ppot_t* get_projector_list(int num_els, int* labels, int* ls, double* proj_grids, double* wave_grids,
	double* projectors, double* aewaves, double* pswaves, double* rmaxs, double grid_encut) {

	setbuf(stdout,NULL);	
	ppot_t* pps = (ppot_t*) malloc(num_els * sizeof(ppot_t));
	CHECK_ALLOCATION(pps);
	int wt = 0;
	int pt = 0;
	int wgt = 0;
	int pgt = 0;
	int l_num = 0;
	for (int i = 0; i < num_els; i++) {
		pps[i].num_projs = labels[4*i+1];
		pps[i].rmax = rmaxs[i];
		//sscanf(rmaxs[i], "%lf", &(pps[i].rmax));
		pps[i].proj_gridsize = labels[4*i+2];
		pps[i].wave_gridsize = labels[4*i+3];
		printf("vals %d %d %d\n", pps[i].num_projs, pps[i].proj_gridsize, pps[i].wave_gridsize);
		pps[i].total_projs = 0;
		pps[i].wave_grid = (double*) malloc((pps[i].wave_gridsize)*sizeof(double));
		pps[i].kwave_grid = (double*) malloc((pps[i].wave_gridsize)*sizeof(double));
		CHECK_ALLOCATION(pps[i].wave_grid);
		CHECK_ALLOCATION(pps[i].kwave_grid);
		pps[i].lmax = 0;
		pps[i].pspw_overlap_matrix = NULL;
		pps[i].aepw_overlap_matrix = NULL;
		pps[i].diff_overlap_matrix = NULL;
		for (int j = 0; j < pps[i].wave_gridsize; j++) {
			pps[i].wave_grid[j] = wave_grids[wgt];
			wgt++;
		}
		pps[i].proj_grid = (double*) malloc(pps[i].proj_gridsize*sizeof(double));
		CHECK_ALLOCATION(pps[i].proj_grid);
		for (int j = 0; j < pps[i].proj_gridsize; j++) {
			pps[i].proj_grid[j] = pps[i].rmax / pps[i].proj_gridsize * j;
			pgt++;
		}
		funcset_t* funcs = (funcset_t*) malloc(pps[i].num_projs*sizeof(funcset_t));
		CHECK_ALLOCATION(funcs);
		double* dense_wavegrid = (double*) malloc(DENSE_GRID_SCALE * pps[i].wave_gridsize * sizeof(double));
		CHECK_ALLOCATION(dense_wavegrid);
		dense_wavegrid[0] = pps[i].wave_grid[0];
		double factor = pow(pps[i].wave_grid[1]/pps[i].wave_grid[0], 1.0/DENSE_GRID_SCALE);
		for (int p = 1; p < pps[i].wave_gridsize * DENSE_GRID_SCALE; p++) {
			dense_wavegrid[p] = dense_wavegrid[p-1] * factor;
		}
		pps[i].wave_rmax = pps[i].wave_grid[pps[i].wave_gridsize-1];
		pps[i].smooth_grid = (double*) malloc(pps[i].proj_gridsize * sizeof(double));
		for (int j = 0; j < pps[i].proj_gridsize; j++) {
			pps[i].smooth_grid[j] = pps[i].wave_rmax / pps[i].proj_gridsize * j;
		}
		double* dense_kwavegrid = (double*) malloc(DENSE_GRID_SCALE * pps[i].wave_gridsize * sizeof(double));
		CHECK_ALLOCATION(dense_kwavegrid);
		for (int k = 0; k < pps[i].num_projs; k++) {
			funcs[k].proj = (double*) malloc(sizeof(double)*pps[i].proj_gridsize);
			funcs[k].aewave = (double*) malloc(sizeof(double)*pps[i].wave_gridsize);
			funcs[k].pswave = (double*) malloc(sizeof(double)*pps[i].wave_gridsize);
			funcs[k].diffwave = (double*) malloc(sizeof(double)*pps[i].wave_gridsize);
			CHECK_ALLOCATION(funcs[k].proj);
			CHECK_ALLOCATION(funcs[k].aewave);
			CHECK_ALLOCATION(funcs[k].pswave);
			CHECK_ALLOCATION(funcs[k].diffwave);
			funcs[k].l = ls[l_num];
			if (funcs[k].l > pps[i].lmax)
				pps[i].lmax = funcs[k].l;
			pps[i].total_projs += 2 * ls[l_num] + 1;
			l_num++;
			for (int j = 0; j < pps[i].wave_gridsize; j++) {
				funcs[k].aewave[j] = aewaves[wt];
				funcs[k].pswave[j] = pswaves[wt];
				funcs[k].diffwave[j] = aewaves[wt] - pswaves[wt];
				wt++;
			}
			for (int j = 0; j < pps[i].proj_gridsize; j++) {
				funcs[k].proj[j] = projectors[pt];
				pt++;
			}
			funcs[k].proj_spline = spline_coeff(pps[i].proj_grid, funcs[k].proj, pps[i].proj_gridsize);
			funcs[k].aewave_spline = spline_coeff(pps[i].wave_grid, funcs[k].aewave, pps[i].wave_gridsize);
			funcs[k].pswave_spline = spline_coeff(pps[i].wave_grid, funcs[k].pswave, pps[i].wave_gridsize);
			funcs[k].diffwave_spline = spline_coeff(pps[i].wave_grid, funcs[k].diffwave, pps[i].wave_gridsize);

			double* dense_diffwave = (double*) malloc(DENSE_GRID_SCALE * pps[i].wave_gridsize * sizeof(double));
			dense_diffwave[0] = funcs[k].diffwave[0];
			for (int p = 1; p < pps[i].wave_gridsize * DENSE_GRID_SCALE; p++) {
				dense_diffwave[p] = wave_interpolate(dense_wavegrid[p], pps[i].wave_gridsize,
					pps[i].wave_grid, funcs[k].diffwave, funcs[k].diffwave_spline);
			}
			funcs[k].smooth_diffwave = dense_diffwave;

		}

		sbt_descriptor_t* d = spherical_bessel_transform_setup(1e7, 0, pps[i].lmax,
			pps[i].wave_gridsize, pps[i].wave_grid, pps[i].kwave_grid);
		for (int k = 0; k < pps[i].num_projs; k++) {
			funcs[k].kwave = wave_spherical_bessel_transform(d, funcs[k].diffwave, funcs[k].l);
			//funcs[k].kwave = besselt(pps[i].wave_grid, pps[i].kwave_grid, funcs[k].diffwave, 520.0, pps[i].wave_gridsize, funcs[k].l);
			funcs[k].kwave_spline = spline_coeff(pps[i].kwave_grid, funcs[k].kwave, pps[i].wave_gridsize);
		}
		//free_sbt_descriptor(d);
		/*
		pps[i].dense_kgrid = (double*) malloc (pps[i].wave_gridsize * DENSE_GRID_SCALE * sizeof(double*));
		d = spherical_bessel_transform_setup(grid_encut, 1e7, pps[i].lmax,
            pps[i].wave_gridsize * DENSE_GRID_SCALE, dense_wavegrid, pps[i].dense_kgrid);
        for (int k = 0; k < pps[i].num_projs; k++) {
            funcs[k].dense_kwave = wave_spherical_bessel_transform(d, funcs[k].smooth_diffwave, funcs[k].l);
            funcs[k].dense_kwave_spline = spline_coeff(pps[i].dense_kgrid, funcs[k].dense_kwave,
                pps[i].wave_gridsize * DENSE_GRID_SCALE);
        }
		free_sbt_descriptor(d);
		*/
		for (int k = 0; k < pps[i].num_projs; k++) {
			//double* dense_kwave = wave_spherical_bessel_transform(d2, funcs[k].smooth_diffwave, funcs[k].l);
			double* dense_kwave = (double*) calloc(pps[i].wave_gridsize * DENSE_GRID_SCALE, sizeof(double));
			int q = 0;
			while (pps[i].kwave_grid[q] < pow(c*grid_encut, 0.5)) {
				//printf("KKKK %lf\n", pps[i].kwave_grid[q]);
				dense_kwave[q] = funcs[k].kwave[q];
				q++;
			}
			double* smooth_diffwave = inverse_wave_spherical_bessel_transform(d, dense_kwave, funcs[k].l);
			double** smooth_wave_spline = spline_coeff(dense_wavegrid,
				smooth_diffwave, DENSE_GRID_SCALE*pps[i].wave_gridsize);
			free(dense_kwave);
			double* sdw = (double*) malloc(pps[i].proj_gridsize*sizeof(double));
			if (funcs[k].l > 0) sdw[0] = 0;
			for (int p = 1; p < pps[i].proj_gridsize; p++) {
				// smooth_grid should be like proj_grid except that rmax should be rmax of the partial wave
				// difference
				sdw[p] = wave_interpolate(pps[i].smooth_grid[p], pps[i].wave_gridsize*DENSE_GRID_SCALE,
					dense_wavegrid, smooth_diffwave, smooth_wave_spline);
			}
			if (funcs[k].l == 0) sdw[0] = sdw[1];
			double** sdw_spline = spline_coeff(pps[i].smooth_grid, sdw, pps[i].proj_gridsize);
			free(funcs[k].smooth_diffwave);
			funcs[k].smooth_diffwave = sdw;
			funcs[k].smooth_diffwave_spline = sdw_spline;
			free(smooth_diffwave);
			free(smooth_wave_spline[0]);
			free(smooth_wave_spline[1]);
			free(smooth_wave_spline[2]);
			free(smooth_wave_spline);
			//for (int p = 0; p < pps[i].proj_gridsize; p++)
			//	printf("PARTIALWAVEDIFF %lf %lf\n", pps[i].smooth_grid[p], funcs[k].smooth_diffwave[p]);
		}
		free_sbt_descriptor(d);
		pps[i].funcs = funcs;
		make_pwave_overlap_matrices(pps+i);
	}
	mkl_free_buffers();
	printf("finished making projector list\n");
	return pps;
}

double* besselt(double* r, double* k, double* f, double encut, int N, int l) {
	
	double kmax = pow(encut*c, 0.5);
	double drho = log(r[1]/r[0]);
	double kmin = kmax * exp((1-N)*drho);
	double* g = (double*) malloc(N*sizeof(double));
	CHECK_ALLOCATION(g);
	for (int i = 0; i < N; i++) {
		double dr = r[0];
		g[i] = 0;
		k[i] = kmin * exp(i*drho);
		for (int j = 0; j < N; j++) {
			g[i] += f[j] * r[j] * sph_bessel(k[i], r[j], l) * dr; 
			if (j != N-1) dr = r[j+1]-r[j];
		}
	}
	return g;

}

real_proj_site_t* projector_values(int num_sites, int* labels, double* coords,
	double* lattice, double* reclattice, ppot_t* pps, int* fftg) {

	double intervals[3] = {mag(lattice)/fftg[0], mag(lattice+3)/fftg[1], mag(lattice+6)/fftg[2]};
	double vol = determinant(lattice);
	int num_pts = fftg[0] * fftg[1] * fftg[2];

	real_proj_site_t* sites = (real_proj_site_t*) malloc(num_sites * sizeof(real_proj_site_t));
	CHECK_ALLOCATION(sites);
	int* all_sites = (int*) malloc(num_sites * sizeof(int));
	for (int i = 0; i < num_sites; i++) {
		all_sites[i] = i;
	}
	setup_site(sites, pps, num_sites, all_sites, labels, coords, lattice, fftg, 0);

	free(all_sites);
	return sites;
}

real_proj_site_t* smooth_pw_values(int num_N, int* Nlst, int* labels, double* coords,
	double* lattice, double* reclattice, ppot_t* pps, int* fftg) {

	double intervals[3] = {mag(lattice)/fftg[0], mag(lattice+3)/fftg[1], mag(lattice+6)/fftg[2]};
	double vol = determinant(lattice);
	int num_pts = fftg[0] * fftg[1] * fftg[2];

	real_proj_site_t* sites = (real_proj_site_t*) malloc(num_N * sizeof(real_proj_site_t));
	CHECK_ALLOCATION(sites);
	setup_site(sites, pps, num_N, Nlst, labels, coords, lattice, fftg, 1);

	return sites;
}

void onto_projector_helper(band_t* band, double complex* x, real_proj_site_t* sites,
	int num_sites, double* lattice, double* reclattice, double* kpt, ppot_t* pps, int* fftg,
	projection_t* projections) {

	double dv = determinant(lattice) / fftg[0] / fftg[1] / fftg[2];

	double path[3] = {0,0,0};
	double kdotr = 0;

	double kpt_cart[3] = {0,0,0};
	kpt_cart[0] = kpt[0];
	kpt_cart[1] = kpt[1];
	kpt_cart[2] = kpt[2];
	frac_to_cartesian(kpt_cart, reclattice);

	int num_indices, index, t=0;
	for (int s = 0; s < num_sites; s++) {
		//printf("checking coord %lf %lf %lf\n", sites[s].coord[0], sites[s].coord[1], sites[s].coord[2]);
		num_indices = sites[s].num_indices;
		int* indices = sites[s].indices;
		projections[s].num_projs = sites[s].num_projs;
		projections[s].total_projs = sites[s].total_projs;
		projections[s].ns = malloc(sites[s].total_projs * sizeof(int));
		projections[s].ls = malloc(sites[s].total_projs * sizeof(int));
		projections[s].ms = malloc(sites[s].total_projs * sizeof(int));
		projections[s].overlaps = (double complex*) malloc(sites[s].total_projs * sizeof(double complex));
		CHECK_ALLOCATION(projections[s].ns);
		CHECK_ALLOCATION(projections[s].ls);
		CHECK_ALLOCATION(projections[s].ms);
		CHECK_ALLOCATION(projections[s].overlaps);
		for (int p = 0; p < sites[s].total_projs; p++) {
			projections[s].ns[p] = sites[s].projs[p].func_num;
			projections[s].ls[p] = sites[s].projs[p].l;
			projections[s].ms[p] = sites[s].projs[p].m;
			double complex* values = sites[s].projs[p].values;
			double complex total = 0 + 0 * I;
			for (int i = 0; i < num_indices; i++) {
				index = indices[i];
				kdotr = dot(kpt_cart, sites[s].projs[p].paths+i*3);
				total += x[index] * conj(values[i]) * dv * cexp(I * kdotr);
			}
			projections[s].overlaps[p] = total;
		}
	}
}

void onto_projector(kpoint_t* kpt, int band_num, real_proj_site_t* sites, int num_sites,
	int* G_bounds, double* lattice, double* reclattice, ppot_t* pps, int* fftg) {

	double* k = kpt->k;
	int* Gs = kpt->Gs;
	float complex* Cs = kpt->bands[band_num]->Cs;
	int num_waves = kpt->num_waves;
	
	double complex* x = (double complex*) mkl_calloc(fftg[0]*fftg[1]*fftg[2],
		sizeof(double complex), 64);
	CHECK_ALLOCATION(x);
	fft3d(x, G_bounds, lattice, k, Gs, Cs, num_waves, fftg);

	band_t* band = kpt->bands[band_num];
	band->projections = (projection_t*) malloc(num_sites * sizeof(projection_t));
	CHECK_ALLOCATION (band->projections);

	onto_projector_helper(kpt->bands[band_num], x, sites, num_sites,
		lattice, reclattice, k, pps, fftg, band->projections);

	//kpt->bands[band_num]->CRs = x;
	mkl_free(x);
}

void onto_projector_ncl(kpoint_t* kpt, int band_num, real_proj_site_t* sites, int num_sites,
	int* G_bounds, double* lattice, double* reclattice, ppot_t* pps, int* fftg) {

	double* k = kpt->k;
	int* Gs = kpt->Gs;
	float complex* Cs = kpt->bands[band_num]->Cs;
	int num_waves = kpt->num_waves;

	double complex* xup = (double complex*) mkl_calloc(fftg[0]*fftg[1]*fftg[2],
		sizeof(double complex), 64);
	double complex* xdown = (double complex*) mkl_calloc(fftg[0]*fftg[1]*fftg[2],
		sizeof(double complex), 64);
	CHECK_ALLOCATION(xup);
	CHECK_ALLOCATION(xdown);
	fft3d(xup, G_bounds, lattice, k, Gs, Cs, num_waves/2, fftg);
	fft3d(xdown, G_bounds, lattice, k, Gs, Cs+num_waves/2, num_waves/2, fftg);

	band_t* band = kpt->bands[band_num];
	band->up_projections =
		(projection_t*) malloc(num_sites * sizeof(projection_t));
	band->down_projections =
		(projection_t*) malloc(num_sites * sizeof(projection_t));
	onto_projector_helper(kpt->bands[band_num], xup, sites, num_sites,
		lattice, reclattice, k, pps, fftg, band->up_projections);
	onto_projector_helper(kpt->bands[band_num], xdown, sites, num_sites,
		lattice, reclattice, k, pps, fftg, band->down_projections);
}

void onto_smoothpw(kpoint_t* kpt, int band_num, real_proj_site_t* sites, int num_sites,
	int* G_bounds, double* lattice, double* reclattice, ppot_t* pps, int* fftg) {

	double* k = kpt->k;
	int* Gs = kpt->Gs;
	float complex* Cs = kpt->bands[band_num]->Cs;
	int num_waves = kpt->num_waves;

	double complex* x = (double complex*) mkl_calloc(fftg[0]*fftg[1]*fftg[2], sizeof(double complex), 64);
	CHECK_ALLOCATION(x);
	fft3d(x, G_bounds, lattice, k, Gs, Cs, num_waves, fftg);

	band_t* band = kpt->bands[band_num];
	band->wave_projections = (projection_t*) malloc(num_sites * sizeof(projection_t));
	CHECK_ALLOCATION (band->wave_projections);

	onto_projector_helper(kpt->bands[band_num], x, sites, num_sites,
		lattice, reclattice, k, pps, fftg, band->wave_projections);

	mkl_free(x);
}

void add_num_cart_gridpts(ppot_t* pp_ptr, double* lattice, int* fftg) {

	ppot_t pp = *pp_ptr;

	double maga1 = mag(lattice+0);
	double maga2 = mag(lattice+3);
	double maga3 = mag(lattice+6);

	double vtemp[3];
	double vmag, sinphi123;
	
	double phi12 = acos(dot(lattice+0, lattice+3) / (maga1 * maga2));
	vcross(vtemp, lattice+0, lattice+3);
	vmag = mag(vtemp);
	sinphi123 = dot(lattice+6, vtemp) / (vmag * maga3);
	double na1maxA = pp.rmax * fftg[0] / (maga1 * fabs(sin(phi12))) + 1;
	double na2maxA = pp.rmax * fftg[1] / (maga2 * fabs(sin(phi12))) + 1;
	double na3maxA = pp.rmax * fftg[2] / (maga3 * fabs(sinphi123)) + 1;
	int npmaxA = (int)(4.0/3.0*PI*na1maxA*na2maxA*na3maxA) + 1;

	double phi13 = acos(dot(lattice+0, lattice+6) / (maga1 * maga3));
	vcross(vtemp, lattice+0, lattice+6);
	vmag = mag(vtemp);
	sinphi123 = dot(lattice+3, vtemp) / (vmag * maga2);
	double na1maxB = pp.rmax * fftg[0] / (maga1 * fabs(sin(phi13))) + 1;
	double na2maxB = pp.rmax * fftg[1] / (maga2 * fabs(sinphi123)) + 1;
	double na3maxB = pp.rmax * fftg[2] / (maga3 * fabs(sin(phi13))) + 1;
	int npmaxB = (int)(4.0/3.0*PI*na1maxB*na2maxB*na3maxB) + 1;

	double phi23 = acos(dot(lattice+3, lattice+6) / (maga2 * maga3));
	vcross(vtemp, lattice+3, lattice+6);
	vmag = mag(vtemp);
	sinphi123 = dot(lattice, vtemp) / (vmag * maga1);
	double na1maxC = pp.rmax * fftg[0] / (maga1 * fabs(sinphi123)) + 1;
	double na2maxC = pp.rmax * fftg[1] / (maga2 * fabs(sin(phi23))) + 1;
	double na3maxC = pp.rmax * fftg[2] / (maga3 * fabs(sin(phi23))) + 1;
	int npmaxC = (int)(4.0/3.0*PI*na1maxC*na2maxC*na3maxC) + 1;

	printf("ancg %lf %lf %lf %lf %lf %d %d %d\n", maga1, maga2, maga3, na2maxA, na3maxA, npmaxA, npmaxB, npmaxC);

	int npmax = npmaxA;
	if (npmaxB > npmax) npmax = npmaxB;
	if (npmaxC > npmax) npmax = npmaxC;

	pp_ptr->num_cart_gridpts = npmax;
}

void make_pwave_overlap_matrices(ppot_t* pp_ptr) {
	ppot_t pp = *pp_ptr;
	int size = pp.num_projs * pp.num_projs;
	double* psov = (double*) calloc(size, sizeof(double));
	double* aeov = (double*) calloc(size, sizeof(double));
	double* diov = (double*) calloc(size, sizeof(double));
	CHECK_ALLOCATION(psov);
	CHECK_ALLOCATION(aeov);
	CHECK_ALLOCATION(diov);

	for (int i = 0; i < pp.num_projs; i++) {
		for (int j = i; j < pp.num_projs; j++) {
			if (pp.funcs[i].l == pp.funcs[j].l) {
				double* ps1 = pp.funcs[i].pswave;
				double* ps2 = pp.funcs[j].pswave;
				double* ae1 = pp.funcs[i].aewave;
				double* ae2 = pp.funcs[j].aewave;
				double* psprod = (double*) malloc(pp.wave_gridsize*sizeof(double));
				double* aeprod = (double*) malloc(pp.wave_gridsize*sizeof(double));
				double* diprod = (double*) malloc(pp.wave_gridsize*sizeof(double));
				for (int k = 0; k < pp.wave_gridsize; k++) {
					psprod[k] = ps1[k] * ps2[k];
					aeprod[k] = ae1[k] * ae2[k];
					diprod[k] = (ae1[k]-ps1[k]) * (ae2[k]-ps2[k]);
				}
				double** psspline = spline_coeff(pp.wave_grid, psprod, pp.wave_gridsize);
				double** aespline = spline_coeff(pp.wave_grid, aeprod, pp.wave_gridsize);
				double** displine = spline_coeff(pp.wave_grid, diprod, pp.wave_gridsize);
				psov[i*pp.num_projs+j] = spline_integral(pp.wave_grid, psprod, psspline, pp.wave_gridsize);
				aeov[i*pp.num_projs+j] = spline_integral(pp.wave_grid, aeprod, aespline, pp.wave_gridsize);
				diov[i*pp.num_projs+j] = spline_integral(pp.wave_grid, diprod, displine, pp.wave_gridsize);
			}
		}
	}
	for (int i = 1; i < pp.num_projs; i++) {
		for (int j = 0; j < i; j++) {
			psov[pp.num_projs*i+j] = psov[pp.num_projs*j+i];
			aeov[pp.num_projs*i+j] = aeov[pp.num_projs*j+i];
			diov[pp.num_projs*i+j] = diov[pp.num_projs*j+i];
		}
	}

	pp_ptr->pspw_overlap_matrix = psov;
	pp_ptr->aepw_overlap_matrix = aeov;
	pp_ptr->diff_overlap_matrix = diov;
}

void setup_projections_no_rayleigh(pswf_t* wf, ppot_t* pps, int num_elems,
	int num_sites, int* fftg, int* labels, double* coords) {

	wf->num_sites = num_sites;
	wf->fftg = (int*) malloc(3*sizeof(int));
	wf->fftg[0] = fftg[0];
	wf->fftg[1] = fftg[1];
	wf->fftg[2] = fftg[2];
	printf("ptrvals %p %p\n", wf, pps);
	wf->num_elems = num_elems;
	wf->num_sites = num_sites;
	wf->pps = pps;
	printf("started setup_proj\n");
	#pragma omp parallel for 
	for (int p = 0; p < num_elems; p++) {
		add_num_cart_gridpts(pps+p, wf->lattice, fftg);
	}
	int NUM_KPTS = wf->nwk * wf->nspin;
	int NUM_BANDS = wf->nband;
	printf("calculating projector_values\n");
	real_proj_site_t* sites = projector_values(num_sites, labels, coords,
		wf->lattice, wf->reclattice, pps, fftg);
	printf("onto_projector calcs\n");
	#pragma omp parallel for 
	for (int w = 0; w < NUM_BANDS * NUM_KPTS; w++) {
		kpoint_t* kpt = wf->kpts[w % NUM_KPTS];
		int band_num = w / NUM_KPTS;
		onto_projector(kpt, band_num, sites, num_sites,
			wf->G_bounds, wf->lattice, wf->reclattice, pps, fftg);
		if (wf->is_ncl) {
			onto_projector_ncl(kpt, band_num, sites, num_sites,
				wf->G_bounds, wf->lattice, wf->reclattice, pps, fftg);
		}
	}
	printf("Done \n");
	free_real_proj_site_list(sites, num_sites);	
}

void setup_projections(pswf_t* wf, ppot_t* pps, int num_elems,
		int num_sites, int* fftg, int* labels, double* coords) {

	setup_projections_no_rayleigh(wf, pps, num_elems,
		num_sites, fftg, labels, coords);
	generate_rayleigh_expansion_terms(wf, pps, num_elems);
}

void setup_projections_copy_rayleigh(pswf_t* wf, pswf_t* wf_R, ppot_t* pps, int num_elems,
		int num_sites, int* fftg, int* labels, double* coords) {

	setup_projections_no_rayleigh(wf, pps, num_elems,
		num_sites, fftg, labels, coords);
	copy_rayleigh_expansion_terms(wf, pps, num_elems, wf_R);
}

void overlap_setup(pswf_t* wf_R, pswf_t* wf_S, ppot_t* pps,
	int* labels_R, int* labels_S, double* coords_R, double* coords_S,
	int* N_R, int* N_S, int* N_RS_R, int* N_RS_S, int num_N_R, int num_N_S, int num_N_RS) {

	double complex** overlaps = NULL;
	if (num_N_RS > 0) {
		overlaps = (double complex**) malloc(num_N_RS * sizeof(double complex*));
		CHECK_ALLOCATION(overlaps);
	}

	printf("STARTING OVERLAP_SETUP\n");
	int NUM_KPTS = wf_R->nwk * wf_R->nspin;
	int NUM_BANDS = wf_S->nband;
	double inv_sqrt_vol = pow(determinant(wf_R->lattice), -0.5);
	#pragma omp parallel for
	for (int w = 0; w < NUM_BANDS * NUM_KPTS; w++) {
		kpoint_t* kpt_R = wf_R->kpts[w%NUM_KPTS];
		kpoint_t* kpt_S = wf_S->kpts[w%NUM_KPTS];
		//band_t* band_R = kpt_R->bands[w/NUM_KPTS];
		band_t* band_S = kpt_S->bands[w/NUM_KPTS];

		projection_t* wps = NULL;
		if (num_N_R > 0) {
			wps = (projection_t*) malloc(num_N_R * sizeof(projection_t));
			CHECK_ALLOCATION(wps);
		}
		for (int n = 0; n < num_N_R; n++) {
			int s = N_R[n];
			ppot_t pp = pps[labels_R[s]];
			wps[n].overlaps = (double complex*) malloc(pp.total_projs * sizeof(double complex));
			CHECK_ALLOCATION(wps[n].overlaps);
			int t = 0;
			for (int i = 0; i < pp.num_projs; i++) {
				for (int m = -pp.funcs[i].l; m <= pp.funcs[i].l; m++) {
					wps[n].overlaps[t] = rayexp(kpt_S->k, kpt_S->Gs,
						band_S->Cs, pp.funcs[i].l, m,
						kpt_S->num_waves,
						kpt_R->expansion[labels_R[s]][i].terms,
						coords_R + s*3) * inv_sqrt_vol;
					t++;
				}
			}
		}
		band_S->wave_projections = wps;
	}
	printf("ONE THIRD DONE\n");
	NUM_BANDS = wf_R->nband;
	#pragma omp parallel for
	for (int w = 0; w < NUM_BANDS * NUM_KPTS; w++) {
		kpoint_t* kpt_R = wf_R->kpts[w%NUM_KPTS];
		kpoint_t* kpt_S = wf_S->kpts[w%NUM_KPTS];
		band_t* band_R = kpt_R->bands[w/NUM_KPTS];
		//band_t* band_S = kpt_S->bands[w/NUM_KPTS];

		projection_t* wps = NULL;
		if (num_N_S > 0) {
			wps = (projection_t*) malloc(num_N_S * sizeof(projection_t));
			CHECK_ALLOCATION(wps);
		}
		for (int n = 0; n < num_N_S; n++) {
			int s = N_S[n];
			ppot_t pp = pps[labels_S[s]];
			wps[n].overlaps = (double complex*) malloc(pp.total_projs * sizeof(double complex));
			int t = 0;
			for (int i = 0; i < pp.num_projs; i++) {
				for (int m = -pp.funcs[i].l; m <= pp.funcs[i].l; m++) {
					wps[n].overlaps[t] = rayexp(kpt_R->k, kpt_R->Gs,
						band_R->Cs, pp.funcs[i].l, m,
						kpt_R->num_waves,
						kpt_S->expansion[labels_S[s]][i].terms,
						coords_S + s*3) * inv_sqrt_vol;
					t++;
				}
			}
		}
		band_R->wave_projections = wps;
	}
	printf("TWO THIRDS DONE\n");
	
	double* dcoords =  NULL;
	if (num_N_RS > 0) {
		dcoords = (double*) malloc(3 * num_N_RS * sizeof(double));
		CHECK_ALLOCATION(dcoords);
	}
	#pragma omp parallel for
	for (int i = 0; i < num_N_RS; i++) {
		double R = 0;
		int l1, l2;
		int s1 = N_RS_R[i];
		int s2 = N_RS_S[i];
		printf("%d %d\n", N_RS_R[i], N_RS_S[i]);
		ppot_t pp1 = pps[labels_R[s1]];
		ppot_t pp2 = pps[labels_S[s2]];
		// CALCULATE THE DIFF COORD HERE, PASS TO offsite_wave_overlap AND SAVE IT FOR USE IN compensation_terms
		overlaps[i] = calloc(pp1.total_projs * pp2.total_projs, sizeof(double complex));
		CHECK_ALLOCATION(overlaps[i]);
		double* coord1 = coords_R + 3 * s1;
		double* coord2 = coords_S + 3 * s2;
		min_cart_path(coord2, coord1, wf_R->lattice, dcoords + 3*i, &R);
		printf("herewego %lf %lf %lf\nherewego%lf %lf %lf\n", coord1[0], coord1[1], coord1[2], coord2[0], coord2[1], coord2[2]);
		int tj = 0;
		for (int j = 0; j < pp1.num_projs; j++) {
			l1 = pp1.funcs[j].l;
			for (int m1 = -l1; m1 <= l1; m1++) {
				int tk = 0;
				for (int k = 0; k < pp2.num_projs; k++) {
					l2 = pp2.funcs[k].l;
					for (int m2 = -l2; m2 <= l2; m2++) {
						if (1){//(R > 0.001) {
							overlaps[i][tj*pp2.total_projs+tk] =
								offsite_wave_overlap(dcoords + 3*i, pp1.wave_grid,
								pp1.funcs[j].diffwave,
								pp1.funcs[j].diffwave_spline, pp1.wave_gridsize,
								pp2.wave_grid, pp2.funcs[k].diffwave,
								pp2.funcs[k].diffwave_spline, pp2.wave_gridsize,
								wf_R->lattice, l1, m1, l2, m2);
							//overlaps[i][tj*pp2.total_projs+tk] =
                            //    reciprocal_offsite_wave_overlap(dcoords + 3*i,
                            //    pp1.kwave_grid, pp1.funcs[j].kwave,
                            //    pp1.funcs[j].kwave_spline, pp1.wave_gridsize,
                            //    pp2.kwave_grid, pp2.funcs[k].kwave,
                            //   pp2.funcs[k].kwave_spline, pp2.wave_gridsize,
                            //    wf_R->lattice, l1, m1, l2, m2);
						} else if (l1 == l2 && m1 == m2) {
							overlaps[i][tj*pp2.total_projs+tk] = pp2.diff_overlap_matrix[j*pp2.num_projs+k];
						}
						//printf("%d %d %d %d\n", N_RS_R[i], N_RS_S[i], tj, tk);
						tk++;
					}
				}
				tj++;
			}
		}
	}
	printf("%d %d %d %d stff\n", num_N_R, num_N_S, num_N_RS, 0);
	//printf("%p %p %p %p\n", overlaps[0], overlaps[1], overlaps[2], overlaps[3]);
	wf_S->overlaps = overlaps;
	wf_S->dcoords = dcoords;
	wf_S->num_aug_overlap_sites = num_N_RS;
	wf_R->num_aug_overlap_sites = num_N_RS;
	printf("finished overlap setup\n");
}

void overlap_setup_real(pswf_t* wf_R, pswf_t* wf_S, ppot_t* pps,
	int* labels_R, int* labels_S, double* coords_R, double* coords_S,
	int* N_R, int* N_S, int* N_RS_R, int* N_RS_S, int num_N_R, int num_N_S, int num_N_RS) {

	clean_wave_projections(wf_R);
	clean_wave_projections(wf_S);

	wf_R->wp_num = num_N_S;
	wf_S->wp_num = num_N_R;

	double complex** overlaps = NULL;
	if (num_N_RS > 0) {
		overlaps = (double complex**) malloc(num_N_RS * sizeof(double complex*));
		CHECK_ALLOCATION(overlaps);
	}

	printf("STARTING OVERLAP_SETUP\n");
	int NUM_KPTS = wf_R->nwk * wf_R->nspin;
	int NUM_BANDS = wf_S->nband;
	double inv_sqrt_vol = pow(determinant(wf_R->lattice), -0.5);
	if (num_N_R > 0) {
		real_proj_site_t* sites_N_R = smooth_pw_values(num_N_R, N_R, labels_R, coords_R,
			wf_S->lattice, wf_S->reclattice, pps, wf_S->fftg);
		#pragma omp parallel for
		for (int w = 0; w < NUM_BANDS * NUM_KPTS; w++) {
			kpoint_t* kpt_S = wf_S->kpts[w%NUM_KPTS];
	
			onto_smoothpw(kpt_S, w/NUM_KPTS, sites_N_R, num_N_R,
				wf_S->G_bounds, wf_S->lattice, wf_S->reclattice, pps, wf_S->fftg);
		}
		free_real_proj_site_list(sites_N_R, num_N_R);
	}
	printf("PART 1 DONE\n");
	if (num_N_S > 0) {
		real_proj_site_t* sites_N_S = smooth_pw_values(num_N_S, N_S, labels_S, coords_S,
			wf_R->lattice, wf_R->reclattice, pps, wf_R->fftg);
		NUM_BANDS = wf_R->nband;
		#pragma omp parallel for
		for (int w = 0; w < NUM_BANDS * NUM_KPTS; w++) {
			kpoint_t* kpt_R = wf_R->kpts[w%NUM_KPTS];

			onto_smoothpw(kpt_R, w/NUM_KPTS, sites_N_S, num_N_S,
				wf_R->G_bounds, wf_R->lattice, wf_R->reclattice, pps, wf_R->fftg);
		}
		free_real_proj_site_list(sites_N_S, num_N_S);
	}
	printf("PART 2 DONE\n");
	
	double* dcoords =  NULL;
	if (num_N_RS > 0) {
		dcoords = (double*) malloc(3 * num_N_RS * sizeof(double));
		CHECK_ALLOCATION(dcoords);
	}
	#pragma omp parallel for
	for (int i = 0; i < num_N_RS; i++) {
		double R = 0;
		int l1, l2;
		int s1 = N_RS_R[i];
		int s2 = N_RS_S[i];
		printf("%d %d\n", N_RS_R[i], N_RS_S[i]);
		ppot_t pp1 = pps[labels_R[s1]];
		ppot_t pp2 = pps[labels_S[s2]];
		// CALCULATE THE DIFF COORD HERE, PASS TO offsite_wave_overlap AND SAVE IT FOR USE IN compensation_terms
		overlaps[i] = (double complex*) calloc(pp1.total_projs * pp2.total_projs, sizeof(double complex));
		CHECK_ALLOCATION(overlaps[i]);
		double* coord1 = coords_R + 3 * s1;
		double* coord2 = coords_S + 3 * s2;
		min_cart_path(coord2, coord1, wf_R->lattice, dcoords + 3*i, &R);
		printf("herewego %lf %lf %lf\nherewego%lf %lf %lf\n", coord1[0], coord1[1], coord1[2], coord2[0], coord2[1], coord2[2]);
		int tj = 0;
		for (int j = 0; j < pp1.num_projs; j++) {
			l1 = pp1.funcs[j].l;
			for (int m1 = -l1; m1 <= l1; m1++) {
				int tk = 0;
				for (int k = 0; k < pp2.num_projs; k++) {
					l2 = pp2.funcs[k].l;
					for (int m2 = -l2; m2 <= l2; m2++) {
						if (1){//(R > 0.001) {
							overlaps[i][tj*pp2.total_projs+tk] =
                                reciprocal_offsite_wave_overlap(dcoords + 3*i,
                                pp1.kwave_grid, pp1.funcs[j].kwave,
                                pp1.funcs[j].kwave_spline, pp1.wave_gridsize,
                                pp2.kwave_grid, pp2.funcs[k].kwave,
                                pp2.funcs[k].kwave_spline, pp2.wave_gridsize,
                                wf_R->lattice, l1, m1, l2, m2);
							/*overlaps[i][tj*pp2.total_projs+tk] =
                            	offsite_wave_overlap(dcoords + 3*i, pp1.wave_grid,
                            	pp1.funcs[j].diffwave,
                            	pp1.funcs[j].diffwave_spline, pp1.wave_gridsize,
                            	pp2.wave_grid, pp2.funcs[k].diffwave,
                            	pp2.funcs[k].diffwave_spline, pp2.wave_gridsize,
                            	wf_R->lattice, l1, m1, l2, m2);*/
							//if (l1==l2 && m1==m2) printf("HIHIHI %d %d %d %d %lf %lf\n", l1,l2,m1,m2, creal(overlaps[i][tj*pp2.total_projs+tk]), cimag(overlaps[i][tj*pp2.total_projs+tk]));
						} else if (l1 == l2 && m1 == m2) {
							overlaps[i][tj*pp2.total_projs+tk] = pp2.diff_overlap_matrix[j*pp2.num_projs+k];
							printf("HIHIHI %d %d %d %d %lf %lf\n", l1,l2,m1,m2, creal(overlaps[i][tj*pp2.total_projs+tk]), cimag(overlaps[i][tj*pp2.total_projs+tk]));
						}
						//printf("%d %d %d %d\n", N_RS_R[i], N_RS_S[i], tj, tk);
						tk++;
					}
				}
				tj++;
			}
		}
	}
	printf("%d %d %d %d stff\n", num_N_R, num_N_S, num_N_RS, 0);
	//printf("%p %p %p %p\n", overlaps[0], overlaps[1], overlaps[2], overlaps[3]);
	wf_S->overlaps = overlaps;
	wf_S->dcoords = dcoords;
	wf_S->num_aug_overlap_sites = num_N_RS;
	wf_R->num_aug_overlap_sites = num_N_RS;
	printf("PART 3 DONE\nFINISHED OVERLAP SETUP\n");
}

double* compensation_terms(int BAND_NUM, pswf_t* wf_S, pswf_t* wf_R, ppot_t* pps,
	int num_elems, int num_M, int num_N_R, int num_N_S, int num_N_RS,
	int* M_R, int* M_S, int* N_R, int* N_S, int* N_RS_R, int* N_RS_S,
	int* proj_labels, double* proj_coords, int* ref_labels, double* ref_coords,
	int* fft_grid) {

	printf("fftg %d\n", fft_grid[0]);
	printf("fftg %d\n", fft_grid[1]);
	printf("fftg %d\n", fft_grid[2]);
	//freopen("tst.out", "w", stdout);
	setbuf(stdout, NULL);

	printf("%d %d %d %d %d %d\n", BAND_NUM, num_elems, num_M, num_N_R, num_N_S, num_N_RS);
	printf("%d %lf %d %lf %d\n", proj_labels[0], proj_coords[0], ref_labels[0], ref_coords[0], fft_grid[1]);
	
	int NUM_KPTS = wf_R->nwk * wf_R->nspin;
	int NUM_BANDS = wf_R->nband;

	double* overlap = (double*) calloc(2 * NUM_KPTS * NUM_BANDS, sizeof(double));
	CHECK_ALLOCATION(overlap);

	double complex** N_RS_overlaps = wf_S->overlaps;
	double inv_sqrt_vol = pow(determinant(wf_R->lattice), -0.5);

	#pragma omp parallel for
	for (int w = 0; w < NUM_BANDS * NUM_KPTS; w++) {
		int ni = 0, nj = 0;
		int l1 = 0, l2 = 0;

		kpoint_t* kpt_R = wf_R->kpts[w%NUM_KPTS];
		kpoint_t* kpt_S = wf_S->kpts[w%NUM_KPTS];
		band_t* band_R = kpt_R->bands[w/NUM_KPTS];
		band_t* band_S = kpt_S->bands[BAND_NUM];

		double complex temp = 0 + 0 * I;
		int t = 0;
		for (int s = 0; s < num_M; s++) {
			ppot_t pp = pps[ref_labels[M_R[s]]];
			int s1 = M_R[s];
			int s2 = M_S[s];
			projection_t pron = band_R->projections[s1];
			projection_t ppron = band_S->projections[s2];

			for (int i = 0; i < pron.total_projs; i++) {
				for (int j = 0; j < ppron.total_projs; j++) {
					if (pron.ls[i] == ppron.ls[j]  && pron.ms[i] == ppron.ms[j]) {
						nj = ppron.ns[j];
						ni = pron.ns[i];
						temp += conj(pron.overlaps[i])
							* (pp.aepw_overlap_matrix[pp.num_projs*ni+nj]
							- pp.pspw_overlap_matrix[pp.num_projs*ni+nj])
							* ppron.overlaps[j];
					}
				}
			}
		}
		overlap[2*w] = creal(temp);
		overlap[2*w+1]= cimag(temp);
		//printf("temp 1 %lf %lf\n", creal(temp), cimag(temp));

		temp = 0 + 0 * I;
		for (int s = 0; s < num_N_R; s++) {
			int site_num = N_R[s];
			projection_t pron = band_R->projections[site_num];
			projection_t ppron = band_S->wave_projections[s];
			ppot_t pp = pps[ref_labels[N_R[s]]];
			for (int i = 0; i < pp.total_projs; i++) {
				temp += ppron.overlaps[i] * conj(pron.overlaps[i]);
			}
		}
		overlap[2*w] += creal(temp);
		overlap[2*w+1]+= cimag(temp);
		//printf("temp 2 %lf %lf\n", creal(temp), cimag(temp));

		temp = 0 + 0 * I;
		for (int s = 0; s < num_N_S; s++) {
			ppot_t pp = pps[ref_labels[N_S[s]]];
			int site_num = N_S[s];
			projection_t pron = band_R->wave_projections[s];
			projection_t ppron = band_S->projections[site_num];
			for (int i = 0; i < pp.total_projs; i++) {
				temp += conj(pron.overlaps[i]) * ppron.overlaps[i];
				//if (w % NUM_KPTS == 0) {
				//	printf("%lf %lf %lf %lf\n", creal(pron.overlaps[i]), cimag(pron.overlaps[i]), creal(ppron.overlaps[i]), cimag(ppron.overlaps[i]));
				//}
			}
		}
		overlap[2*w] += creal(temp);
		overlap[2*w+1]+= cimag(temp);
		//printf("temp 3 %d %d %d %lf %lf\n", kpt_S->num_waves, kpt_R->num_waves, w%NUM_KPTS, creal(temp), cimag(temp));

		temp = 0 + 0 * I;
		for (int s = 0; s < num_N_RS; s++) {
			ppot_t pp = pps[ref_labels[N_RS_R[s]]];
			int site_num1 = N_RS_R[s];
			int site_num2 = N_RS_S[s];
			projection_t pron = band_R->projections[site_num1];
			projection_t ppron = band_S->projections[site_num2];
			for (int i = 0; i < pron.total_projs; i++) {
				for (int j = 0; j < ppron.total_projs; j++) {
					temp += conj(pron.overlaps[i])
						* (N_RS_overlaps[s][i*ppron.total_projs+j])
						* ppron.overlaps[j] * cexp(2*I*PI *
						dot(kpt_R->k, wf_S->dcoords + 3*s));
				}
			}
		}
		overlap[2*w] += creal(temp);
		overlap[2*w+1]+= cimag(temp);
		//printf("temp 4 %lf %lf\n", creal(temp), cimag(temp));
	}

	mkl_free_buffers();
	return overlap;
}
