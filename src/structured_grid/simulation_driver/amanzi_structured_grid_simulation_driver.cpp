#include "amanzi_structured_grid_simulation_driver.H"
#include "ParmParse.H"
#include "Amr.H"
#include "PorousMedia.H"

#include "ParmParseHelpers.H"

void
Structured_observations(const PArray<Observation>& observation_array,
			Amanzi::ObservationData& observation_data)
{
  for (int i=0; i<observation_array.size(); ++i)
    {
      std::string label = observation_array[i].name;
      int ntimes = observation_array[i].times.size();
      std::vector<Amanzi::ObservationData::DataTriple> dt(ntimes);
      const std::map<int, Real> vals = observation_array[i].vals;
      for (std::map<int, Real>::const_iterator it = vals.begin();
           it != vals.end(); ++it) {
        int j = it->first;
        dt[j].value = it->second;
        dt[j].time = observation_array[i].times[j];
        dt[j].is_valid = true;
      }
      observation_data[label] = dt;
    }	
}  

Amanzi::Simulator::ReturnType
AmanziStructuredGridSimulationDriver::Run (const MPI_Comm&               mpi_comm,
                                           Teuchos::ParameterList&       input_parameter_list,
                                           Amanzi::ObservationData&      output_observations)
{
    int argc=0;
    char** argv;

    BoxLib::Initialize(argc,argv,false,mpi_comm);
    if (input_parameter_list.isParameter("PPfile"))
      {
	std::string PPfile = Teuchos::getParameter<std::string>(input_parameter_list, "PPfile");
	ParmParse::Initialize(argc,argv,PPfile.c_str());
      }

    // Determine whether we need to convert to input file to 
    //native structured format
    bool native = input_parameter_list.get<bool>("Native Structured Input",false);
    Teuchos::ParameterList converted_parameter_list;
    if (!native) 
      converted_parameter_list =
	Amanzi::AmanziInput::convert_to_structured(input_parameter_list);
    else
      converted_parameter_list = input_parameter_list;

    Teuchos::writeParameterListToXmlFile(converted_parameter_list,"tmpfile");

    BoxLib::Initialize_ParmParse(converted_parameter_list);

    const Real run_strt = ParallelDescriptor::second();

    int  max_step;
    Real strt_time;
    Real stop_time;

    ParmParse pp;

    ParmParse::dumpTable(std::cout,true);

    max_step  = -1;    
    strt_time =  0.0;  
    stop_time = -1.0;  

    pp.query("max_step",max_step);
    pp.query("strt_time",strt_time);
    pp.query("stop_time",stop_time);

    if (strt_time < 0.0)
        BoxLib::Abort("MUST SPECIFY a non-negative strt_time");

    if (max_step < 0 && stop_time < 0.0)
    {
        BoxLib::Abort(
            "Exiting because neither max_step nor stop_time is non-negative.");
    }


    Amr* amrptr = new Amr;

    amrptr->init(strt_time,stop_time);

    while ( amrptr->okToContinue()           &&
           (amrptr->levelSteps(0) < max_step || max_step < 0) &&
           (amrptr->cumTime() < stop_time || stop_time < 0.0) )
    {
        amrptr->coarseTimeStep(stop_time);
    }

    // Process the observations
    const PArray<Observation>& observation_array = PorousMedia::TheObservationArray();

    Structured_observations(observation_array,output_observations);

    delete amrptr;

    const int IOProc   = ParallelDescriptor::IOProcessorNumber();
    Real      run_stop = ParallelDescriptor::second() - run_strt;

    ParallelDescriptor::ReduceRealMax(run_stop,IOProc);

    if (ParallelDescriptor::IOProcessor())
      {
        std::cout << "Run time = " << run_stop << std::endl;
	std::cout << "SCOMPLETED\n";
      }
    BoxLib::Finalize(false); // Calling routine responsible for MPI_Finalize call

    return Amanzi::Simulator::SUCCESS;
}
