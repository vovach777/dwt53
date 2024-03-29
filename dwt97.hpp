 #pragma once
/**
 *  dwt97.c - Fast discrete biorthogonal CDF 9/7 wavelet forward and inverse transform (lifting implementation)
 *
 *  This code is provided "as is" and is given for educational purposes.
 *  2006 - Gregoire Pau - gregoire.pau@ebi.ac.uk
 */

/**
 *  fwt97 - Forward biorthogonal 9/7 wavelet transform (lifting implementation)
 *
 *  x is an input signal, which will be replaced by its output transform.
 *  n is the length of the signal, and must be a power of 2.
 *
 *  The first half part of the output signal contains the approximation coefficients.
 *  The second half part contains the detail coefficients (aka. the wavelets coefficients).
 *
 *  See also iwt97.
 */
inline void dwt97(float* x, float *tempbank, int n) {
  float a;
  int i;

  // Predict 1
  a=-1.586134342f;
  for (i=1;i<n-2;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[n-1]+=2*a*x[n-2];

  // Update 1
  a=-0.05298011854f;
  for (i=2;i<n;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[0]+=2*a*x[1];

  // Predict 2
  a=0.8829110762f;
  for (i=1;i<n-2;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[n-1]+=2*a*x[n-2];

  // Update 2
  a=0.4435068522f;
  for (i=2;i<n;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[0]+=2*a*x[1];

  // Scale
  a=1/1.149604398f;
  for (i=0;i<n;i++) {
    if (i%2) x[i]*=a;
    else x[i]/=a;
  }

  // Pack
  for (i=0;i<n;i++) {
    if (i%2==0) tempbank[i/2]=x[i];
    else tempbank[n/2+i/2]=x[i];
  }
  for (i=0;i<n;i++) x[i]=tempbank[i];
}

/**
 *  iwt97 - Inverse biorthogonal 9/7 wavelet transform
 *
 *  This is the inverse of fwt97 so that iwt97(fwt97(x,n),n)=x for every signal x of length n.
 *
 *  See also fwt97.
 */
inline void idwt97(float* x, float *tempbank, int n) {
  float a;
  int i;

  // Unpack
  for (i=0;i<n/2;i++) {
    tempbank[i*2]=x[i];
    tempbank[i*2+1]=x[i+n/2];
  }
  for (i=0;i<n;i++) x[i]=tempbank[i];

  // Undo scale
  a=1.149604398f;
  for (i=0;i<n;i++) {
    if (i%2) x[i]*=a;
    else x[i]/=a;
  }

  // Undo update 2
  a=-0.4435068522f;
  for (i=2;i<n;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[0]+=2*a*x[1];

  // Undo predict 2
  a=-0.8829110762f;
  for (i=1;i<n-2;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[n-1]+=2*a*x[n-2];

  // Undo update 1
  a=0.05298011854f;
  for (i=2;i<n;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[0]+=2*a*x[1];

  // Undo predict 1
  a=1.586134342f;
  for (i=1;i<n-2;i+=2) {
    x[i]+=a*(x[i-1]+x[i+1]);
  }
  x[n-1]+=2*a*x[n-2];
}
