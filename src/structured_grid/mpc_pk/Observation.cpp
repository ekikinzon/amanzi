#include <winstd.H>

#include <PorousMedia.H>

Amr* Observation::amrp;
std::map<std::string, int> Observation::obs_type_list;
std::map<std::string, int> Observation::op_type_list;

static bool initialized = false;

void
Observation::Cleanup ()
{
    Observation::amrp = 0;
    Observation::obs_type_list.clear();
    Observation::op_type_list.clear();
    initialized = false;
}

Observation::Observation(const std::string& name,
                         const std::string& field,
                         const Region&      region,
                         const std::string& obs_type,
                         const std::string& op_type,
                         const Array<Real>& times)
    : name(name), field(field), region(region), obs_type(obs_type), op_type(op_type), times(times)
{
    if (!initialized) {
        BoxLib::ExecOnFinalize(Observation::Cleanup);
        initialized = true;

        obs_type_list["average"]          = 0;
        obs_type_list["integral"]         = 1;
        obs_type_list["time_integral"]    = 2;
        obs_type_list["squared_integral"] = 3;
        obs_type_list["flux"]             = 4;
        obs_type_list["point_sample"]     = 5;

        op_type_list["average"]             = 0;
        op_type_list["space_integral"]      = 1;
        op_type_list["space_time_integral"] = 2;
    }

    if (obs_type_list.find(obs_type) == obs_type_list.end()) {
        BoxLib::Abort("Unsupported observation type");
    }

    if (op_type_list.find(op_type) == op_type_list.end()) {
        BoxLib::Abort("Unsupported observation operation type");
    }
}

void 
Observation::process(Real t_old, 
		     Real t_new)
{
  // Must set the amr pointer prior to use via Observation::setAmrPtr
  BL_ASSERT(amrp); 

  // determine observation at time t_new
  switch (obs_type_list[obs_type] )
    {
    case 0:
      val_new  = average(t_new);

      break;
      
    case 1:
      val_new = volume_integral(t_new);
      break;
      
    case 2:
      // times[nObs] = 0.5*(t_old+t_new);
      val_new = volume_time_integral(t_old,t_new);
      break;

    case 5:
      val_new = point_sample(t_new);
      break;

    default:
      ;// Do nothing
    }

  if (!initialized) {
      val_old = val_new;
      initialized = true;
  }

  // determine which of the observations are requested at this time step
  switch (obs_type_list[obs_type] )
    {
    case 2:
      if (vals.size() < 1) 
	{
	  vals[0] = 0.;
	}
      if (times[0] <= t_old && times[1] >= t_new)
	vals[0] = vals[0] + val_new;   
      break;

    default:
      for (int i=0; i<times.size(); i++)
	{
	  if (times[i] >= t_old && times[i] <= t_new)
	    {
              Real eta = std::min(1.,std::max(0.,(times[i]-t_old)/(t_new-t_old)));
	      vals[i] = val_old*(1 - eta) + val_new*eta;
	    }
	}
    }
  val_old = val_new;
}

static IntVect
Index (const pointRegion& region,
       int                lev,
       const Amr*         amr)
{
    BL_ASSERT(amr != 0);
    BL_ASSERT(lev >= 0 && lev <= amr->finestLevel());

    IntVect iv;

    const Geometry& geom = amr->Geom(lev);

    D_TERM(iv[0]=floor((region.coor[0]-geom.ProbLo(0))/geom.CellSize(0));,
           iv[1]=floor((region.coor[1]-geom.ProbLo(1))/geom.CellSize(1));,
           iv[2]=floor((region.coor[2]-geom.ProbLo(2))/geom.CellSize(2)););

    iv += geom.Domain().smallEnd();

    return iv;
}

Real
Observation::point_sample (Real time)
{
  const int finest_level = amrp->finestLevel();
  const Array<IntVect>& refRatio = amrp->refRatio();


  const Region* regPtr = &region;
  const pointRegion* ptreg = dynamic_cast<const pointRegion*>(regPtr);
  if (ptreg == 0) 
  {
      BoxLib::Abort("Point Sample observation requires a point region");
  }

  int proc_with_data = -1;
  Real value;
  for (int lev = finest_level; lev >= 0 && proc_with_data<0; lev--)
  {
      // Compute IntVect at this level of cell containing this point
      IntVect idx = Index(*ptreg,lev,amrp);

      // Decide if this processor owns a fab at this level containing this point
      //
      // FIXME: Do test on grids, derive when necessary
      //
      int nGrow = 0;
      const MultiFab* S_new = amrp->getLevel(lev).derive(field,time,nGrow);

      for (MFIter mfi(*S_new); mfi.isValid() && proc_with_data<0; ++mfi)
      {
          const Box& box = mfi.validbox();

          if (box.contains(idx))
          {
              proc_with_data = ParallelDescriptor::MyProc();
              value = (*S_new)[mfi](idx,0);
          }
      }
      ParallelDescriptor::ReduceIntMax(proc_with_data);
  }

  ParallelDescriptor::Bcast(&value,1,proc_with_data);
  return value;
}

std::pair<Real,Real>
Observation::integral_and_volume (Real time)
{
  const int finest_level = amrp->finestLevel();
  const Array<IntVect>& refRatio = amrp->refRatio();

  Real int_inside = 0.0;
  Real vol_inside = 0;  
  
  Real vol_scale_lev = 1;

  for (int lev = 0; lev <= finest_level; lev++)
    {
      if (lev>0)
        {
          for (int d=0; d<BL_SPACEDIM; ++d)
            {
              vol_scale_lev *= 1./refRatio[lev][d];
            }
        }

      int nGrow = 0;
      const MultiFab* S = amrp->getLevel(lev).derive(name,time,nGrow);
      BoxArray baf;
      if (lev < finest_level)
        {
          BoxArray baf = amrp->boxArray(lev+1);
          baf.coarsen(refRatio[lev]);
        }

      FArrayBox fabVOL, fabINT;
      const Real* dx = amrp->Geom(lev).CellSize();
      Real vol = 1.;
      for (int i=0;i<BL_SPACEDIM;i++) vol *= dx[i];

      for (MFIter mfi(*S); mfi.isValid(); ++mfi)
        {
          const Box& cbox = mfi.validbox();
          fabVOL.resize(cbox,1);

          // Initialize to zero everywhere, then set to 1 inside region
          fabVOL.setVal(0);
          region.setVal(fabVOL,vol,0,dx,0);

          // Zero where covered with better data
          if (lev < finest_level)
            {
              std::vector< std::pair<int,Box> > isects = baf.intersections(cbox);
              
              for (int ii = 0, N = isects.size(); ii < N; ii++)
                {
                  fabVOL.setVal(0,isects[ii].second,0,fabVOL.nComp());
                }
            }

          // Now increment volume and integral inside
          vol_inside += fabVOL.sum(0) * vol_scale_lev;
          
          fabINT.resize(cbox,1);
          fabINT.copy((*S)[mfi],0,0,1);
          fabINT.mult(fabVOL);
          int_inside += fabINT.sum(0);
        }
    }
  ParallelDescriptor::ReduceRealSum(vol_inside);
  ParallelDescriptor::ReduceRealSum(int_inside);
  return std::pair<Real,Real>(int_inside,vol_inside);
}

Real
Observation::average (Real time)
{
  std::pair<Real,Real> int_vol = integral_and_volume(time);
  return (int_vol.second == 0 ? 0 : int_vol.first / int_vol.second);
}

Real
Observation::volume_integral (Real time)
{
  return integral_and_volume(time).first;
}

Real
Observation::volume_time_integral (Real t_old, Real t_new)
{
  // Use trapezoid rule in time on volume integrals.
  std::pair<Real,Real> int_vol_0 = integral_and_volume(t_old);
  std::pair<Real,Real> int_vol_1 = integral_and_volume(t_new);

  return 0.5*(t_new-t_old)*(int_vol_0.first + int_vol_1.first);
}
