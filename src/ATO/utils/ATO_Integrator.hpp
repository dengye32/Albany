//*****************************************************************//
//    Albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#ifndef Integrator_HPP
#define Integrator_HPP

#include "Sacado.hpp"
#include "Intrepid_FieldContainer.hpp"
#include "Teuchos_RCP.hpp"
#include "Shards_CellTopology.hpp"
#include "Intrepid_HGRAD_HEX_Cn_FEM.hpp"
#include <Intrepid_MiniTensor.h>
#include <vector>
#include <functional>


typedef unsigned int uint;
typedef double RealType;
typedef Sacado::Fad::DFad<RealType> FadType;
typedef Sacado::mpl::apply<FadType,RealType>::type DFadType;


namespace ATO {

  enum Sense { Positive, Negative };

/** \brief Conformal integration class

    This class provides basic support for conformal integration.

*/
class Integrator
{

 public:
  Integrator(Teuchos::RCP<shards::CellTopology> celltype,
             Teuchos::RCP<Intrepid::Basis<RealType, Intrepid::FieldContainer<RealType> > > basis);
  virtual ~Integrator(){};

  template<typename C>
  void getMeasure(RealType& measure, 
                 const Intrepid::FieldContainer<RealType>& coordCon, 
                 const Intrepid::FieldContainer<RealType>& topoVals,
                 RealType zeroVal,
                 C comparison);

  void getCubature(std::vector<std::vector<RealType> >& refPoints, std::vector<RealType>& weights, 
                   const Intrepid::FieldContainer<RealType>& topoVals, 
                   const Intrepid::FieldContainer<RealType>& coordCon, 
                   RealType zeroVal);

 private:

  typedef Intrepid::Vector<RealType,3> Vector3D;
  typedef Intrepid::Vector<int,3> Tri;
  typedef Intrepid::Vector<int,4> Tet;

  struct Intersection {
    Intersection(Vector3D p, std::pair<int,int> c):point(p),connect(c){}
    Vector3D point;
    std::pair<int, int> connect;
  };

  struct MiniPoly {
    MiniPoly(){}
    MiniPoly(int n){points.resize(n,Vector3D(Intrepid::ZEROS));mapToBase.resize(n);}
    std::vector<Vector3D> points;
    std::vector<int> mapToBase;
  };

  Teuchos::RCP<shards::CellTopology> cellTopology;

  RealType getTriMeasure(const std::vector< Vector3D >& points,
                  const Tri& tri);

  template<typename C>
  void getSurfaceTris(std::vector< Vector3D >& points,
                      std::vector< Tri >& tris,
                      const Intrepid::FieldContainer<RealType>& topoVals, 
                      const Intrepid::FieldContainer<RealType>& coordCon, 
                      RealType zeroVal, C comparison);

  template<typename C>
  bool included(Teuchos::RCP<MiniPoly> poly,
                const Intrepid::FieldContainer<RealType>& topoVals,
                RealType zeroVal, C compare);

  void trisFromPoly(std::vector< Vector3D >& points,
                    std::vector< Tri >& tris,
                    Teuchos::RCP<MiniPoly> poly);

  void partitionBySegment(std::vector<Teuchos::RCP<MiniPoly> >& polys,
                          const std::pair<Vector3D,Vector3D>& segment);

  void addCubature(std::vector<std::vector<RealType> >& refPoints, std::vector<RealType>& weights, 
                   const Vector3D& c0,
                   const Vector3D& c1,
                   const Vector3D& c2);
  void addCubature(std::vector<std::vector<RealType> >& refPoints, std::vector<RealType>& weights, 
                   const Vector3D& c0,
                   const Vector3D& c1,
                   const Vector3D& c2,
                   const Vector3D& c3);

  Teuchos::RCP<Intrepid::Basis<RealType, Intrepid::FieldContainer<RealType> > > basis;

};

class SubIntegrator
{

 public:
  SubIntegrator(Teuchos::RCP<shards::CellTopology> celltype,
                Teuchos::RCP<Intrepid::Basis<RealType, Intrepid::FieldContainer<RealType> > > basis,
                uint maxRefs, RealType maxErr);
  virtual ~SubIntegrator(){};

  void getMeasure(RealType& measure, 
                  const Intrepid::FieldContainer<RealType>& coordCon, 
                  const Intrepid::FieldContainer<RealType>& topoVals,
                  RealType zeroVal,
                  Sense sense);

  void getMeasure(RealType& measure, 
                  Intrepid::FieldContainer<RealType>& dMdtopo,
                  const Intrepid::FieldContainer<RealType>& coordCon, 
                  const Intrepid::FieldContainer<RealType>& topoVals,
                  RealType zeroVal,
                  Sense sense);

  void getCubature(std::vector<std::vector<RealType> >& refPoints, std::vector<RealType>& weights, 
                   const Intrepid::FieldContainer<RealType>& topoVals, 
                   const Intrepid::FieldContainer<RealType>& coordCon, 
                   RealType zeroVal);

 private:

  template <typename N> struct Positive { typedef std::greater_equal<N> Type; };
  template <typename N> struct Negative { typedef std::less_equal<N> Type; };

  template <typename N> struct Vector3D { typedef Intrepid::Vector<N,3> Type; };
  typedef Intrepid::Vector<int,3> Tri;
  typedef Intrepid::Vector<int,4> Tet;

  template <typename V, typename P>
  struct Simplex {
    Simplex(){}
    Simplex(int n){points.resize(n,typename Vector3D<P>::Type(Intrepid::ZEROS));fieldvals.resize(n);}
    std::vector<Intrepid::Vector<P,3> > points;
    std::vector<V> fieldvals;
  };

  Teuchos::RCP<shards::CellTopology> cellTopology;

  template <typename V, typename P>
  V Volume(Simplex<V,P>& simplex);

  template <typename N, typename V, typename P>
  V Volume(Simplex<V,P>& simplex, const Intrepid::FieldContainer<N>& coordCon);

  template <typename N, typename V, typename P>
  void Project( const Intrepid::FieldContainer<N>& topoVals, 
                std::vector<Simplex<V,P> >& implicitPolys);

  template<typename C, typename V, typename P>
  void Dice(const std::vector<Simplex<V,P> >& implicitPolys, const V zeroVal, 
       const C comparison, std::vector<Simplex<V,P> >& explicitPolys);

  template<typename P>
  void SortMap(const std::vector<typename Vector3D<P>::Type>& points, std::vector<int>& map);

  template<typename V, typename P>
  void getValues( Intrepid::FieldContainer<V>& Nvals, const Intrepid::FieldContainer<P>& evalPoints);

  template<typename V, typename P>
  void Refine( std::vector<Simplex<V,P> >& inpolys,
               std::vector<Simplex<V,P> >& outpolys);

  Teuchos::RCP<Intrepid::Basis<RealType, Intrepid::FieldContainer<RealType> > > basis;
  Teuchos::RCP<Intrepid::Basis<DFadType, Intrepid::FieldContainer<DFadType> > > DFadBasis;

  Intrepid::FieldContainer<RealType> parentCoords;

  std::vector< std::vector<Simplex<RealType,RealType> > > refinement;
  std::vector< std::vector<Simplex<DFadType,DFadType> > > DFadRefinement;

  uint nDims;

  uint maxRefinements;
  RealType maxError;
};

}

#include "ATO_Integrator_Def.hpp"
#endif