# -*- mode: cmake -*-

#
#  Amanzi
#    Chemistry Reactions
#

# Chemistry includes
include_directories(${CHEM_INCLUDES_DIR})


#
# library
#
set(SRCS 
	./aqueous_equilibrium_complex.cc 
	./beaker.cc 
	./general_rxn.cc 
	./radioactive_decay.cc 
	./ion_exchange_complex.cc 
	./ion_exchange_rxn.cc 
	./kinetic_rate.cc 
	./kinetic_rate_tst.cc 
	./mineral.cc 
	./mineral_kinetics_factory.cc 
	./secondary_species.cc 
	./simple_thermo_database.cc
	./surface_complex.cc 
	./surface_complexation_rxn.cc 
	./sorption_isotherm.cc 
	./sorption_isotherm_factory.cc 
	./sorption_isotherm_freundlich.cc 
	./sorption_isotherm_langmuir.cc 
	./sorption_isotherm_linear.cc 
	./sorption_isotherm_rxn.cc 
)

add_amanzi_library(geochemrxns SOURCE ${SRCS}) 

