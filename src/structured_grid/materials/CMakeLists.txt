# -*- mode: cmake -*-
#
# Define a project name
# After this command the following varaibles are defined
#   STRUCTMAT_SOURCE_DIR
#   STRUCTMAT_BINARY_DIR
# Other projects (subdirectories) can reference this directory
# through these variables.
project(STRUCTMAT)

set(CXX_header_files Material.H MatFiller.H MatFiller_F.H)
set(CXX_source_files Material.cpp MatFiller.cpp)
set(FPP_source_files MatFiller_${AMANZI_SPACEDIM}D.F)


include_directories(${STRUCTMAT_SOURCE_DIR})
include_directories(${ATK_SOURCE_DIR})
include_directories(${PLATFORM_INTERFACE_SOURCE_DIR})

# The interfaces
include_directories(${INTERFACE_PLATFORM_DIR})
include_directories(${AMANZI_BINARY_DIR}) # required to pick up amanzi_version.hh

#
# The structured grid pieces
#
include_directories(${CCSE_INCLUDE_DIRS})
include_directories(${PMAMR_INCLUDE_DIRS})
link_directories(${CCSE_LIBRARY_DIR})
include_directories(${BOXDRIV_SOURCE_DIR})
link_directories(${BOXDRIV_SOURCE_DIR})
include_directories(${STRUCTUREDUTILS_SOURCE_DIR})
link_directories(${STRUCTUREDUTILS_SOURCE_DIR})
add_definitions("-DENABLE_Structured")

#
# The common pieces
#
include_directories(${DBC_SOURCE_DIR})
include_directories(${CHEM_INCLUDES_DIR})

# External (TPL) include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${Teuchos_INCLUDE_DIRS})
include_directories(${Epetra_INCLUDE_DIRS})


preprocess_boxlib_fortran(FPP_out_files ${FPP_source_files})

add_amanzi_library(structmat
                   SOURCE ${FPP_out_files} ${CXX_source_files} ${F90_source_files} ${F77_source_files}
                   HEADERS ${CXX_headers_files})


set(structured_grid_libs pmamr gsint boxprob boxdriv structmat structutils ${CCSE_LIBRARIES})
link_directories(${CCSE_EXT_LIBRARY_DIRS})
if (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  #message(">>>>>> using darwin ld flags")
  set(ld_flag_prefix -force_load)
  set(ld_flag_suffix)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  #message(">>>>>> using linux ld flags")
  set(ld_flag_prefix -Wl,-whole-archive)
  set(ld_flag_suffix -Wl,-no-whole-archive)
else()
  message(FATAL_ERROR "Linking of structured grid is not supported on this system: '${CMAKE_SYSTEM_NAME}'")
endif()

foreach(lib ${structured_grid_libs})
  set(ssc_structured_grid_libs ${ssc_structured_grid_libs} ${ld_flag_prefix} ${lib} ${ld_flag_suffix})
endforeach(lib)
list(APPEND ssc_structured_grid_libs ${PETSC_LIBRARIES})

get_property(GEOCHEM_LIBS GLOBAL PROPERTY GEOCHEM_LIBS)
set(ssc_common_libs error_handling ${GEOCHEM_LIBS} interface atk)

set(ssc_tpl_libs_incl ${ssc_structured_grid_libs} ${ssc_common_libs})
set(ssc_tpl_libs ${Teuchos_LIBRARIES} ${Epetra_LIBRARIES} ${Boost_LIBRARIES})

list(APPEND ssc_tpl_libs ${CCSE_EXT_LIBRARIES})

if (BUILD_TESTS) 

  add_amanzi_test(MaterialIDFiller tMIDf
                  KIND unit
                  SOURCE tMIDf.cpp
                  LINK_LIBS ${structured_grid_libs} ${tpl_libs})

  add_amanzi_test(MaterialPropertyFiller tMatFiller
                  KIND unit
                  SOURCE tMatFiller.cpp
                  LINK_LIBS ${structured_grid_libs} ${tpl_libs})

  add_amanzi_test(MaterialPropertyFiller4 "./tMatFiller_parallel" "case_size=large"
                  KIND unit
	          NPROCS 4
                  SOURCE tMatFiller.cpp
                  LINK_LIBS ${structured_grid_libs} ${tpl_libs})
endif()