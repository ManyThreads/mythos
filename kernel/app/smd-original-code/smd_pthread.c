// Argon gas MD simulation

// Copyright (c) 2016/2017, HLRS - University of Stuttgart
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the <organization> nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Contact: Daniel Rubio Bonilla
//          rubio@hlrs.de

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

// Number of Threads by default
#ifndef NTH
#define NTH 4
#endif

// Maximum number of supported threads
#ifndef MTH
#define MTH 128
#endif


int D1_N;
int D2_N;
int D3_N;
#define INDEX(X,Y,Z) (((Z)*D1_N*D2_N)+((Y)*D1_N)+(X))


const double ARmass = 39.948; // A.U.s
const double ARsigma = 3.405; // Angstroms
const double AReps   = 119.8; // Kelvins
const double CellDim = 12.0;  // Angstroms
const int    NPartPerCell = 20; // Number of atoms per cell


int _nextP = 0;


typedef struct {
  double x;
  double y;
  double z;
} tri;

tri* pos;
tri* fos;
tri* acs;
tri* ves;


typedef struct {
  int* particles;
  uint np;
} Cell;

Cell** cells;

// A Simple timer for measuring the walltime
double WallTime(void) {
  static long zsec =0;
  static long zusec = 0;
  double esec;
  struct timeval tp;
  struct timezone tzp;
  gettimeofday(&tp, &tzp);
  if(zsec ==0)
    zsec  = tp.tv_sec;
  if(zusec==0)
    zusec = tp.tv_usec;
  esec = (tp.tv_sec-zsec)+(tp.tv_usec-zusec)*0.000001;
  return esec;
}


double update(int p) {
  double DT = 0.000911633;
  acs[p].x = fos[p].x/ARmass;
  acs[p].y = fos[p].y/ARmass;
  acs[p].z = fos[p].z/ARmass;
  ves[p].x += acs[p].x*0.5*DT;
  ves[p].y += acs[p].y*0.5*DT;
  ves[p].z += acs[p].z*0.5*DT;
  pos[p].x += DT*ves[p].x;
  pos[p].y += DT*ves[p].y;
  pos[p].z += DT*ves[p].z;
  fos[p].x = 0.0;
  fos[p].y = 0.0;
  fos[p].z = 0.0;
  return 0.5*ARmass*(ves[p].x*ves[p].x + ves[p].y*ves[p].y + ves[p].z*ves[p].z);
}




void cellInit(Cell* c, int x, int y, int z, uint nParticles) {
  c->np = nParticles;
  c->particles = (int*) malloc(sizeof(int) *nParticles);
  int* particles = c->particles;

  for (uint i = 0; i< nParticles; i++) {
    particles[i] = _nextP;
    _nextP++;
  }

  
  //  We will be filling the cells, making sure than
  //  No atom is less than 2 Angstroms from another
  double rcelldim = ((double)CellDim);
  double centerx = rcelldim*((double)x) + 0.5*rcelldim;
  double centery = rcelldim*((double)y) + 0.5*rcelldim;
  double centerz = rcelldim*((double)z) + 0.5*rcelldim;
   
  // Randomly initialize particles to be some distance 
  // from the center of the square
  // place first atom in cell
  pos[particles[0]].x = centerx + ((drand48()-0.5)*(CellDim-2));
  pos[particles[0]].y = centery + ((drand48()-0.5)*(CellDim-2));
  pos[particles[0]].z = centerz + ((drand48()-0.5)*(CellDim-2));
  
  double r,rx,ry,rz;
  for (uint i = 1; i < nParticles; i++) {
    r = 0;
    while(r < 4.0) {   // square of 2
      r = 4.00001;
      rx = centerx + ((drand48()-0.5)*(CellDim-2)); 
      ry = centery + ((drand48()-0.5)*(CellDim-2));
      rz = centerz + ((drand48()-0.5)*(CellDim-2));
      //loop over all other current atoms
      for(uint j = 0; j<i; j++){
	double rxj = rx - pos[particles[j]].x;
	double ryj = ry - pos[particles[j]].y;
	double rzj = rz - pos[particles[j]].z;
	double rt = rxj*rxj + ryj*ryj + rzj*rzj;
	if(rt < r) r=rt;
      }
    }
    pos[particles[i]].x = rx;
    pos[particles[i]].y = ry;
    pos[particles[i]].z = rz;
  }
}



//Force calculation
//------------------------------------
double interact(int atom1, int atom2){
  double rx,ry,rz,r,fx,fy,fz,f;
  double sigma6,sigma12;
  double _z, _z3;
  
  // computing base values
  rx = pos[atom1].x - pos[atom2].x;
  ry = pos[atom1].y - pos[atom2].y;
  rz = pos[atom1].z - pos[atom2].z;
  r = rx*rx + ry*ry + rz*rz;

  if(r < 0.000001)
    return 0.0;
  
  r=sqrt(r);
  _z = ARsigma/r;
  _z3 = _z*_z*_z;
  sigma6 = _z3*_z3;
  sigma12 = sigma6*sigma6;
  f = ((sigma12-0.5*sigma6)*48.0*AReps)/r;
  fx = f * rx;
  fy = f * ry;
  fz = f * rz;
  // updating particle properties
  fos[atom1].x += fx;
  fos[atom1].y += fy;
  fos[atom1].z += fz;
  return 4.0*AReps*(sigma12-sigma6);
}


int ComputeAtomsPerCell(uint *sizes, 
			int NRows, int NCols, int NPlanes,
			int NParts){
  
  int max = NPartPerCell;

  for(int k=0; k<NPlanes; k++) {
    for(int j=0; j<NCols; j++) {
      for(int i=0; i<NRows; i++) {
	sizes[INDEX(i, j, k)] = NPartPerCell;
      }
    }
  }
  
  int molsum = NRows*NCols*NPlanes*NPartPerCell;
  while(molsum < NParts){
    max++;
    for(int k=0;k<NPlanes;k++){
      for(int j=0;j<NCols;j++){
	for(int i=0;i<NRows;i++){
	  sizes[INDEX(i, j, k)]++;
	  molsum++;
	  if(molsum >= NParts) {
	    return max;
	  }
	}
      }
    }
  }
  
  return max;
}


typedef struct _thread_data_t1 {
  int nelems;
  int start;
  int id;
  double* w;
} td1_t;
typedef struct _thread_data_t2 {
  int nelems;
  int start;
  int id;
  double* w;
} td2_t;



void *f1(void* arg) {
  td1_t* data = (td1_t*)arg;
  double _k = 0.0;

  for (int cz1 = data->start; cz1 < data->start+data->nelems; cz1++) {
    for (int cy1 = 0; cy1 < D2_N; cy1++) {
      for (int cx1 = 0; cx1 < D1_N; cx1++) {
	
	// Consider interactions between particles within the cell
	for (uint i = 0; i < cells[INDEX(cx1,cy1,cz1)]->np; i++) {
	  for (uint j = 0; j < cells[INDEX(cx1,cy1,cz1)]->np; j++) {
	    if(i!=j) {
	      _k += interact(cells[INDEX(cx1,cy1,cz1)]->particles[i],
			     cells[INDEX(cx1,cy1,cz1)]->particles[j]);
	    }
	  }
	}
	
	
	// Consider each other cell... 
	for (int cz2 = 0; cz2 < D3_N; cz2++) {
	  for (int cy2 = 0; cy2 < D2_N; cy2++) {
	    for (int cx2 = 0; cx2 < D1_N; cx2++) {
	      
	      // if condition is true if cell[cy1][cy2] is neighbor of cell[cx1][cy1] 
	      if ((abs(cx1-cx2) < 2) && (abs(cy1-cy2) < 2) && (abs(cz1-cz2) < 2) &&
		  (cx1 != cx2 || cy1 != cy2 || cz1 != cz2)) {
		// Consider all interactions between particles
		for (uint i = 0; i < cells[INDEX(cx1,cy1,cz1)]->np; i++) {
		  for (uint j = 0; j < cells[INDEX(cx2,cy2,cz2)]->np; j++) {
		    _k += interact(cells[INDEX(cx1,cy1,cz1)]->particles[i],
				   cells[INDEX(cx2,cy2,cz2)]->particles[j]);
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
    
  *(data->w) = _k;
  pthread_exit(NULL);
}



void *f2(void* arg) {
  td2_t* data = (td2_t*)arg;
  double _k = 0.0;

  for (int cz1 = data->start; cz1 < data->start+data->nelems; cz1++) {
    for (int cy1 = 0; cy1 < D2_N; cy1++) {
      for (int cx1 = 0; cx1 < D1_N; cx1++) {
	for(uint i=0; i < cells[INDEX(cx1,cy1,cz1)]->np; i++) {
	  _k += update(cells[INDEX(cx1,cy1,cz1)]->particles[i]);
	}
      }
    }
  }

  *(data->w) = _k;
  pthread_exit(NULL);
}



int main(int argc,char* argv[]){
  int NumParticles;
  int NumIterations;
  int nth;

  //// THREAD CODE ////
  pthread_t thr1[MTH];
  pthread_t thr2[MTH];
  td2_t thr_data1[MTH];
  td2_t thr_data2[MTH];
  double pk1[MTH];
  double pk2[MTH];
  /////////////////////
  
  // Get Command Line Arguments
  if(argc==3) {
    NumParticles = atoi(argv[1]);
    NumIterations  = atoi(argv[2]);
    nth = NTH;
  }
  else if(argc==4) {
    NumParticles = atoi(argv[1]);
    NumIterations  = atoi(argv[2]);
    nth = atoi(argv[3]);
  }
  else {
    printf("Incorrect syntax: should be two or three arguments\n");
    exit(2);
  }
  

  int Dimension   = (int)pow(NumParticles/((double)NPartPerCell), 1.0/3.0);
  int TotalCells  = Dimension*Dimension*Dimension;
  D1_N = Dimension;
  D2_N = Dimension;
  D3_N = Dimension;
  int MaxCellSize;

  uint* CellSizes = (uint*) malloc(sizeof(uint*)*D1_N*D2_N*D3_N);
  MaxCellSize = ComputeAtomsPerCell(CellSizes, D1_N, D2_N, D3_N, NumParticles);


  pos = (tri*) malloc(sizeof(tri)*NumParticles);
  fos = (tri*) malloc(sizeof(tri)*NumParticles);
  acs = (tri*) malloc(sizeof(tri)*NumParticles);
  ves = (tri*) malloc(sizeof(tri)*NumParticles);


  printf("\nThe Total Number of Cells is %d", TotalCells);
  printf(" With a maximum of %d particles per cell,\n", MaxCellSize);
  printf("and %d particles total in system\n", NumParticles);;


  // Allocate Cell, space for Particles couched in the Cell creator
  cells = (Cell**) malloc(sizeof(Cell*)*D1_N*D2_N*D3_N);
  for (int k = 0; k < D3_N; k++) {
    for (int j = 0; j < D2_N; j++) {
      for (int i = 0; i < D1_N; i++) {
	cells[INDEX(i, j, k)] = (Cell*) malloc(sizeof(Cell));
	cellInit(cells[INDEX(i, j, k)], i, j, k, CellSizes[INDEX(i, j, k)]);
      }
    }
  }

  
  
  double TimeStart = WallTime();
  for (int t = 0; t < NumIterations; t++) { // For each timestep
    double TotPot=0.0;
    double TotKin=0.0;
 
    // Set elements per thread
    int _n = D3_N / nth;
    int _e = D3_N % nth;
    int _s = 0;
    for (int i=0; i<nth; i++) {
      thr_data1[i].start = _s;
      thr_data1[i].nelems = _n;
      thr_data1[i].id = i;
      if(_e) {
	thr_data1[i].nelems++;
	_e--;
      }
      _s += thr_data1[i].nelems;

      thr_data1[i].w = &pk1[i];
    }
    

    /* create threads */
    int rc;
    for (int i = 0; i < nth; ++i) {
      if ((rc = pthread_create(&thr1[i], NULL, f1, &thr_data1[i]))) {
	fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
	return EXIT_FAILURE;
      }
    }
    /* block until all threads complete */
    for (int i = 0; i < nth; ++i) {
      pthread_join(thr1[i], NULL);
    }


    for (int i = 0; i < nth; ++i) {
      TotPot += pk1[i];
    }

    
    // Set elements per thread
    _n = D3_N / nth;
    _e = D3_N % nth;
    _s = 0;
    for (int i=0; i<nth; i++) {
      thr_data2[i].start = _s;
      thr_data2[i].nelems = _n;
      thr_data2[i].id = i;
      if(_e) {
	thr_data2[i].nelems++;
	_e--;
      }
      _s += thr_data2[i].nelems;

      thr_data2[i].w = &pk2[i];
    }


    /* create threads */
    for (int i = 0; i < nth; ++i) {
      if ((rc = pthread_create(&thr2[i], NULL, f2, &thr_data2[i]))) {
	fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
	return EXIT_FAILURE;
      }
    }
    /* block until all threads complete */
    for (int i = 0; i < nth; ++i) {
      pthread_join(thr2[i], NULL);
    }


    for (int i = 0; i < nth; ++i) {
      TotKin += pk2[i];
    }
    

    printf("\nIteration#%d with Total Energy %e per Atom",
	   t,(TotPot+TotKin)/NumParticles);
  } // iterate time steps

  double TimeEnd = WallTime();
  printf("\nTime for %d  is %f\n", NumIterations, TimeEnd-TimeStart);

  
  // Free Memory
  for (int k = 0; k < D3_N; k++) {
    for (int j = 0; j < D2_N; j++) {
      for (int i = 0; i < D1_N; i++) {
	free(cells[INDEX(i, j, k)]->particles);
      }
    }
  }
  
  free(pos);
  free(fos);
  free(acs);
  free(ves);
  free(CellSizes);
    
  return 0;
}
