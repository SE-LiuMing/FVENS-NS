/** \file ameshutils.hpp
 * \brief Some mesh-related functionality
 * \author Aditya Kashi
 * \date 2018-02
 */

#ifndef AMESHUTILS_H
#define AMESHUTILS_H

#include "amesh2dh.hpp"
#include "spatial/aspatial.hpp"

namespace fvens {

/// Returns a ready-to-use mesh object from the path to mesh file
UMesh2dh<a_real> constructMesh(const std::string mesh_path);

/// Computes various entity lists required for mesh traversal, also reorders the cells if requested
/** This can, and should, be called immediately after [reading](UMesh2dh::readMesh) the mesh.
 * Does not compute [periodic boundary maps](UMesh2dh::compute_periodic_map);
 * this must be done separately.
 */
template <typename scalar>
StatusCode preprocessMesh(UMesh2dh<scalar>& m);

/// Reorders the mesh cells in a given ordering using PETSc
/** Symmetric premutations only.
 * \warning It is the caller's responsibility to recompute things that are affected by the reordering,
 * such as \ref UMesh2dh::compute_topological.
 *
 * \param ordering The ordering to use - "rcm" is recommended. See the relevant
 * [page](www.mcs.anl.gov/petsc/petsc-current/docs/manualpages/Mat/MatOrderingType.html)
 * in the PETSc manual for the full list.
 * \param sd Spatial discretization to be used to generate a Jacobian matrix
 * \param m The mesh context
 */
template <typename scalar>
StatusCode reorderMesh(const char *const ordering, const Spatial<a_real,1>& sd, UMesh2dh<scalar>& m);

/// Divides mesh cells into levels within each of which no cell is coupled to another
/** Returns a list of cell indices corresponding to the start of each level.
 * The length of the list is one more than the number of levels.
 */
template <typename scalar>
std::vector<a_int> levelSchedule(const UMesh2dh<scalar>& m);

/// Compares two meshes for equality
/** \return An array of booleans which represents whether the following, in order, are the same:
 *  - number of elements
 *  - number of points
 *  - number of physical boundary faces
 *  - number of nodes per element for each element
 *  - number of faces per element for each element
 *  - element-point interconnectivity list
 *  - boundary faces' array along with boundary tags
 *  - coordinates of points
 */
std::array<bool,8> compareMeshes(const UMesh2dh<a_real>& m1, const UMesh2dh<a_real>& m2);

}
#endif
