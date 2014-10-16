#pragma once

#include <vector>
#include <string>
#include "fmu20/src/shared/fmi2.h"

#pragma warning(disable: 4251)

class SimulationLog;
class Simulation
{
	friend class SimulationLog;

private:
	/// <summary>Current communication point</summary>
	fmi2Real m_dStepSize;

	/// <summary>Path to the temp directory where the FMU is extracted</summary>
	std::string m_strTempDir;

	FMU m_FMU;
	fmi2Component m_fmi2Component;
	fmi2CallbackFunctions m_fnCallbacks;

	SimulationLog *m_pSimulationLog;

	/// <summary>
	/// <para>Stores the step size for each consecutive iteration when using a step size provided in an external file.</para>
	/// See <see cref="ReadStepSizeFromFile"/>.
	/// </summary>
	std::vector<double> m_vStepSize;

	fmi2FMUstate m_FMUState;

public:
	static fmi2Component LoadFMU(const std::string &strFMUFileName, FMU &fmu, fmi2CallbackFunctions &fnCallbacks, std::string &strTempDir);

	Simulation(const std::string &strFMUInstanceName, const std::string &strFMUFileName, fmi2Real startTime, fmi2Real stopTime, fmi2Real tolerance = 1E-6);
	~Simulation(void);
	void PerformStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize);
	void EnterInitializationMode();
	void ExitInitializationMode();

	SimulationLog *AttachLogFile(const std::string &logFileName);

	/// <summary>
	/// Use a custom step size read from <i>fileName</i>.
	/// </summary>
	void ReadStepSizeFromFile(const std::string fileName);

	/// <summary>
	/// Retrieve m_vStepSize[index]
	/// </summary>
	double GetStepSizeAt(unsigned int index);
	__forceinline unsigned int GetNumStepSizes()	{return m_vStepSize.size();}

	void SaveState();
	void RestoreSavedState();

	std::string GetVariableValueAsString(const std::string &strVariableName);

	template <typename T>
	T GetVariableValueAs(const std::string &strVariableName)
	{
		auto var = getVariable(m_FMU.modelDescription, strVariableName.c_str());
		if(!var)
		{
			std::string error = "Variable " + strVariableName + " not found in the FMU.";
			MessageBoxA(NULL, error.c_str(), "Error", MB_OK | MB_ICONERROR);
			ExitProcess(-1);
		}
		fmi2ValueReference vr = getValueReference(var);
		void *ret = nullptr;
		switch (getElementType(getTypeSpec(var)))
		{
		case elm_Real:
			fmi2Real r;	
			m_FMU.getReal(m_fmi2Component, &vr, 1, &r);
			ret = &r;
			break;
		case elm_Integer:
		case elm_Enumeration:
			fmi2Integer i;
			m_FMU.getInteger(m_fmi2Component, &vr, 1, &i);
			ret = &i;
			break;
		case elm_Boolean:
			fmi2Boolean b;
			m_FMU.getBoolean(m_fmi2Component, &vr, 1, &b);
			ret = &b;
			break;
		case elm_String:
			fmi2String s;
			m_FMU.getString(m_fmi2Component, &vr, 1, &s);
			ret = &s;
			break;
		}

		return *((T*)(ret));
	}

	void SetVariable(const std::string &strVariableName, bool value);
	void SetVariable(const std::string &strVariableName, fmi2Real value);
	void SetVariable(const std::string &strVariableName, fmi2Integer value);
	void SetVariable(const std::string &strVariableName, fmi2String value);

	__forceinline const FMU &GetFMU()	{return m_FMU;}
	__forceinline fmi2Component Getfmi2Component()	{return m_fmi2Component;}
};

#pragma warning(default: 4251)