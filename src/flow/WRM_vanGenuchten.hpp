/*
This is the flow component of the Amanzi code. 

Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
Amanzi is released under the three-clause BSD License. 
The terms of use and "as is" disclaimer for this license are 
provided in the top-level COPYRIGHT file.

Authors: Neil Carlson (version 1)
         Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#ifndef __VAN_GENUCHTEN_MODEL_HPP__
#define __VAN_GENUCHTEN_MODEL_HPP__

#include "WaterRetentionModel.hpp"

namespace Amanzi {
namespace AmanziFlow {

class WRM_vanGenuchten : public WaterRetentionModel {
 public:
  explicit WRM_vanGenuchten(std::string region, double m, double alpha, double sr, 
                            std::string krel_function, double pc0 = 0.0);
  ~WRM_vanGenuchten() {};
  
  // required methods from the base class
  double k_relative(double pc);
  double saturation(double pc);
  double dSdPc(double pc);  
  double capillaryPressure(double saturation);
  double residualSaturation() { return sr_; }

 private:
  double m_;  // van Genuchten parameters: m, n, alpha
  double n_; 
  const double alpha_; 
  const double sr_;  // van Genuchten residual saturation

  const double pc0_;  // regularization threshold (ususally 0 to 500 Pa)
  double factor_dSdPc_;  // frequently used constant
  double se_pc0, se_pc1, f_pc0, f_pc1, fab; 
};

}  // namespace AmanziFlow
}  // namespace Amanzi
 
#endif
