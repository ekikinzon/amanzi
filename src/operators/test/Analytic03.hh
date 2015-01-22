/*
  This is the operators component of the Amanzi code.

  License: BSD
  Authors: Konstantin Lipnikov (lipnikov@lanl.gov)

  Discrete source operator.
*/

#include "AnalyticBase.hh"

class Analytic03 : public AnalyticBase {
 public:
  Analytic03(Teuchos::RCP<const Amanzi::AmanziMesh::Mesh> mesh) : AnalyticBase(mesh) {
    k1 = 1.0;
    k2 = 20.0;
    a1 = 1.0 / k1;
    a2 = 1.0 / k2;
    b2 = (a1 - a2) / 4;

    dim = mesh_->space_dimension();
  }
  ~Analytic03() {};

  Amanzi::WhetStone::Tensor Tensor(const Amanzi::AmanziGeometry::Point& p, double t) {
    double x = p[0];
    double y = p[1];
    Amanzi::WhetStone::Tensor K(dim, 1);
    if (x < 0.5) { 
      K(0, 0) = k1 * (1.0 + x * sin(y));
    } else {
      K(0, 0) = k2 * (1.0 + 2 * x * x * sin(y));
    }
    return K;
  }

  // gradient of scalar factor of the tensor
  Amanzi::AmanziGeometry::Point ScalarTensorGradient(const Amanzi::AmanziGeometry::Point& p, double t) {
    double x = p[0];
    double y = p[1];
    Amanzi::AmanziGeometry::Point v(dim);
    if (x < 0.5) { 
      v[0] = k1 * sin(y);
      v[1] = k1 * x * cos(y);
    } else {
      v[0] = k2 * 4 * x * sin(y);
      v[1] = k2 * 2 * x * x * cos(y);
    }
    return v;
  }

  double pressure_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    if (x < 0.5) { 
      return a1 * x * x + y * y;
    } else {
      return a2 * x * x + y * y + b2;
    }
  }

  Amanzi::AmanziGeometry::Point velocity_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    double kmean;
    Amanzi::AmanziGeometry::Point v(dim);

    kmean = (Tensor(p, t))(0, 0);
    v = gradient_exact(p, t); 
    return -kmean * v;
  }
 
  Amanzi::AmanziGeometry::Point gradient_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    Amanzi::AmanziGeometry::Point v(dim);
    if (x < 0.5) { 
      v[0] = 2 * a1 * x;
      v[1] = 2 * y;
    } else {
      v[0] = 2 * a2 * x;
      v[1] = 2 * y;
    }
    return v;
  }

  double source_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];

    double plaplace, pmean, kmean;
    Amanzi::AmanziGeometry::Point pgrad(dim), kgrad(dim);

    kmean = (Tensor(p, t))(0, 0);
    kgrad = ScalarTensorGradient(p, t);

    pmean = pressure_exact(p, t);
    pgrad = gradient_exact(p, t);

    if (x < 0.5) { 
      plaplace = 2 * (1.0 + a1);
    } else {
      plaplace = 2 * (1.0 + a2);
    }

    return -kgrad * pgrad - kmean * plaplace;
  }

 private:
  int dim;
  double k1, k2;
  double a1, a2, b2;
};
