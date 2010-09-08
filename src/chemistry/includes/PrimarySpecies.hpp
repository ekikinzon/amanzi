#ifndef __Primary_Species_hpp__
#define __Primary_Species_hpp__

#include "Teuchos_RCP.hpp"
#include "State.hpp"
#include "Chemistry_State.hpp"


// Base class for species

class PrimarySpecies : public Species {

  PrimarySpecies(const double s_charge, const double s_GMW, 
		   const ActivityCoefficient* s_activityCoefficient, 
		   Species::SpeciesName s_name);

  ~PrimarySpecies();

private:
  std::vector<Species::SpeciesId> primary_list_;
  std::vector<double> primary_coefficients_;


};

#endif
