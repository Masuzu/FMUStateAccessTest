#include "Simulation.h"
#include "SimulationLog.h"
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <sstream>

#include "fmu20/src/shared/sim_support.h"

static std::string GetTmpPath()
{
	char tmpPath[MAX_PATH];
	if(! GetTempPath(MAX_PATH, tmpPath)) {
		printf ("error: Could not find temporary disk space\n");
		return NULL;
	}

	char* buffer;
	UUID guid;
	UuidCreate(&guid);
	if (UuidToString(&guid, (RPC_CSTR*)&buffer) == RPC_S_OK)
	{
		strcat(tmpPath, "fmu");
		strcat(tmpPath, buffer);
		strcat(tmpPath, "\\");
		RpcStringFree((RPC_CSTR*)&buffer);
	}

	return tmpPath;
}

static int DeleteDirectory(const std::string &refcstrRootDirectory, bool bDeleteSubdirectories)
{
	bool            bSubdirectory = false;       // Flag, indicating whether
	// subdirectories have been found
	HANDLE          hFile;                       // Handle to directory
	std::string     strFilePath;                 // Filepath
	std::string     strPattern;                  // Pattern
	WIN32_FIND_DATA FileInformation;             // File information


	strPattern = refcstrRootDirectory + "\\*.*";
	hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(FileInformation.cFileName[0] != '.')
			{
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

				if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(bDeleteSubdirectories)
					{
						// Delete subdirectory
						int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
						if(iRC)
							return iRC;
					}
					else
						bSubdirectory = true;
				}
				else
				{
					// Set file attributes
					if(::SetFileAttributes(strFilePath.c_str(),
						FILE_ATTRIBUTE_NORMAL) == FALSE)
						return ::GetLastError();

					// Delete file
					if(::DeleteFile(strFilePath.c_str()) == FALSE)
						return ::GetLastError();
				}
			}
		} while(::FindNextFile(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if(dwError != ERROR_NO_MORE_FILES)
			return dwError;
		else
		{
			if(!bSubdirectory)
			{
				// Set directory attributes
				if(::SetFileAttributes(refcstrRootDirectory.c_str(),
					FILE_ATTRIBUTE_NORMAL) == FALSE)
					return ::GetLastError();

				// Delete directory
				if(::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE)
					return ::GetLastError();
			}
		}
	}

	return 0;
}

fmi2Component Simulation::LoadFMU(const std::string &strFMUFileName, FMU &fmu, fmi2CallbackFunctions &fnCallbacks, std::string &strTempDir)
{
	strTempDir = GetTmpPath();

	loadFMU(strFMUFileName.c_str(), strTempDir.c_str(), fmu);

	std::string fmuResourceLocation = "file:///" + strTempDir + "resources//";
	fmi2Boolean visible = fmi2False;        // no simulator user interface

	fnCallbacks.logger = fmuLogger;
	fnCallbacks.allocateMemory = calloc;
	fnCallbacks.freeMemory = free;
	fnCallbacks.componentEnvironment = &fmu;
	fnCallbacks.stepFinished = NULL;
	fmi2Boolean toleranceDefined = fmi2False;  // true if model description define tolerance

	// instantiate the fmu
	ModelDescription *pModelDescription = fmu.modelDescription;
	// global unique id of the fmu
	const char *guid = getAttributeValue((Element *)pModelDescription, att_guid);
	const char *instanceName = getAttributeValue((Element *)getCoSimulation(pModelDescription), att_modelIdentifier);
	// instance of the fmu
#ifdef _DEBUG
	fmi2Component c = fmu.instantiate(instanceName, fmi2CoSimulation, guid, fmuResourceLocation.c_str(), &fnCallbacks, visible, false);
	//char *loggingCategories []= {"logStatusPending", "logStatusFatal", "logStatusError"};
	//fmu.setDebugLogging(m_fmi2Component, fmi2True, 3, loggingCategories);
#else
	fmi2Component c = fmu.instantiate(instanceName, fmi2CoSimulation, guid, fmuResourceLocation.c_str(), &fnCallbacks, visible, false);
#endif

	if (!c)
	{
		std::cerr << "could not instantiate model";
		ExitProcess(-1);
	}

	return c;
}

Simulation::Simulation(const std::string &strFMUInstanceName,const std::string &strFMUFileName, fmi2Real startTime, fmi2Real stopTime, fmi2Real tolerance)
	: m_pSimulationLog(nullptr),
	m_FMUState(nullptr)
{
	m_fmi2Component = LoadFMU(strFMUFileName, m_FMU, m_fnCallbacks, m_strTempDir);

	fmi2Boolean toleranceDefined = fmi2False;  // true if model description define tolerance

	Element *pDefaultExperiment = getDefaultExperiment(m_FMU.modelDescription);
	ValueStatus vs = valueMissing;
	if (pDefaultExperiment)
		getAttributeDouble(pDefaultExperiment, att_tolerance, &vs);
	if (vs == valueDefined)
		toleranceDefined = fmi2True;

	if (m_FMU.setupExperiment(m_fmi2Component, toleranceDefined, tolerance, startTime, fmi2True, stopTime) > fmi2Warning)
	{
		std::cerr << "could not initialize model; failed FMI setup experiment";
		ExitProcess(-1);
	}

	// Retrieve FMU capabilities
	Component *pCoSimulation = getCoSimulation(m_FMU.modelDescription);
	if(!pCoSimulation)
	{
		std::cerr << "Co-Simulation is not supported by the FMU " << strFMUFileName;
		ExitProcess(-1);
	}
}

Simulation::~Simulation(void)
{
	if(m_pSimulationLog)
		delete m_pSimulationLog;
	m_FMU.terminate(m_fmi2Component);
	m_FMU.freeInstance(m_fmi2Component);
	FreeLibrary(m_FMU.dllHandle);
	DeleteDirectory(m_strTempDir, true);
}

void Simulation::EnterInitializationMode()
{
	if (m_FMU.enterInitializationMode(m_fmi2Component) > fmi2Warning)
		std::cerr << "could not initialize model; failed FMI enter initialization mode";
}

void Simulation::ExitInitializationMode()
{
	if (m_FMU.exitInitializationMode(m_fmi2Component) > fmi2Warning)
		std::cerr << "could not initialize model; failed FMI exit initialization mode";
}

SimulationLog *Simulation::AttachLogFile(const std::string &logFileName)
{
	m_pSimulationLog = new SimulationLog(this, logFileName);
	return m_pSimulationLog;
}

void Simulation::ReadStepSizeFromFile(const std::string fileName)
{
	std::ifstream input(fileName);
	if(!input)
	{
		std::string error = "Could not open file " + fileName;
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
	for(std::string line; getline(input, line, '\n');)
		m_vStepSize.push_back(atof(line.c_str()));
}

double Simulation::GetStepSizeAt(unsigned int index)
{
	return m_vStepSize.at(index);
}

void Simulation::SaveState()
{
	m_FMU.getFMUstate(m_fmi2Component, &m_FMUState);
}

void Simulation::RestoreSavedState()
{
	m_FMU.setFMUstate(m_fmi2Component, m_FMUState);
}

std::string Simulation::GetVariableValueAsString(const std::string &strVariableName)
{
	auto var = getVariable(m_FMU.modelDescription, strVariableName.c_str());
	if(!var)
	{
		std::string error = "Variable " + strVariableName + " not found in the FMU.";
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
	fmi2ValueReference vr = getValueReference(var);
	std::ostringstream strs;
	switch (getElementType(getTypeSpec(var)))
	{
	case elm_Real:
		fmi2Real r;
		m_FMU.getReal(m_fmi2Component, &vr, 1, &r);
		strs << r;
		return strs.str();
		break;
	case elm_Integer:
	case elm_Enumeration:
		fmi2Integer i;
		m_FMU.getInteger(m_fmi2Component, &vr, 1, &i);
		strs << i;
		return strs.str();
		break;
	case elm_Boolean:
		fmi2Boolean b;
		m_FMU.getBoolean(m_fmi2Component, &vr, 1, &b);
		strs << b;
		return strs.str();
		break;
	case elm_String:
		fmi2String s;
		m_FMU.getString(m_fmi2Component, &vr, 1, &s);
		return s;
		break;
	}
}

void Simulation::PerformStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize)
{
	fmi2Status fmi2Flag = m_FMU.doStep(m_fmi2Component, currentCommunicationPoint, communicationStepSize, fmi2False);
	if (fmi2Flag == fmi2Discard)
	{
		fmi2Boolean b;
		// check if model requests to end simulation
		if (fmi2OK != m_FMU.getBooleanStatus(m_fmi2Component, fmi2Terminated, &b))
			std::cerr << "could not complete simulation of the model. getBooleanStatus return other than fmi2OK";
		if (b == fmi2True)
			std::cerr << "the model requested to end the simulation";

		std::cerr << "could not complete simulation of the model";
	}
	if (fmi2Flag != fmi2OK)
		std::cerr << "could not complete simulation of the model";
}

void Simulation::SetVariable(const std::string &strVariableName, bool value)
{
	auto var = getVariable(m_FMU.modelDescription, strVariableName.c_str());
	if(!var)
	{
		std::string error = "Variable " + strVariableName + " not found in the FMU.";
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
	fmi2Boolean b = value ? fmi2True : fmi2False;
	fmi2ValueReference vr = getValueReference(var);
	m_FMU.setBoolean(m_fmi2Component, &vr, 1, &b);
}

void Simulation::SetVariable(const std::string &strVariableName, fmi2Real value)
{
	auto var = getVariable(m_FMU.modelDescription, strVariableName.c_str());
	if(!var)
	{
		std::string error = "Variable " + strVariableName + " not found in the FMU.";
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
	fmi2ValueReference vr = getValueReference(var);
	m_FMU.setReal(m_fmi2Component, &vr, 1, &value);
}

void Simulation::SetVariable(const std::string &strVariableName, fmi2Integer value)
{
	auto var = getVariable(m_FMU.modelDescription, strVariableName.c_str());
	if(!var)
	{
		std::string error = "Variable " + strVariableName + " not found in the FMU.";
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
	fmi2ValueReference vr = getValueReference(var);
	m_FMU.setInteger(m_fmi2Component, &vr, 1, &value);
}

void Simulation::SetVariable(const std::string &strVariableName, fmi2String value)
{
	auto var = getVariable(m_FMU.modelDescription, strVariableName.c_str());
	if(!var)
	{
		std::string error = "Variable " + strVariableName + " not found in the FMU.";
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
	fmi2ValueReference vr = getValueReference(var);
	m_FMU.setString(m_fmi2Component, &vr, 1, &value);
}
