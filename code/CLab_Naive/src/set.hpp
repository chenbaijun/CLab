//////////////////////////////////////////////////////////
// File  : set.hpp   
// Desc. : implements a polymorph type set of mathematical sets
//         See e.g. "Using the STL, The C++ Standard Template 
//         Library" by Robert Robson, sec. edit., Springer, 1999
//         for an in depth discussion of these algorithms.
//         Obs: this header contains code, since templates
//         must be instantiated when defined.
// Author: Rune M. Jensen
// Date  : 2/11/00
//////////////////////////////////////////////////////////

#ifndef SETHPP
#define SETHPP

#include <set>
#include <algorithm>
#include <iostream>
#include <typeinfo>
using namespace std;


// set member
template<class T> bool setMember(const set<T>& s,const T& e) {
  return ( s.find(e) != s.end() );
}


// setunion
template<class T> set<T> setUnion(const set<T>& s1, const set<T>& s2) {
  set<T> res;
  insert_iterator< set<T> > res_ins(res, res.begin());
  set_union(s1.begin(),s1.end(),s2.begin(),s2.end(),res_ins);
  return res;
}

// set intersection
template<class T> set<T> setIntersection(const set<T>& s1, const set<T>& s2) {
  set<T> res;
  insert_iterator< set<T> > res_ins(res, res.begin());
  set_intersection(s1.begin(),s1.end(),s2.begin(),s2.end(),res_ins);
  return res;
}      

// set difference (s1 - s2)
template<class T> set<T> setDifference(const set<T>& s1, const set<T>& s2) {
  set<T> res;
  insert_iterator< set<T> > res_ins(res, res.begin());
  set_difference(s1.begin(),s1.end(),s2.begin(),s2.end(),res_ins);
  return res;
}


// set symmetric difference (union - intersection)
template<class T> set<T> setSymmetricDifference(const set<T>& s1, const set<T>& s2) {
  set<T> res;
  insert_iterator< set<T> > res_ins(res, res.begin());
  set_symmetric_difference(s1.begin(),s1.end(),s2.begin(),s2.end(),res_ins);
  return res;
}


// set subset s1 is a subset of s2 (not necessarily a true subset)
template<class T> bool setSubset(const set<T>& s1, const set<T>& s2) {
  return includes(s2.begin(),s2.end(),s1.begin(),s1.end());
}



// regular printing prototypes
string setWrite(const set<int>& s);
string setWrite(const set<string>& s);


#endif
