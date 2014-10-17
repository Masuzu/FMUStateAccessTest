#define __TEST_DYMOLA_FMU_STATE_INCONSISTENCIES	1

#define __7_ZIP_FOLDER	"C:\\Program Files\\7-Zip"
#define __NETWORK_FMU_PATH	"C:\\Users\\JPhT\\Documents\\FMU\\Projet_0PWH_Pre_0FMUs_Reseau.fmu"
#define __STEP_SIZE	2E-4
#define __NUMBER_OF_ITERATIONS	5

#include "Simulation.h"
#include <iostream>

int main()
{
	SetEnvironmentVariableA("FMUSDK_HOME", __7_ZIP_FOLDER);

	double dStartTime = 0.0;
	double dStopTime = 2.0;
	double dTime = dStartTime;
	Simulation *pSimulationNetwork = new Simulation("Network", __NETWORK_FMU_PATH, dStartTime, dStopTime, 1E-7);
	pSimulationNetwork->EnterInitializationMode();
	pSimulationNetwork->ExitInitializationMode();
	
	for(UINT iteration = 0; iteration < __NUMBER_OF_ITERATIONS; ++iteration)
	{
#ifdef __TEST_DYMOLA_FMU_STATE_INCONSISTENCIES
		pSimulationNetwork->SaveState();		// Call to getFMUState
		pSimulationNetwork->PerformStep(dTime, __STEP_SIZE);		// Call to doStep
		pSimulationNetwork->RestoreSavedState();		// Call to setFMUState
#endif
		pSimulationNetwork->PerformStep(dTime, __STEP_SIZE);
		
		// Calls to getReal
		fmi2Real Vr_t1_1 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Vr_t1_1");
		fmi2Real Vr_t1_2 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Vr_t1_2");
		fmi2Real Ir_t1_1 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Ir_t1_1");
		fmi2Real Ir_t1_2 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Ir_t1_2");

		dTime += __STEP_SIZE;
		std::cout << "Communication point = " << dTime << std::endl;
		std::cout << "Vr_t1_1 = " << Vr_t1_1 << std::endl;
		std::cout << "Vr_t1_2 = " << Vr_t1_2 << std::endl;
		std::cout << "Ir_t1_1 = " << Ir_t1_1 << std::endl;
		std::cout << "Ir_t1_2 = " << Ir_t1_2 << std::endl;
		std::cout << std::endl;
	}

	delete pSimulationNetwork;

	system("pause");
	return 0;
}