# -*- mode: cmake -*-

project (HANTUSH_ANISOTROPIC_2D)

add_amanzi_comparison_test(hantush_anisotropic_2d
                           INPUT amanzi_hantush_anisotropic_2d.xml
                           FILES porflow4_6.exo
                           TOLERANCE 2e-3
                           FIELD hydraulic_head.cell.0
                           REFERENCE ${HANTUSH_ANISOTROPIC_2D_SOURCE_DIR}/golden_output/plot_data.h5)
