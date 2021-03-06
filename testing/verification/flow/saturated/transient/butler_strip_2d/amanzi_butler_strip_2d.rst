Transient Flow in a 2D Confined Aquifer with a Linear Strip
===========================================================

Introduction
------------

Butler and Liu (1991) :cite:`strip-Butler_Liu_linear_strip_1991` developed a semi-analytical solution for calculating drawdown in an aquifer system, in which an infinite linear strip of one material is embedded in a matrix of different hydraulic properties. The problem of interest is the drawdown as a function of location and time due to pumping from a fully penetrating well located in any of three zones. The problem is solved analytically in the Fourier-Laplace space and the drawdown is solved numerically by inversion from the Fourier-Laplace space to the real space.

Problem Specification
---------------------

Flow within zones that do not contain the pumping well can be described mathematically as :eq:`flow_nowell`

.. math:: \frac{\partial ^2 s_i}{\partial x^2} 
   + \frac{\partial ^2 s_i}{\partial y^2} 
   = \frac{S_i}{T_i} \frac{\partial s_i}{\partial t},
  :label: flow_nowell

where 
:math:`s_i` [L] is the drawdown in material :math:`i`,
:math:`t` [T] is the time,
:math:`T_i` [L\ :sup:`2`\/T] is the transmissivity of material :math:`i`, and
:math:`S_i` [-] is the storage coefficient of material :math:`i`.

Flow within zones that contain the pumping well can be represented as

.. math:: \frac{\partial ^2 s_i}{\partial x^2} 
   + \frac{\partial ^2 s_i}{\partial y^2} 
   + \frac{Q}{T_i} \delta(x-a)\delta(y-b)
   = \frac{S_i}{T_i} \frac{\partial s_i}{\partial t},
  :label: flow_well

where
:math:`Q` [L\ :sup:`3`\/T]is the pumpage from well located at :math:`(a,b)`,
:math:`\delta(x)` is the Direc delta function, being 1 for :math:`x = 0` and :math:`0 \text{ otherwise}`.

The initial conditions are the same for all three zones:

.. math:: s_i(x,y,0) =0.
  :label: ic_ButlerLiu_strip

The boundary conditions are:

.. math:: 
.. math::    s_1(-\infty, y, t) =  0,\\
.. math::      s_i(x,\pm\infty, t) =  0, \\
.. math::     s_3(\infty, y, t) =  0,\\
.. math::     s_1(-d, y, t) =  s_2(-d, y, t),\\
.. math::     s_2(0, y, t) =  s_3(0, y, t),\\
.. math::      T_1\frac{\partial s_1(-d,y,t)}{\partial x} = T_2\frac{\partial s_2(-d,y,t)}{\partial x},\\
.. math::      T_2\frac{\partial s_2(0,y,t)}{\partial x} = T_3\frac{\partial s_3(0,y,t)}{\partial x}.
  :label: bc_ButlerLiu_strip



Problem Specification
---------------------

Schematic
~~~~~~~~~

The domain configuration and well locations are indicated in the following schematic. The origin of the coordinate system is shown in the figure as 'o'.

.. figure:: schematic/butler_strip_schematic.png
    :figclass: align-center
    :width: 600 px

    Figure 1.  Schematic of the Butler and Liu's Linear Strip verification problem 


The boundary conditions are given as: constant pressure head of 1.07785 MPa (i.e., 100m) at all four boundaries and initially the pressure head is 1.07785 MPa everywhere in the domain. The parameter values for the problem are given as:

	Transmissivity: :math:`\;\; T_1 = 0.11574 \; m2/s`; :math:`T_2 = 0.011574 \;m2/s`; :math:`T_3 = 0.0011574 \;m2/s`

	Storativity: :math:`\;\; S_1 = 5\times 10^{-4}`; :math:`S_2 = 2\times 10^{-4}`; :math:`S_3 = 2\times 10^{-5}`;

	Pumping rate: :math:`\;\; Q = 1000 \;m3/day (= 0.011574 \;m3/s)`

	Width of the strip: :math:`\;\; d = 18 \;m`

	Pumping well location :math:`\;\; (-9 m, 0 m)`

	Observation well locations :math:`(15 m, 0 m)` and :math:`(91 m, 0 m)`, which gives the distance between the pumping well and observation wells :math:`r = 24 \;m` and :math:`r = 100 \;m`.


Results and Comparison
----------------------


.. _plot_ButlerLiu_strip:

Comparison of  Analytic Solution and Amanzi Results
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. plot:: amanzi_butler_strip_2d.py
             :align: center


The comparison shows that the results from the Amanzi model match the analytical solution very well at early time, and they deviate when the effect of pumping hits the constant head boundary of the domain. Note that, the analytical solution was developed for unbounded domain, and therefore it is expected that the two solutions will deviate each other at later time.
To show that such a deviation is indeed caused by the boundary effect, we also conducted numerical simulations using 
FEHM, a widely used numerical simulator for simulating heat and mass flow in subsurface environment :cite:`strip-Zyvoloski_FEHM_summary_1997`. It is showed that the results from Amanzi are almost the same as those from FEHM, see :cite:`strip-Lu_Harp_Birdsell_benchmarking_2014` for detailed comparison.

References
----------

.. bibliography:: /bib/ascem.bib
   :filter: docname in docnames
   :style:  alpha
   :keyprefix: strip-

	    
About
-----

* Directory: testing/verification/flow/saturated/transient/butler_strip_2d

* Authors:  Zhiming Lu (zhiming@lanl.gov),  Dylan Harp (dharp@lanl.gov)

* Maintainer(s):  Zhiming Lu,  Dylan Harp

* Input Files: 
  
  * amanzi_butler_strip_2d.xml
 
     * Spec: Version 2.0
     * Mesh: Generated in running time
     * Runs

* Analytical Solutions

  * Directory: analytic/

  * Executable: butler_strip.x, compiled from FORTRAN code under the Linux environment.

  * Input Files:

     * now.dat

  * Output Files:
   
     * drdn.dat,  drawdown as a function of time for all observation wells.

Status
~~~~~~

The analytical solution was solved using a FORTRAN code modified from the original code from Greg Ruskauf.
We may need to implement the algorithm by ourselves or get permission from Greg Ruskauf for using the code.
As the flow problem was solved analytically in the Laplace-Fourier transformed space, one needs to implement
numerical inversion from the Laplace-Fourier transformed space back to the real space.

