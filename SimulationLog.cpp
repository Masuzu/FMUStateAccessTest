#include "SimulationLog.h"
#include "Simulation.h"
#include <algorithm>
#include <fstream>

SimulationLog::SimulationLog(Simulation *pSimulation, const std::string &strLogFileName, const char separator)
	: m_pSimulation(pSimulation),
	m_cSeparator(separator)
{
	if (!(m_pFile = fopen(strLogFileName.c_str(), "w")))
	{
		std::string error = "Could not create file " + strLogFileName;
		MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
		ExitProcess(-1);
	}
}

SimulationLog::~SimulationLog(void)
{
	fclose(m_pFile);
}

void SimulationLog::LogVariable(const std::string &variableName)
{
	m_vNameOfVariableToLog.push_back(variableName);
}

void SimulationLog::WriteHeader()
{
	fprintf(m_pFile, "time");
	for(auto s : m_vNameOfVariableToLog)
		fprintf(m_pFile, "%c%s", m_cSeparator, s.c_str());
	fprintf(m_pFile, "\n");
}

void SimulationLog::RecordValues(double time)
{
	if(m_cSeparator == ';')
	{
		char acTime[256];
		sprintf(acTime, "%.16g", time);
		std::string strTime = acTime;
		std::replace(strTime.begin(), strTime.end(), '.', ',');
		fprintf(m_pFile, "%s", strTime.c_str());
	}
	else
		fprintf(m_pFile, "%.16g", time);
	for(auto s : m_vNameOfVariableToLog)
	{
		auto value = m_pSimulation->GetVariableValueAsString(s);
		if(m_cSeparator == ';')
			std::replace(value.begin(), value.end(), '.', ',');
		fprintf(m_pFile, "%c%s", m_cSeparator, value.c_str());
	}
	fprintf(m_pFile, "\n");
}