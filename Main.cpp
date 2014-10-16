#define __TEST_DYMOLA_FMU_STATE_INCONSISTENCIES	1

#define __DELTA_T_FILE_PATH	"C:\\Users\\JPhT\\Documents\\Visual Studio 2012\\Projects\\DACCOSIM\\x64\\Release\\deltaT.txt"
#define __7_ZIP_FOLDER	"C:\\Program Files\\7-Zip"
#define __NETWORK_FMU_PATH	"C:\\Users\\JPhT\\Documents\\FMU\\Projet_0PWH_Pre_0FMUs_Reseau.fmu"
#define __PWH_F_FMU_PATH	"C:\\Users\\JPhT\\Documents\\FMU\\Projet_0PWH_Pre_0FMUs_PWH_0F.fmu"
#define __PWH_S_FMU_PATH	"C:\\Users\\JPhT\\Documents\\FMU\\Projet_0PWH_Pre_0FMUs_PWH_0S.fmu"

#include "Simulation.h"
#include "SimulationLog.h"
#include <thread>
#include <iostream>
#include <omp.h>

Simulation *pSimulationNetwork;
SimulationLog *logFile;
Simulation *pSimulationPWH_F;
Simulation *pSimulationPWH_S;

HANDLE ghSemaphoreCalculationStepComplete;
HANDLE ghSemaphorePWH_FCalculationStepComplete;
HANDLE ghSemaphorePWH_SCalculationStepComplete;
unsigned int CalculationStepCompletionCount = 0;

// Shared variables for inter-FMU communications
fmi2Boolean S_AVT_1;
fmi2Boolean S_AVT_2;
fmi2Real Vr_t1_1;
fmi2Real Vr_t1_2;
fmi2Real Ir_t1_1;
fmi2Real Ir_t1_2;

void WaitCalculationStepCompletion()
{
	#pragma omp critical
	{
		++CalculationStepCompletionCount;
		if(CalculationStepCompletionCount == 3)
		{
			CalculationStepCompletionCount = 0;
			ReleaseSemaphore( 
				ghSemaphoreCalculationStepComplete,  // handle to semaphore
				1,            // increase count by one
				NULL);       // not interested in previous count
			
			ReleaseSemaphore( 
				ghSemaphorePWH_FCalculationStepComplete,  // handle to semaphore
				1,            // increase count by one
				NULL);       // not interested in previous count

			ReleaseSemaphore( 
				ghSemaphorePWH_SCalculationStepComplete,  // handle to semaphore
				1,            // increase count by one
				NULL);       // not interested in previous count
		}
	}
}

void SimulateNetwork()
{
	UINT iteration = 0;
	for(double dTime = 0; iteration<pSimulationNetwork->GetNumStepSizes();)
	{
		double dStepSize = pSimulationNetwork->GetStepSizeAt(iteration);

#ifdef __TEST_DYMOLA_FMU_STATE_INCONSISTENCIES
		pSimulationNetwork->SaveState();
		pSimulationNetwork->SetVariable("S_AVT_1", S_AVT_1);
		pSimulationNetwork->SetVariable("S_AVT_2", S_AVT_2);
		pSimulationNetwork->PerformStep(dTime, dStepSize);
		pSimulationNetwork->RestoreSavedState();
#endif
		pSimulationNetwork->SetVariable("S_AVT_1", S_AVT_1);
		pSimulationNetwork->SetVariable("S_AVT_2", S_AVT_2);
		pSimulationNetwork->PerformStep(dTime, dStepSize);

		Vr_t1_1 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Vr_t1_1");
		Vr_t1_2 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Vr_t1_2");
		Ir_t1_1 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Ir_t1_1");
		Ir_t1_2 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Ir_t1_2");

		dTime+=dStepSize;
		logFile->RecordValues(dTime);
		++iteration;

		WaitCalculationStepCompletion();
		WaitForSingleObject( 
            ghSemaphoreCalculationStepComplete,   // handle to semaphore
            INFINITE);           // zero-second time-out interval
	}
	std::cout << "SimulateNetwork end" << std::endl;
}

void SimulatePWH_F()
{
	UINT iteration = 0;
	for(double dTime = 0; iteration<pSimulationNetwork->GetNumStepSizes();)
	{
		double dStepSize = pSimulationNetwork->GetStepSizeAt(iteration);

#ifdef __TEST_DYMOLA_FMU_STATE_INCONSISTENCIES
		pSimulationPWH_F->SaveState();
		pSimulationPWH_F->SetVariable("Vr_t1", Vr_t1_1);
		pSimulationPWH_F->SetVariable("Ir_t1", Ir_t1_1);
		pSimulationPWH_F->PerformStep(dTime, dStepSize);
		pSimulationPWH_F->RestoreSavedState();
#endif
		pSimulationPWH_F->SetVariable("Vr_t1", Vr_t1_1);
		pSimulationPWH_F->SetVariable("Ir_t1", Ir_t1_1);

		pSimulationPWH_F->PerformStep(dTime, dStepSize);

		S_AVT_1 = pSimulationPWH_F->GetVariableValueAs<fmi2Boolean>("S_AVT");

		dTime+=dStepSize;
		++iteration;

		WaitCalculationStepCompletion();
		WaitForSingleObject( 
            ghSemaphorePWH_FCalculationStepComplete,   // handle to semaphore
            INFINITE);           // zero-second time-out interval
	}
	std::cout << "SimulatePWH_F end" << std::endl;
}

void SimulatePWH_S()
{
	UINT iteration = 0;
	for(double dTime = 0; iteration<pSimulationNetwork->GetNumStepSizes();)
	{
		double dStepSize = pSimulationNetwork->GetStepSizeAt(iteration);	

#ifdef __TEST_DYMOLA_FMU_STATE_INCONSISTENCIES
		pSimulationPWH_S->SaveState();
		pSimulationPWH_S->SetVariable("Vr_t1", Vr_t1_2);
		pSimulationPWH_S->SetVariable("Ir_t1", Ir_t1_2);
		pSimulationPWH_S->PerformStep(dTime, dStepSize);
		pSimulationPWH_S->RestoreSavedState();
#endif
		pSimulationPWH_S->SetVariable("Vr_t1", Vr_t1_2);
		pSimulationPWH_S->SetVariable("Ir_t1", Ir_t1_2);
		pSimulationPWH_S->PerformStep(dTime, dStepSize);

		S_AVT_2 = pSimulationPWH_S->GetVariableValueAs<fmi2Boolean>("S_AVT");

		dTime+=dStepSize;
		++iteration;

		WaitCalculationStepCompletion();
		WaitForSingleObject( 
            ghSemaphorePWH_SCalculationStepComplete,   // handle to semaphore
            INFINITE);           // zero-second time-out interval
	}
	std::cout << "SimulatePWH_S end" << std::endl;
}

int main()
{
#pragma region Synchronization
	ghSemaphoreCalculationStepComplete = CreateSemaphore( 
        NULL,           // default security attributes
        0,  // initial count
        1,  // maximum count
        NULL);          // unnamed semaphore

	ghSemaphorePWH_FCalculationStepComplete = CreateSemaphore( 
        NULL,           // default security attributes
        0,  // initial count
        1,  // maximum count
        NULL);          // unnamed semaphore

	ghSemaphorePWH_SCalculationStepComplete = CreateSemaphore( 
        NULL,           // default security attributes
        0,  // initial count
        1,  // maximum count
        NULL);          // unnamed semaphore
#pragma endregion Synchronization

	SetEnvironmentVariableA("FMUSDK_HOME", __7_ZIP_FOLDER);

#pragma region FMU_Initialization
	double dStartTime = 0.0;
	double dStopTime = 2.0;

	pSimulationNetwork = new Simulation("Network", __NETWORK_FMU_PATH, dStartTime, dStopTime, 1E-7);
	pSimulationNetwork->EnterInitializationMode();
	pSimulationNetwork->ExitInitializationMode();
	pSimulationNetwork->ReadStepSizeFromFile(__DELTA_T_FILE_PATH);

	logFile = pSimulationNetwork->AttachLogFile("C:\\Users\\JPhT\\Documents\\FMU\\NetworkCPP.csv");
	logFile->LogVariable("Vr_t1_1");
	logFile->LogVariable("Ir_t1_1");
	logFile->LogVariable("S_AVT_1");
	logFile->LogVariable("S_AVT_2");

	pSimulationPWH_F = new Simulation("PWH_F", __PWH_F_FMU_PATH, dStartTime, dStopTime, 1E-7);
	pSimulationPWH_F->EnterInitializationMode();
	pSimulationPWH_F->ExitInitializationMode();
	
	pSimulationPWH_S = new Simulation("PWH_S", __PWH_S_FMU_PATH, dStartTime, dStopTime, 1E-7);
	pSimulationPWH_S->EnterInitializationMode();
	pSimulationPWH_S->ExitInitializationMode();
	
	Vr_t1_1 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Vr_t1_1");
	Vr_t1_2 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Vr_t1_2");
	Ir_t1_1 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Ir_t1_1");
	Ir_t1_2 = pSimulationNetwork->GetVariableValueAs<fmi2Real>("Ir_t1_2");
	S_AVT_1 = pSimulationPWH_F->GetVariableValueAs<fmi2Boolean>("S_AVT");
	S_AVT_2 = pSimulationPWH_S->GetVariableValueAs<fmi2Boolean>("S_AVT");

	logFile->WriteHeader();
	logFile->RecordValues(0.0);
#pragma endregion FMU_Initialization

#pragma region CoSimulation
	std::thread networkThread(SimulateNetwork);
	std::thread PWH_FThread(SimulatePWH_F);
	std::thread PWH_SThread(SimulatePWH_S);

	networkThread.join();
	PWH_FThread.join();
	PWH_SThread.join();
#pragma endregion CoSimulation

	delete pSimulationNetwork;
	delete pSimulationPWH_F;
	delete pSimulationPWH_S;

	system("pause");
	return 0;
}