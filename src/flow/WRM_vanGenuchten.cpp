/*
This is the flow component of the Amanzi code. 

Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
Amanzi is released under the three-clause BSD License. 
The terms of use and "as is" disclaimer for this license are 
provided in the top-level COPYRIGHT file.

Authors: Neil Carlson (version 1)
         Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#include <cmath>
#include <string>

#include "WRM_vanGenuchten.hpp"

namespace Amanzi {
namespace AmanziFlow {

/* ******************************************************************
* Setup fundamental parameters for this model.
* Default value of the regularization interval is pc0 = 0.                                           
****************************************************************** */
WRM_vanGenuchten::WRM_vanGenuchten(
    std::string region, double m, double alpha, double sr, std::string krel_function, double pc0)
    : m_(m), alpha_(alpha), sr_(sr), pc0_(pc0)
{
  n_ = 1.0 / (1.0 - m_);
  set_region(region);

  factor_dSdPc_ = -m_ * n_ * alpha_ * (1.0 - sr_);

  if (pc0 > 0) {
    se_pc0 = pow(1.0 + pow(alpha_*pc0_, n_), -m_);
    f_pc0 = sqrt(se_pc0) * pow(1.0 - pow(1.0 - pow(se_pc0, 1.0/m_), m_), 2);
    fab = (f_pc0 - 1.0) / pc0_;

    se_pc1 = pow(1.0 + pow(alpha_ * (pc0_ + 1.0), n_), -m_);
    f_pc1 = sqrt(se_pc1) * pow(1.0 - pow(1.0 - pow(se_pc1, 1.0/m_), m_), 2);
  }
}


/* ******************************************************************
* Relative permeability formula: input is capillary pressure pc.
* The original curve is regulized on interval (0, pc0) using the 
* Hermite interpolant of order 3.       
****************************************************************** */
double WRM_vanGenuchten::k_relative(double pc)
{
  if (pc > pc0_) {
    double se = pow(1.0 + pow(alpha_*pc, n_), -m_);
    return sqrt(se) * pow(1.0 - pow(1.0 - pow(se, 1.0/m_), m_), 2.0);
  } else if (pc <= 0.0) {
    return 1.0;
  } else {
    return 1.0 + pc*pc * fab/pc0_ + pc*pc * (pc-pc0_) * (f_pc1-f_pc0-2*fab) / (pc0_*pc0_);
  }
}


/* ******************************************************************
* Saturation formula (3.5)-(3.6).                                         
****************************************************************** */
double WRM_vanGenuchten::saturation(double pc)
{
  if (pc > 0.0) {
    return pow(1.0 + pow(alpha_*pc, n_), -m_) * (1.0 - sr_) + sr_;
  } else {
    return 1.0;
  }
}


/* ******************************************************************
* Derivative of the saturation formula w.r.t. capillary pressure.
* Warning: remember that dSdP = -dSdPc.                                        
****************************************************************** */
double WRM_vanGenuchten::dSdPc(double pc)
{
  if (pc > 0.0) {
    double alpha_pc = alpha_*pc;
    double x = pow(alpha_pc, n_-1.0);
    double y = x * alpha_pc;
    return pow(1.0 + y, -m_-1.0) * x * factor_dSdPc_;
  } else {
    return 0.0;
  }
}


/* ******************************************************************
* Pressure as a function of saturation.                                       
****************************************************************** */
double WRM_vanGenuchten::capillaryPressure(double s)
{
  double se = (s - sr_) / (1.0 - sr_);
  return (pow(pow(se, -1.0/m_) - 1.0, 1/n_)) / alpha_;
}

}  // namespace AmanziFlow
}  // namespace Amanzi

