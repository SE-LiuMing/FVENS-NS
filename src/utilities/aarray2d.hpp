/**
 * @file aarray2d.hpp
 * @brief Defines a class to manipulate 2d arrays.
 * 
 * Part of FVENS.
 * @author Aditya Kashi
 * @date Feb 10, 2015
 *
 * 2016-04-17: Removed variable storage-order. Everything is row-major now.
 * Further, all indexing is now by a_int rather than int.
 */

/**
 * \namespace amat
 * \brief Includes matrix and some linear algebra classes.
 */

#ifndef AARRAY2D_H
#define AARRAY2D_H

#include <cassert>
#include "aconstants.hpp"

#ifndef MATRIX_DOUBLE_PRECISION
#define MATRIX_DOUBLE_PRECISION 14
#endif

namespace amat {
	
/// Real type
using acfd::a_real;

/// Integer type
using acfd::a_int;

const int WIDTH = 10;		// width of field for printing matrices

/**
 * \class Array2d
 * \brief Stores a dense 2D row-major array.
 * \deprecated Use std::vector instead, if possible.
 */
template <class T>
class Array2d
{
protected:
	a_int nrows;           ///< Number of rows
	a_int ncols;           ///< Number of columns
	a_int size;            ///< Total number of entries
	T* elems;              ///< Raw array of entries

public:
	/// No-arg constructor. Note: no memory allocation!
	Array2d() : nrows{0}, ncols{0}, size{0}, elems{nullptr}
	{ }

	/// Allocate some storage
	Array2d(const a_int nr, const a_int nc)
	{
		assert(nc>0);
		assert(nr>0);
		
		nrows = nr; ncols = nc;
		size = nrows*ncols;
		elems = new T[nrows*ncols];
	}

	/// Deep copy
	Array2d(const Array2d<T>& other)
		: nrows{other.nrows}, ncols{other.ncols}, size{other.size},
		elems{new T[nrows*ncols]}
	{
		for(a_int i = 0; i < nrows*ncols; i++)
		{
			elems[i] = other.elems[i];
		}
	}

	~Array2d()
	{
		delete [] elems;
	}

	/// Deep copy
	Array2d<T>& operator=(const Array2d<T>& rhs)
	{
#ifdef DEBUG
		if(this==&rhs) return *this;		// check for self-assignment
#endif
		nrows = rhs.nrows;
		ncols = rhs.ncols;
		size = nrows*ncols;
		delete [] elems;
		elems = new T[nrows*ncols];
		for(a_int i = 0; i < nrows*ncols; i++)
		{
			elems[i] = rhs.elems[i];
		}
		return *this;
	}
	
	/// Sets a new size for the array, deletes the contents and allocates new memory
	void resize(const a_int nr, const a_int nc)
	{
		assert(nc>0);
		assert(nr>0);
		
		nrows = nr; ncols = nc;
		size = nrows*ncols;
		delete [] elems;
		elems = new T[nrows*ncols];
	}

	/// Setup without deleting earlier allocation: use in case of Array2d<t>* (pointer to Array2d<t>)
	void setupraw(const a_int nr, const a_int nc);
	
	/// Fill the array with zeros.
	void zeros()
	{
		for(a_int i = 0; i < size; i++)
			elems[i] = (T)(0.0);
	}

	/// Fill the array with ones
	void ones();

	/// function to set matrix elements from a ROW-MAJOR array
	void setdata(const T* A, a_int sz);

	T get(const a_int i, const a_int j=0) const
	{
		assert(i < nrows);
		assert(j < ncols);
		assert(i>=0 && j>=0);
		return elems[i*ncols + j];
	}

	void set(a_int i, a_int j, T data)
	{
		assert(i < nrows);
		assert(j < ncols);
		assert(i>=0 && j>=0);
		elems[i*ncols + j] = data;
	}

	a_int rows() const { return nrows; }
	a_int cols() const { return ncols; }
	a_int msize() const { return size; }

	/// Getter/setter function for expressions like A(1,2) = 141 to set the element at 1st row and 2nd column to 141
	T& operator()(const a_int x, const a_int y=0)
	{
		assert(x < nrows);
		assert(y < ncols);
		assert(x >= 0 && y >= 0);
		return elems[x*ncols + y];
	}
	
	/// Const Getter/setter function for expressions like x = A(1,2) to get the element at 1st row and 2nd column
	const T& operator()(const a_int x, const a_int y=0) const
	{
		assert(x < nrows);
		assert(x >= 0 && y >= 0);
		return elems[x*ncols + y];
	}

	/// Returns a pointer-to-const to the beginning of a row
	const T* const_row_pointer(const a_int r) const
	{
		assert(r < nrows);
		return &elems[r*ncols];
	}
	
	/// Returns a pointer to the beginning of a row
	T* row_pointer(const a_int r)
	{
		assert(r < nrows);
		return &elems[r*ncols];
	}
	
	/// Prints the matrix to standard output.
	void mprint() const;

	/// Prints the matrix to file
	void fprint(std::ofstream& outfile) const;

	/// Reads matrix from file
	void fread(std::ifstream& infile);
	
	/// Separate setup function in case no-arg constructor has to be used
	/** \deprecated Please use resize() instead.
	 */
	void setup(const a_int nr, const a_int nc);
};


} //end namespace amat

#endif
