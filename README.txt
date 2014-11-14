Showcase of a bug with Dymola 2015 FMUs (FMUs are black boxes representing a model and which embed solvers). The program on this repository performs the following actions:
1. Start simulation at time t0
2. Save FMU state
3. Simulate until t1
4. Read the results and store them in R1
5. Go back to t0 by restoring the FMU state saved at 2
6. Simulate until t1
7. Read the results and store them in R2

R1 is expected to be equal to R2. Yet, it is not the case!

Requires VC++ 2012 for compilation.
Open FMUStateAccessTest.sln and press F7 for compilation.
Press F5 to run the tests.
Comment or uncomment the line 1 in Main.cpp (#define __TEST_DYMOLA_FMU_STATE_INCONSISTENCIES	1) in order toggle the activation of rollbacks.
Please make sure you change the FMU file path depending on your configuration
See:
#define __7_ZIP_FOLDER	"C:\\Program Files\\7-Zip"
#define __NETWORK_FMU_PATH	"C:\\Users\\JPhT\\Documents\\FMU\\Projet_0PWH_Pre_0FMUs_Reseau.fmu"
