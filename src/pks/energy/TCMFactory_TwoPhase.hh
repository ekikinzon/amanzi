/*
  Energy

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon

  Self-registering factory for thermal conductivity models.
*/

#ifndef PK_ENERGY_TCM_FACTORY_TWOPHASE_HH_
#define PK_ENERGY_TCM_FACTORY_TWOPHASE_HH_

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "TCM_TwoPhase.hh"
#include "factory.hh"

namespace Amanzi {
namespace Energy {

class TCMFactory_TwoPhase : public Utils::Factory<TCM_TwoPhase> {
 public:
  Teuchos::RCP<TCM_TwoPhase> CreateTCM(Teuchos::ParameterList& plist);
};

}  // namespace Energy
}  // namespace Amanzi

#endif
