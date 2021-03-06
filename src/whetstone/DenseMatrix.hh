/*
  WhetStone, version 2.0
  Release name: naka-to.

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#ifndef AMANZI_DENSE_MATRIX_HH_
#define AMANZI_DENSE_MATRIX_HH_

#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <iomanip>

#include "lapack.hh"
#include "DenseVector.hh"

namespace Amanzi {
namespace WhetStone {

const int WHETSTONE_DATA_ACCESS_COPY = 1;
const int WHETSTONE_DATA_ACCESS_VIEW = 2;

class DenseMatrix {
 public:
  DenseMatrix();
  DenseMatrix(int mrow, int ncol);
  DenseMatrix(int mrow, int ncol, double* data, int data_access = WHETSTONE_DATA_ACCESS_COPY);
  DenseMatrix(const DenseMatrix& B);
  DenseMatrix(const DenseMatrix& B, int m1, int m2, int n1, int n2);
  ~DenseMatrix() { if (data_ != NULL && access_ == WHETSTONE_DATA_ACCESS_COPY) { delete[] data_; } }
  
  // primary members 
  void clear() { for (int i = 0; i < m_ * n_; i++) data_[i] = 0.0; } 

  double& operator()(int i, int j) { return data_[j * m_ + i]; }
  const double& operator()(int i, int j) const { return data_[j * m_ + i]; }

  DenseMatrix& operator=(const DenseMatrix& B) {
    if (this != &B) {
      if (m_ * n_ != B.m_ * B.n_ && B.m_ * B.n_ != 0) {
        if (data_ != NULL) {
          delete [] data_;
        }
        data_ = new double[B.m_ * B.n_];
      }
      n_ = B.n_;
      m_ = B.m_;
      const double *b = B.Values();
      for (int i = 0; i < m_ * n_; i++) data_[i] = b[i];
    }
    return (*this);
  }

  DenseMatrix& operator=(double val) {
    for (int i = 0; i < m_ * n_; i++) data_[i] = val;
    return *this;
  }

  DenseMatrix& operator*=(double val) {
    for (int i = 0; i < m_ * n_; i++) data_[i] *= val;
    return *this;
  }

  DenseMatrix& operator/=(double val) {
    for (int i = 0; i < m_ * n_; i++) data_[i] /= val;
    return *this;
  }

  DenseMatrix& operator+=(const DenseMatrix& A) {
    for (int i = 0; i < m_ * n_; i++) data_[i] += A.data_[i];
    return *this;
  }

  int Multiply(const DenseMatrix& A, const DenseMatrix& B, bool transposeA);
  int Multiply(const DenseVector& A, DenseVector& B, bool transpose) const;

  void PutScalar(double val) {
    for (int i = 0; i < m_ * n_; i++) data_[i] = val;
  }

  // access: the data are ordered by columns
  int NumRows() const { return m_; }
  int NumCols() const { return n_; }

  inline double* Values() { return data_; }
  inline double* Value(int i, int j)  { return data_ + j * m_ + i; } 
  inline const double* Values() const { return data_; }
  inline const double* Value(int i, int j) const { return data_ + j * m_ + i; } 

  // output 
  friend std::ostream& operator << (std::ostream& os, const DenseMatrix& A) {
    for (int i = 0; i < A.NumRows(); i++) {
      for (int j = 0; j < A.NumCols(); j++) {
        os << std::setw(12) << std::setprecision(12) << A(i, j) << " ";
      }
      os << "\n";
    }
    return os;
  }
  void PrintMatrix() const {
    for (int i = 0; i < m_; i++) {
      for (int j = 0; j < n_; j++) {
        printf("%12.5f", *(data_ + j * m_ + i));
      }
      printf("\n");
    }
  }

  // First level routines
  // -- trace of a rectangular matrix
  double Trace();

  // -- extrema in rows and columns
  void MaxRowValue(int irow, int* j, double* value) { 
    MaxRowValue(irow, 0, n_, j, value); 
  } 
  void MaxRowValue(int irow, int jmin, int jmax, int* j, double* value); 

  void MaxRowMagnitude(int irow, int* j, double* value) { 
    MaxRowMagnitude(irow, 0, n_, j, value); 
  } 
  void MaxRowMagnitude(int irow, int jmin, int jmax, int* j, double* value); 

  double NormInf() {
    double a = 0.0;
    for (int i = 0; i < m_ * n_; i++) a = std::max(a, data_[i]);
    return a;
  }

  void Scale(double value) {
    for (int i = 0; i < m_ * n_; i++) data_[i] *= value;
  }

  // Second level routines
  // -- inversion is applicable for square matrices only
  int Inverse();
  int NullSpace(DenseMatrix& D);
  double Det();  // limited capabilities

  // -- orthonormalize matrix columns between n1 and n2-1.
  //    Returns 0 is sucessful.
  int OrthonormalizeColumns(int n1, int n2);

  // -- permutations
  void SwapColumns(int n1, int n2);

 private:
  int m_, n_, access_;
  double* data_;                       
};


inline bool operator==(const DenseMatrix& A, const DenseMatrix& B) {
  if (A.NumRows() != B.NumRows()) return false;
  if (A.NumCols() != B.NumCols()) return false;
  for (int i = 0; i != A.NumRows() * A.NumCols(); ++i) 
    if (A.Values()[i] != B.Values()[i]) return false;
  return true;
}


inline bool operator!=(const DenseMatrix& A, const DenseMatrix& B) {
  return !(A == B);
}

}  // namespace WhetStone
}  // namespace Amanzi

#endif
