# -*- mode: cmake -*-

#
#  Amanzi
#    Chemistry Base
#


set(SRCS 
	./activity_model_factory.cc
	./activity_model.cc
	./activity_model_debye_huckel.cc
	./activity_model_pitzer_hwm.cc 
	./virial_coefficient.cc
	./activity_model_unit.cc
	./ion_exchange_site.cc
	./species.cc
	./surface_site.cc
)

add_amanzi_library(geochembase SOURCE ${SRCS}) 
