/*
  Operators

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon (ecoon@lanl.gov)

  Non-polynomial solution with constant tensor and non-constant scalar coefficients.
*/

#ifndef AMANZI_OPERATOR_ANALYTIC_04_HH_
#define AMANZI_OPERATOR_ANALYTIC_04_HH_

#include <math.h>
#include "AnalyticBase.hh"

class Analytic04 : public AnalyticBase {
 public:
  Analytic04(Teuchos::RCP<const Amanzi::AmanziMesh::Mesh> mesh) : AnalyticBase(mesh) {};
  ~Analytic04() {};

  Amanzi::WhetStone::Tensor Tensor(const Amanzi::AmanziGeometry::Point& p, double t) {
    Amanzi::WhetStone::Tensor K(1,1);
    K(0, 0) = 1.0;
    return K;
  }

  double ScalarCoefficient(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    double k;
    if (x < 0.0) {
      k = 0.0;
    } else if (x > Teuchos::Utils::pi()/2.0) {
      k = 1.0;
    } else {
      k = std::pow(std::sin(x),2);
    }
    return k;      
  }

  double pressure_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    double k;
    if (x < 0.0) {
      k = 0.0;
    } else if (x > Teuchos::Utils::pi()/2.0) {
      k = 1.0;
    } else {
      k = std::pow(std::sin(x),2);
    }
    return k;      
  }

  Amanzi::AmanziGeometry::Point gradient_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    Amanzi::AmanziGeometry::Point v(2);
    v[1] = 0.0;
    if (x < 0.0) {
      v[0] = 0.0;
    } else if (x > Teuchos::Utils::pi() / 2.0) {
      v[0] = 0.0;
    } else {
      v[0] = 2.0 * std::sin(x) * std::cos(x);
    }
    return v;
  }

  double source_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    double q;
    if (x < 0.0) {
      q = 0.0;
    } else if (x > Teuchos::Utils::pi()/2.0) {
      q = 0.0;
    } else {
      q = 6 * std::pow(std::sin(x), 2) * std::pow(std::cos(x), 2) - 2 * std::pow(std::sin(x), 4);
    }
    return q;
  }
};

#endif

