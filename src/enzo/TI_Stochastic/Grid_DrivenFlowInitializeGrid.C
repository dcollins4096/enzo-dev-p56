/***********************************************************************
/
/  GRID CLASS: DrivenFlowInitializeGrid
/
/  written by: Wolfram Schmidt
/  date:       May, 2005
/  modified1:  Sep, 2014: modified to support enzo 2.4 // P. Grete
/
/  PURPOSE: Initializes grid for a driven flow simulation
/
/  RETURNS: FAIL or SUCCESS
/
************************************************************************/

#include "preincludes.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "Fluxes.h"
#include "GridList.h"
#include "ExternalBoundary.h"
#include "Grid.h"

int GetUnits(float *DensityUnits, float *LengthUnits,
              float *TemperatureUnits, float *TimeUnits,
              float *VelocityUnits, FLOAT *MassUnits, FLOAT Time);

int grid::DrivenFlowInitializeGrid(float DrivenFlowDensity,
                   float DrivenFlowPressure, float DrivenFlowMagField, int SetBaryonFields)
{
  /* create fields */

  NumberOfBaryonFields = 0;
  FieldType[NumberOfBaryonFields++] = Density;

  int vel = NumberOfBaryonFields;

  FieldType[NumberOfBaryonFields++] = Velocity1;
  FieldType[NumberOfBaryonFields++] = Velocity2;
  FieldType[NumberOfBaryonFields++] = Velocity3;

  if (EquationOfState == 0)
    FieldType[NumberOfBaryonFields++] = TotalEnergy;

  if (DualEnergyFormalism)
      FieldType[NumberOfBaryonFields++] = InternalEnergy;

  if (UseMHD) {
    FieldType[NumberOfBaryonFields++] = Bfield1;
    FieldType[NumberOfBaryonFields++] = Bfield2;
    FieldType[NumberOfBaryonFields++] = Bfield3;
  }
  if (HydroMethod == MHD_RK){
    FieldType[NumberOfBaryonFields++] = PhiField;
    
    if(UseDivergenceCleaning)
      FieldType[NumberOfBaryonFields++] = Phi_pField;
  }

  int accel = NumberOfBaryonFields;

  FieldType[NumberOfBaryonFields++] = DrivingField1;
  FieldType[NumberOfBaryonFields++] = DrivingField2;
  FieldType[NumberOfBaryonFields++] = DrivingField3;


  /* Return if this doesn't concern us. */

  if (ProcessorNumber != MyProcessorNumber)
    return SUCCESS;

  if (!SetBaryonFields)
    return SUCCESS;

  float DensityUnits, LengthUnits, TemperatureUnits, TimeUnits,
        VelocityUnits;
  FLOAT MassUnits = 1;

  int size = 1;

  for (int dim = 0; dim < GridRank; dim++)
    size *= GridDimension[dim];


  this->AllocateGrids();

  /* set density, total energy and velocity in problem dimension */

  float Energy = DrivenFlowPressure / ((Gamma-1.0) * DrivenFlowDensity);

  for (int i = 0; i < size; i++) {
    BaryonField[iden][i] = DrivenFlowDensity;
    BaryonField[ietot][i] = Energy;
  }

  if (DualEnergyFormalism) ///CF (internal energy = total energy, because v=0)
    for (int i = 0; i < size; i++) {
      BaryonField[ieint][i] = Energy;
    }
  printf("DrivenFlowInitializeGrid %"FSYM" %"FSYM" %"FSYM" %"FSYM"\n",
    DrivenFlowDensity,DrivenFlowPressure,Gamma,Energy);
  
  if (HydroMethod == MHD_RK) {
      for (int i = 0; i < size; i++) {
          BaryonField[iBx  ][i]  = DrivenFlowMagField;
      }
      Energy += 0.5 * pow(DrivenFlowMagField,2) / DrivenFlowDensity;
  }
  
  if ( UseMHDCT ){
    for ( int i = 0; i < MagneticSize[0]; i++){
       MagneticField[0][i] = DrivenFlowMagField;
    }
    Energy += 0.5 * pow(DrivenFlowMagField,2) / DrivenFlowDensity;
  this->CenterMagneticField();
  }
  
  if (EquationOfState == 0)
    for( int i = 0; i < size; i++)
      BaryonField[ietot][i] = Energy;

  fprintf(stderr,"CLOWN bf %0.2e %0.2e\n", BaryonField[0][8*8*8], BaryonField[ietot][8*8*8]);
  return SUCCESS;
}
