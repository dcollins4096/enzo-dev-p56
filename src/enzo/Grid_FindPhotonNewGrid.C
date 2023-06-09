#define DEBUG 0
/***********************************************************************
/
/  GRID CLASS (CHECKS IF PHOTON HAS LEFT THE GRID)
/
/  written by: John Wise
/  date:       October, 2009
/  modified1:
/
/  PURPOSE: 
/
/  RETURNS: TRUE  = photon belongs here
/           FALSE = photon should be moved
/
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ErrorExceptions.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "ExternalBoundary.h"
#include "Fluxes.h"
#include "GridList.h"
#include "Grid.h"

int grid::FindPhotonNewGrid(int cindex, FLOAT *r, FLOAT *u, int *g,
			    PhotonPackageEntry* &PP,
			    grid* &MoveToGrid, int &DeltaLevel,
			    const float *DomainWidth, int &DeleteMe,
			    grid *ParentGrid)
{

  int Refinement;
  int dim, RayInsideGrid;
  bool InsideDomain;

  /* First determine whether the ray has left the grid, store the
     destination grid, and "nudge" the photon package to avoid
     loitering on boundaries. */

  PP->Radius += PFLOAT_EPSILON;
  for (dim = 0; dim < MAX_DIMENSION; dim++) {
    r[dim] += u[dim] * PFLOAT_EPSILON;
    g[dim] = GridStartIndex[dim] + 
      nint(floor((r[dim] - GridLeftEdge[dim]) / CellWidth[dim][0]));
  }

  cindex = GRIDINDEX_NOGHOST(g[0], g[1], g[2]);
  RayInsideGrid = this->PointInGridNB(r);
  MoveToGrid = SubgridMarker[cindex];

  /***** Root grids *****/

  if (ParentGrid == NULL) {

    if (RayInsideGrid) {
      // Inside root grid -> Child grid
      DeltaLevel = +1;
    } else {
      // Outside root grid -> Other root grid or outside domain
      DeltaLevel = 0;
      for (dim = 0, InsideDomain = true; dim < MAX_DIMENSION; dim++)
	InsideDomain &= (r[dim] >= DomainLeftEdge[dim] && 
			 r[dim] <= DomainRightEdge[dim]);
      if (!InsideDomain) {
	if (RadiativeTransferPeriodicBoundary) {
	  for (dim = 0; dim < 3; dim++)
	    if (r[dim] < DomainLeftEdge[dim]) {
 	      PP->SourcePosition[dim] += DomainWidth[dim];
	    } else if (r[dim] > DomainRightEdge[dim]) {
 	      PP->SourcePosition[dim] -= DomainWidth[dim];
	    }
	} // ENDIF periodic
	else {
	  MoveToGrid = NULL;
	  DeleteMe = TRUE;
	} // ENDELSE periodic
	
      } // ENDIF !InsideDomain
    } // ENDELSE inside grid

  } // ENDIF parent grid

  /***** Subgrids *****/

  else {
      
    if (RayInsideGrid) {
      DeltaLevel = +1;
    } else {
      // Outside the grid, we have to determine whether it's a parent
      // or some other grid
      if (MoveToGrid == ParentGrid)
	DeltaLevel = -1;
      else if (MoveToGrid != NULL) {
	Refinement = nint( MoveToGrid->CellWidth[0][0] /
			   CellWidth[0][0] );
	DeltaLevel = 0;
	while (Refinement > 1) {
	  Refinement /= RefineBy;
	  DeltaLevel--;
	}
      } else if (RadiativeTransferPeriodicBoundary == FALSE) {
	DeleteMe = TRUE; // photon has left the domain
      }

      for (dim = 0, InsideDomain = true; dim < MAX_DIMENSION; dim++)
	InsideDomain &= (r[dim] >= DomainLeftEdge[dim] && 
			 r[dim] <= DomainRightEdge[dim]);
      if (!InsideDomain) {
	if (RadiativeTransferPeriodicBoundary) {
	  for (dim = 0; dim < 3; dim++)
	    if (r[dim] < DomainLeftEdge[dim]) {
	      PP->SourcePosition[dim] += DomainWidth[dim];
	    } else if (r[dim] > DomainRightEdge[dim]) {
	      PP->SourcePosition[dim] -= DomainWidth[dim];
	    }
	} // ENDIF periodic
      } // ENDELSE !InsideDomain
    } // ENDELSE

  } // ENDELSE (subgrid)

  /* Error check */

  if (DEBUG && MoveToGrid != NULL) {
    fprintf(stdout, "Walk: left grid: sent photon to grid %d [dx=%g] (DeltaL = %d)\n", 
	    MoveToGrid->ID, MoveToGrid->CellWidth[0][0], DeltaLevel);
  }


#ifdef UNUSED
  if (MoveToGrid != NULL && InsideDomain)
    if (MoveToGrid->PointInGridNB(r) == FALSE) {
      printf("Grid %d, MoveGrid %d\n", this->ID, MoveToGrid->ID);
      printf("Cell = %6d %6d %6d\n", g[0], g[1], g[2]);
      printf("Position = %15.12f %15.12f %15.12f\n", r[0], r[1], r[2]);
      printf("Source = %15.12f %15.12f %15.12f\n", (PP)->SourcePosition[0],
	     (PP)->SourcePosition[1], (PP)->SourcePosition[2]);
      printf("Direction = %15.12f %15.12f %15.12f\n", u[0], u[1], u[2]);
      (PP)->PrintInfo();
      printf("GridLeft = %15.12f %15.12f %15.12f\n", GridLeftEdge[0], GridLeftEdge[1], GridLeftEdge[2]);
      printf("GridRight = %15.12f %15.12f %15.12f\n", GridRightEdge[0], GridRightEdge[1], GridRightEdge[2]);
      printf("MoveGridLeft = %15.12f %15.12f %15.12f\n", MoveToGrid->GridLeftEdge[0], MoveToGrid->GridLeftEdge[1], MoveToGrid->GridLeftEdge[2]);
      printf("MoveGridRight = %15.12f %15.12f %15.12f\n", MoveToGrid->GridRightEdge[0], MoveToGrid->GridRightEdge[1], MoveToGrid->GridRightEdge[2]);
      ENZO_FAIL("Photon not contained in MoveToGrid!");
    }
#endif

  return SUCCESS;

}
