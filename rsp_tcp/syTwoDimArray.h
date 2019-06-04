//matrix------------------------------------------------------------------------
//
//      Subsystem     : None, general purpose class
//      Classes       : matrix
//      Purpose       : Twodimensional dynamical array
//
//      Author        : Clemens Schmidt, adapted from "STL konzentriert"
//------------------------------------------------------------------------------
// Ver. |Release| Auth| Change                                | Remarks
//------------------------------------------------------------------------------
// 1.0  |19Nov00| CS  |                                       |
// 1.1  |06Jun02| CS  |  private inheritance,operator[]       |
// 1.2  |06Jun02| CS  |  typename                             |
// 1.2  |28Oct05| CS  |  setSize                              |
//------------------------------------------------------------------------------
//  (c)softsyst GmbH, Tel: +49-7551-932 855, www.softsyst.com
//------------------------------------------------------------------------------
//
//////////////////////////////////////////////////////////////////////
// matrix
#ifndef matrix_t
#define matrix_t
#pragma warning (disable:4786)
#include <iterator>
#include <vector>
using namespace std;
// matrix als Vektor von Vektoren

template<class T>
class matrix : private vector<vector<T> >
{
   typedef typename vector<T>::size_type size_type ;
               
   public:
     matrix(size_type x = 0, size_type y = 0)
     : vector<vector<T> >(x,vector<T>(y)){}
	 /////////////////////////
     size_type rows() const {return size(); }
     size_type columns() const 
	 {
		 if (size() > 0)
			 return at(0).size();
		 else
			 return 0;
	 }
	 /////////////////////////
	 vector<T>& operator[] (const size_type& ix){return at(ix);}
	 /////////////////////////
	 void setSize (const size_type rows, const size_type cols, const T& value)
	 {
	    clear_all();		//this is mandatory in case only cols change!!!! Otherwise no action is taken
							//because the resize(rows, colvect) is a NOP if rows did not change.
		vector<T> colvect;
		colvect.resize(cols, value);
		resize (rows, colvect);
	 }

	 /////////////////////////
	 void clear_all()
	 {
        for (size_type i = 0; i < size(); i++)
			at(i).clear();
		clear();		 
	 }
	 

	 /////////////////////////
     void init(const T& Wert)
     {
        for (size_type i = 0; i < rows(); i++)
           for (size_type j = 0; j < columns() ; j++)
			   at(i)[j] = Wert;
               //operator[](i)[j] = Wert;
               //(operator[](i)).operator[](j) = Wert;
     }
	 /////////////////////////
     void insertRow(const int row, const int columns, const T& value)
     {
		vector<T> colvect;
		colvect.resize(columns, value);
		iterator it = begin();
		advance(it, row);
		insert(it, colvect);
     }
	 /////////////////////////
     void removeRow(const int row)
     {
		if (row < 0 || row > rows())
			return;
		iterator it = begin();
		advance(it, row);
		erase (it);
     }
	//////////////////////////////////////////////////////////////////////
	//order < 0 : row1 precedes row2
	//order > 0 : row2 precedes row1
	//order == 0: no change
	void reorderRows(int row1, int row2, int order)
	{
		if (row1 < 0 || row2 < 0 || row1 >= rows() || row2 >= rows())
			return;
		if (order == 0)
			return;

		vector<T> colvect;
		int first, second;
		if (order < 0)
		{
			first = row1;
			second = row2;
		}
		else
		{
			first = row2;
			second = row1;
		}

		//test
		T t;
		for (int i=0; i < rows(); i++)
			t = at(i)[3];

		iterator it, it2;
		it = begin();
		advance(it, first);
		colvect = at(first); //this colvect has to be inserted before "second"

		if (second > first)
			second--;
		erase (it);
		it2 = begin();
		advance (it2, second);
		insert(it2, colvect);

		for ( i=0; i < rows(); i++)
			t = at(i)[3];

	}
};

template<class T>
ostream& operator<<(ostream& s, const matrix<T>& m )
{
    typedef vector<T>::size_type size_type;
    for (size_type i = 0; i < m.rows(); i++)
    { s << endl << i <<" :  ";
      for (size_type j = 0; j < m.columns(); j++)
           s << m[i][j] <<" ";
    }
    s <<endl;
    return s;
}

#endif


