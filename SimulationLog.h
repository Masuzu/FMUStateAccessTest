#pragma once

#include <vector>
#include <string>

#pragma warning(disable: 4251)

class Simulation;
class SimulationLog
{
private:
	FILE* m_pFile;
	Simulation *m_pSimulation;
	std::vector<std::string> m_vNameOfVariableToLog;
	char m_cSeparator;

public:
	SimulationLog(Simulation *pSimulation, const std::string &strLogFileName, const char separator = ';');
	void LogVariable(const std::string &variableName);
	void WriteHeader();
	void RecordValues(double time);
	~SimulationLog(void);
};

#pragma warning(default: 4251)
