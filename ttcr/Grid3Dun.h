//
//  Grid3Dun.h
//  ttcr
//
//  Created by Bernard Giroux on 2014-04-21.
//  Copyright (c) 2014 Bernard Giroux. All rights reserved.
//

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ttcr_Grid3Dun_h
#define ttcr_Grid3Dun_h

#include <cassert>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef VTK
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProbeFilter.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#endif


#include "Grad.h"
#include "Grid3D.h"
#include "Interpolator.h"
#include "utils.h"

namespace ttcr {
    
    template<typename T1, typename T2, typename NODE>
    class Grid3Dun : public Grid3D<T1,T2> {
    public:
        Grid3Dun(const std::vector<sxyz<T1>>& no,
                 const std::vector<tetrahedronElem<T2>>& tet,
                 const size_t nt=1) :
        nThreads(nt),
        nPrimary(static_cast<T2>(no.size())),
        source_radius(0.0),
        nodes(std::vector<NODE>(no.size(), NODE(nt))),
        neighbors(std::vector<std::vector<T2>>(tet.size())),
        tetrahedra(tet)
        {}
        
        virtual ~Grid3Dun() {}
        
        void setSlowness(const T1 s) {
            for ( size_t n=0; n<nodes.size(); ++n ) {
                nodes[n].setNodeSlowness( s[n] );
            }
        }
        
        void setSlowness(const T1 *s, const size_t ns) {
            if ( nodes.size() != ns ) {
                throw std::length_error("Error: slowness vectors of incompatible size.");
            }
            for ( size_t n=0; n<nodes.size(); ++n ) {
                nodes[n].setNodeSlowness( s[n] );
            }
        }
        
        void setSlowness(const std::vector<T1>& s) {
            if ( nodes.size() != s.size() ) {
                throw std::length_error("Error: slowness vectors of incompatible size.");
            }
            for ( size_t n=0; n<nodes.size(); ++n ) {
                nodes[n].setNodeSlowness( s[n] );
            }
        }
        
        void setSourceRadius(const double r) { source_radius = r; }
        
        void setTT(const T1 tt, const size_t nn, const size_t nt=0) {
            nodes[nn].setTT(tt, nt);
        }
        
        size_t getNumberOfNodes() const { return nodes.size(); }
        
        const T1 getXmin() const {
            T1 xmin = nodes[0].getX();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it )
                xmin = xmin<it->getX() ? xmin : it->getX();
            return xmin;
        }
        const T1 getXmax() const {
            T1 xmax = nodes[0].getX();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it )
                xmax = xmax>it->getX() ? xmax : it->getX();
            return xmax;
        }
        const T1 getYmin() const {
            T1 ymin = nodes[0].getY();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it )
                ymin = ymin<it->getY() ? ymin : it->getY();
            return ymin;
        }
        const T1 getYmax() const {
            T1 ymax = nodes[0].getY();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it )
                ymax = ymax>it->getY() ? ymax : it->getY();
            return ymax;
        }
        const T1 getZmin() const {
            T1 zmin = nodes[0].getZ();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it )
                zmin = zmin<it->getZ() ? zmin : it->getZ();
            return zmin;
        }
        const T1 getZmax() const {
            T1 zmax = nodes[0].getZ();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it )
                zmax = zmax>it->getZ() ? zmax : it->getZ();
            return zmax;
        }
        
        virtual void raytrace(const std::vector<sxyz<T1>>& Tx,
                             const std::vector<T1>& t0,
                             const std::vector<sxyz<T1>>& Rx,
                             std::vector<T1>& traveltimes,
                             const size_t threadNo=0) const {}
        
        virtual void raytrace(const std::vector<sxyz<T1>>& Tx,
                             const std::vector<T1>& t0,
                             const std::vector<const std::vector<sxyz<T1>>*>& Rx,
                             std::vector<std::vector<T1>*>& traveltimes,
                             const size_t=0) const {}
        
        virtual void raytrace(const std::vector<sxyz<T1>>& Tx,
                             const std::vector<T1>& t0,
                             const std::vector<sxyz<T1>>& Rx,
                             std::vector<T1>& traveltimes,
                             std::vector<std::vector<sxyz<T1>>>& r_data,
                             const size_t threadNo=0) const {}
        
        virtual void raytrace(const std::vector<sxyz<T1>>& Tx,
                             const std::vector<T1>& t0,
                             const std::vector<const std::vector<sxyz<T1>>*>& Rx,
                             std::vector<std::vector<T1>*>& traveltimes,
                             std::vector<std::vector<std::vector<sxyz<T1>>>*>& r_data,
                             const size_t=0) const {}
        
        void saveTT(const std::string &, const int, const size_t nt=0,
                    const bool vtkFormat=0) const;
        
#ifdef VTK
        void saveModelVTU(const std::string &, const bool saveSlowness=true,
                          const bool savePhysicalEntity=false) const;
#endif
        
        const size_t getNthreads() const { return nThreads; }
        
    protected:
        const size_t nThreads;
        T2 nPrimary;
        T1 source_radius;
        mutable std::vector<NODE> nodes;
        std::vector<std::vector<T2>> neighbors;  // nodes common to a cell
        std::vector<tetrahedronElem<T2>> tetrahedra;
        
        T1 computeDt(const NODE& source, const NODE& node) const {
            return (node.getNodeSlowness()+source.getNodeSlowness())/2 * source.getDistance( node );
        }
        
        T1 computeDt(const NODE& source, const sxyz<T1>& node, T1 slo) const {
            return (slo+source.getNodeSlowness())/2 * source.getDistance( node );
        }
        
        T1 getTraveltime(const sxyz<T1>& Rx,
                         const std::vector<NODE>& nodes,
                         const size_t threadNo) const;
        
        void checkPts(const std::vector<sxyz<T1>>&) const;
        
        bool insideTetrahedron(const sxyz<T1>&, const T2) const;
        
        T2 getCellNo(const sxyz<T1>& pt) const {
            for ( T2 n=0; n<tetrahedra.size(); ++n ) {
                if ( insideTetrahedron(pt, n) ) {
                    return n;
                }
            }
            return -1;
        }
        
        void buildGridNeighbors() {
            // Index the neighbors nodes of each cell
            for ( T2 n=0; n<nodes.size(); ++n ) {
                for ( size_t n2=0; n2<nodes[n].getOwners().size(); ++n2) {
                    neighbors[ nodes[n].getOwners()[n2] ].push_back(n);
                }
            }
        }
        
        void localUpdate3D(NODE *vertexC, const size_t threadNo) const;
        
        T1 localUpdate2D(const NODE *vertexA,
                         const NODE *vertexB,
                         const NODE *vertexC,
                         const T2 tetraNo,
                         const size_t threadNo) const;
        
        void local3Dsolver(NODE *vertexC, const size_t threadNo) const;
        
        T1 local2Dsolver(const NODE *vertexA,
                         const NODE *vertexB,
                         const NODE *vertexC,
                         const T2 tetraNo,
                         const size_t threadNo) const;
        
        int solveEq23(const T1 a[], const T1 b[], T1 n[][3]) const;
        
        bool testInTriangle(const NODE *vertexA,
                            const NODE *vertexB,
                            const NODE *vertexC,
                            const sxyz<T1> &E) const;
        
        void barycentric(const NODE *a, const NODE *b, const NODE *c,
                         const sxyz<T1> &p, T1 &u, T1 &v, T1 &w) const;
        
        void getRaypath(const std::vector<sxyz<T1>>& Tx,
                        const sxyz<T1> &Rx,
                        std::vector<sxyz<T1>> &r_data,
                        const size_t threadNo) const;
        
        void getRaypath_ho(const std::vector<sxyz<T1>>& Tx,
                           const sxyz<T1> &Rx,
                           std::vector<sxyz<T1>> &r_data,
                           const size_t threadNo) const;
        
        void update_m_data(std::vector<sijv<T1>>& m_data,
                           sijv<T1>& m,
                           const std::set<T2>& allNodes,
                           const sxyz<T1>& mid_pt,
                           const T1 s,
                           const T1 ds) const ;
        
        void getRaypath(const std::vector<sxyz<T1>>& Tx,
                        const sxyz<T1> &Rx,
                        std::vector<sxyz<T1>> &r_data,
                        std::vector<sijv<T1>>& m_data,
                        const size_t RxNo,
                        const size_t threadNo) const;
        
        void getRaypath_ho(const std::vector<sxyz<T1>>& Tx,
                           const sxyz<T1> &Rx,
                           std::vector<sxyz<T1>> &r_data,
                           std::vector<sijv<T1>>& m_data,
                           const size_t RxNo,
                           const size_t threadNo) const;
        
        bool intersectVecTriangle(const T2 iO, const sxyz<T1> &vec,
                                  const T2 iA, T2 iB, T2 iC,
                                  sxyz<T1> &pt_i) const;
        bool intersectVecTriangle(const sxyz<T1> &O, const sxyz<T1> &vec,
                                  const T2 iA, T2 iB, T2 iC,
                                  sxyz<T1> &pt_i) const;
        
        bool areCollinear(const sxyz<T1> &pt, const T2 i0, const T2 i1) const;
        bool areCoplanar(const sxyz<T1> &pt, const T2 i0, const T2 i1, const T2 i2) const;
        
        T2 findAdjacentCell1(const std::array<T2,3> &faceNodes, const T2 nodeNo) const;
        T2 findAdjacentCell2(const std::array<T2,3> &faceNodes, const T2 cellNo) const;
        
        void getNeighborNodes(const T2 cellNo, std::set<NODE*> &nnodes) const;
        
        void plotCell(const T2 cellNo, const sxyz<T1> &pt, const sxyz<T1> &g) const;
        
        T1 computeSlowness( const sxyz<T1>& Rx ) const;
    };
    
    template<typename T1, typename T2, typename NODE>
    T1 Grid3Dun<T1,T2,NODE>::getTraveltime(const sxyz<T1>& Rx,
                                           const std::vector<NODE>& nodes,
                                           const size_t threadNo) const {
        
        for ( size_t nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == Rx ) {
                return nodes[nn].getTT(threadNo);
            }
        }
        //If Rx is not on a node:
        T1 slo = computeSlowness( Rx );
        
        T2 cellNo = getCellNo( Rx );
        
        T2 neibNo = neighbors[cellNo][0];
        T1 dt = computeDt(nodes[neibNo], Rx, slo);
        
        T1 traveltime = nodes[neibNo].getTT(threadNo)+dt;
        for ( size_t k=1; k< neighbors[cellNo].size(); ++k ) {
            neibNo = neighbors[cellNo][k];
            dt = computeDt(nodes[neibNo], Rx, slo);
            if ( traveltime > nodes[neibNo].getTT(threadNo)+dt ) {
                traveltime =  nodes[neibNo].getTT(threadNo)+dt;
            }
        }
        return traveltime;
    }
    
    
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::checkPts(const std::vector<sxyz<T1>>& pts) const {
        
        for (size_t n=0; n<pts.size(); ++n) {
            bool found = false;
            // check first if point is on a node
            for ( T2 nt=0; nt<nodes.size(); ++nt ) {
                if ( nodes[nt] == pts[n]) {
                    found = true;
                    break;
                }
            }
            if ( found == false ) {
                // check if inside tetrahedra
                for ( T2 nt=0; nt<tetrahedra.size(); ++nt ) {
                    if ( insideTetrahedron(pts[n], nt) ) {
                        found = true;
                        break;
                    }
                }
            }
            if ( found == false ) {
                std::ostringstream msg;
                msg << "Error: Point (" << pts[n].x << ", " << pts[n].y << ", " << pts[n] .z << ") outside grid.";
                throw std::runtime_error(msg.str());
            }
        }
    }
    
    
    
    template<typename T1, typename T2, typename NODE>
    bool Grid3Dun<T1,T2,NODE>::insideTetrahedron(const sxyz<T1>& v, const T2 nt) const {
        
        
        // from http://steve.hollasch.net/cgindex/geometry/ptintet.html
        
        sxyz<T1> v1 = { nodes[ tetrahedra[nt].i[0] ].getX(),
            nodes[ tetrahedra[nt].i[0] ].getY(),
            nodes[ tetrahedra[nt].i[0] ].getZ() };
        
        sxyz<T1> v2 = { nodes[ tetrahedra[nt].i[1] ].getX(),
            nodes[ tetrahedra[nt].i[1] ].getY(),
            nodes[ tetrahedra[nt].i[1] ].getZ() };
        
        sxyz<T1> v3 = { nodes[ tetrahedra[nt].i[2] ].getX(),
            nodes[ tetrahedra[nt].i[2] ].getY(),
            nodes[ tetrahedra[nt].i[2] ].getZ() };
        
        sxyz<T1> v4 = { nodes[ tetrahedra[nt].i[3] ].getX(),
            nodes[ tetrahedra[nt].i[3] ].getY(),
            nodes[ tetrahedra[nt].i[3] ].getZ() };
        
        T1 D0 = det4(v1, v2, v3, v4);
        T1 D1 = det4( v, v2, v3, v4);
        T1 D2 = det4(v1,  v, v3, v4);
        T1 D3 = det4(v1, v2,  v, v4);
        T1 D4 = det4(v1, v2, v3,  v);
        
        int t1, t2, t3, t4;
        
        if ( fabs(D1)<small ) {
            // points are coplanar, check if pt is inside triangle
            if ( testInTriangle(&(nodes[ tetrahedra[nt].i[1] ]),
                                &(nodes[ tetrahedra[nt].i[2] ]),
                                &(nodes[ tetrahedra[nt].i[3] ]), v))
                return 1;
            else
                return 0;
            
        } else {
            t1 = (signum(D0)==signum(D1));
        }
        
        if ( fabs(D2)<small ) {
            if ( testInTriangle(&(nodes[ tetrahedra[nt].i[0] ]),
                                &(nodes[ tetrahedra[nt].i[2] ]),
                                &(nodes[ tetrahedra[nt].i[3] ]), v))
                return 1;
            else
                return 0;
        } else {
            t2 = (signum(D0)==signum(D2));
        }
        
        if ( fabs(D3)<small ) {
            if ( testInTriangle(&(nodes[ tetrahedra[nt].i[0] ]),
                                &(nodes[ tetrahedra[nt].i[1] ]),
                                &(nodes[ tetrahedra[nt].i[3] ]), v))
                return 1;
            else
                return 0;
        } else {
            t3 = (signum(D0)==signum(D3));
        }
        
        if ( fabs(D4)<small ) {
            if ( testInTriangle(&(nodes[ tetrahedra[nt].i[0] ]),
                                &(nodes[ tetrahedra[nt].i[1] ]),
                                &(nodes[ tetrahedra[nt].i[2] ]), v))
                return 1;
            else
                return 0;
        } else {
            t4 = (signum(D0)==signum(D4));
        }
        
        return t1 && t2 && t3 && t4;
    }
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::saveTT(const std::string &fname, const int all,
                                      const size_t nt, const bool vtkFormat) const {
        
        if (vtkFormat) {
#ifdef VTK
            std::string filename = fname+".vtu";
            
            vtkSmartPointer<vtkUnstructuredGrid> ugrid =
            vtkSmartPointer<vtkUnstructuredGrid>::New();
            
            vtkSmartPointer<vtkPoints> newPts =
            vtkSmartPointer<vtkPoints>::New();
            vtkSmartPointer<vtkDoubleArray> newScalars =
            vtkSmartPointer<vtkDoubleArray>::New();
            
            newScalars->SetName("Travel time");
            
            double xyz[3];
            T2 nMax = nPrimary;  // only primary are saved
            for (size_t n=0; n<nMax; ++n) {
                xyz[0] = nodes[n].getX();
                xyz[1] = nodes[n].getY();
                xyz[2] = nodes[n].getZ();
                newPts->InsertPoint(n, xyz);
                newScalars->InsertValue(n, nodes[n].getTT(nt) );
            }
            
            ugrid->SetPoints(newPts);
            ugrid->GetPointData()->SetScalars(newScalars);
            
            vtkSmartPointer<vtkTetra> tet =
            vtkSmartPointer<vtkTetra>::New();
            for (size_t n=0; n<tetrahedra.size(); ++n) {
                tet->GetPointIds()->SetId(0, tetrahedra[n].i[0] );
                tet->GetPointIds()->SetId(1, tetrahedra[n].i[1] );
                tet->GetPointIds()->SetId(2, tetrahedra[n].i[2] );
                tet->GetPointIds()->SetId(3, tetrahedra[n].i[3] );
                
                ugrid->InsertNextCell( tet->GetCellType(), tet->GetPointIds() );
            }
            vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
            vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
            
            writer->SetFileName( filename.c_str() );
            //		writer->SetInputConnection( ugrid->GetProducerPort() );
            writer->SetInputData( ugrid );
            writer->SetDataModeToBinary();
            writer->Update();
#else
            std::cerr << "VTK not included during compilation.\nNothing saved.\n";
#endif
        } else {
            std::string filename = fname+".dat";
            std::ofstream fout(filename.c_str());
            T2 nMax = nPrimary;
            if ( all == 1 ) {
                nMax = static_cast<T2>(nodes.size());
            }
            for ( T2 n=0; n<nMax; ++n ) {
                fout << nodes[n].getX() << '\t'
                << nodes[n].getY() << '\t'
                << nodes[n].getZ() << '\t'
                << nodes[n].getTT(nt) << '\n';
            }
            fout.close();
        }
    }
    
#ifdef VTK
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::saveModelVTU(const std::string &fname,
                                            const bool saveSlowness,
                                            const bool savePhysicalEntity) const {
        
        vtkSmartPointer<vtkUnstructuredGrid> ugrid =
        vtkSmartPointer<vtkUnstructuredGrid>::New();
        
        vtkSmartPointer<vtkPoints> newPts =
        vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkDoubleArray> newScalars =
        vtkSmartPointer<vtkDoubleArray>::New();
        
        double xyz[3];
        T2 nMax = nPrimary;  // only primary are saved
        if ( saveSlowness ) {
            newScalars->SetName("Slowness");
            
            for (size_t n=0; n<nMax; ++n) {
                xyz[0] = nodes[n].getX();
                xyz[1] = nodes[n].getY();
                xyz[2] = nodes[n].getZ();
                newPts->InsertPoint(n, xyz);
                newScalars->InsertValue(n, nodes[n].getNodeSlowness() );
            }
        } else {
            newScalars->SetName("Velocity");
            
            for (size_t n=0; n<nMax; ++n) {
                xyz[0] = nodes[n].getX();
                xyz[1] = nodes[n].getY();
                xyz[2] = nodes[n].getZ();
                newPts->InsertPoint(n, xyz);
                newScalars->InsertValue(n, static_cast<T1>(1.0)/nodes[n].getNodeSlowness() );
            }
        }
        
        ugrid->SetPoints(newPts);
        ugrid->GetPointData()->SetScalars(newScalars);
        
        vtkSmartPointer<vtkTetra> tet =
        vtkSmartPointer<vtkTetra>::New();
        
        for (size_t n=0; n<tetrahedra.size(); ++n) {
            tet->GetPointIds()->SetId(0, tetrahedra[n].i[0] );
            tet->GetPointIds()->SetId(1, tetrahedra[n].i[1] );
            tet->GetPointIds()->SetId(2, tetrahedra[n].i[2] );
            tet->GetPointIds()->SetId(3, tetrahedra[n].i[3] );
            
            ugrid->InsertNextCell( tet->GetCellType(), tet->GetPointIds() );
        }
        
        vtkSmartPointer<vtkIntArray> data_pe = vtkSmartPointer<vtkIntArray>::New();
        if ( savePhysicalEntity ) {
            data_pe->SetName("Physical entity");
            for (size_t n=0; n<tetrahedra.size(); ++n) {
                data_pe->InsertNextValue(tetrahedra[n].physical_entity );
            }
            ugrid->GetCellData()->AddArray(data_pe);
        }
        
        vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
        vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
        
        writer->SetFileName( fname.c_str() );
        writer->SetInputData( ugrid );
        writer->SetDataModeToBinary();
        writer->Update();
        
    }
    
#endif
    
    
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::localUpdate3D(NODE *vertexD,
                                             const size_t threadNo) const {
        
        // méthode of Lelievre et al. 2011
        
        T2 iA, iB, iC, iD;
        NODE *vertexA, *vertexB, *vertexC;
        
        for ( size_t no=0; no<vertexD->getOwners().size(); ++no ) {
            
            T2 tetNo = vertexD->getOwners()[no];
            
            for ( iD=0; iD<4; ++iD ) {
                if ( vertexD->getGridIndex() == tetrahedra[tetNo].i[iD] ) break;
            }
            
            iA = (iD+1)%4;
            iB = (iD+2)%4;
            iC = (iD+3)%4;
            vertexA = &(nodes[tetrahedra[tetNo].i[iA]]);
            vertexB = &(nodes[tetrahedra[tetNo].i[iB]]);
            vertexC = &(nodes[tetrahedra[tetNo].i[iC]]);
            
            if ( vertexA->getTT(threadNo) > vertexB->getTT(threadNo) ) {
                std::swap(iA, iB);
                std::swap(vertexA, vertexB);
            }
            if ( vertexA->getTT(threadNo) > vertexC->getTT(threadNo) ) {
                std::swap(iA, iC);
                std::swap(vertexA, vertexC);
            }
            if ( vertexB->getTT(threadNo) > vertexC->getTT(threadNo) ) {
                std::swap(iB, iC);
                std::swap(vertexB, vertexC);
            }

            if ( vertexA->getTT(threadNo) == std::numeric_limits<T1>::max() ) {
                continue;
            }
            
            T1 tABC = std::numeric_limits<T1>::max();
            
            if ( vertexB->getTT(threadNo) != std::numeric_limits<T1>::max() &&
                vertexC->getTT(threadNo) != std::numeric_limits<T1>::max() ) {
                
                T1 u = vertexB->getTT(threadNo) - vertexA->getTT(threadNo);
                T1 v = vertexC->getTT(threadNo) - vertexA->getTT(threadNo);
                
                sxyz<T1> v_b = { vertexC->getX() - vertexA->getX(),
                    vertexC->getY() - vertexA->getY(),
                    vertexC->getZ() - vertexA->getZ() };
                sxyz<T1> v_c = { vertexB->getX() - vertexA->getX(),
                    vertexB->getY() - vertexA->getY(),
                    vertexB->getZ() - vertexA->getZ() };
                
                // vector normal to plane
                sxyz<T1> v_n = cross(v_b, v_c);
                
                T1 b = norm( v_b );
                T1 c = norm( v_c );
                T1 d2 = dot(v_b, v_c);
                
                T1 alpha = acos( d2 / (b*c) );
                
                T1 phi = c*b*sin(alpha);
                
                // check for negative value
                T1 w_tilde = vertexD->getNodeSlowness()*vertexD->getNodeSlowness()*phi*phi -
                                  u*u*b*b - v*v*c*c + 2.*u*v*d2;
                if ( w_tilde > 0.0 ) {
                
                    w_tilde = sqrt( w_tilde );
                    
                    // Point (ξ_0 , ζ_0 ) is the normalized projection of node D onto face ABC
                    // project D on plane
                    
                    T1 d_tmp = -vertexA->getX()*v_n.x - vertexA->getY()*v_n.y - vertexA->getZ()*v_n.z;
                    
                    T1 k = -(d_tmp + v_n.x*vertexD->getX() + v_n.y*vertexD->getY() + v_n.z*vertexD->getZ())/
                    norm2(v_n);
                    
                    sxyz<T1> pt;   // -> Point (ξ_0 , ζ_0 )
                    pt.x = vertexD->getX() + k*v_n.x;
                    pt.y = vertexD->getY() + k*v_n.y;
                    pt.z = vertexD->getZ() + k*v_n.z;
                    
                    T1 rho0 = vertexD->getDistance( pt );
                    
                    sxyz<T1> v_pt = {pt.x-vertexA->getX(), pt.y-vertexA->getY(), pt.z-vertexA->getZ()};
                    
                    T1 xi0;
                    T1 zeta0;
                    projNorm(v_b/b, v_c/c, v_pt, xi0, zeta0);
                    if ( xi0 < 0.0 || zeta0 < 0.0 ) {
                        // this should not happen unless we have incorrect triangle
                        continue;
                    }
                    
                    T1 beta = u*b*b - v*d2;
                    T1 gamma = v*c*c - u*d2;
                    
                    T1 xi_tilde = -fabs(beta)*rho0/(phi*w_tilde);
                    T1 zeta_tilde = -fabs(gamma)*rho0/(phi*w_tilde);
                    
                    T1 xi = xi_tilde + xi0;
                    T1 zeta = zeta_tilde + zeta0;
                    
                    if ( 0.<xi && xi<1. && 0.<zeta && zeta<1. && 0.<(xi+zeta) && (xi+zeta)<1. ) {
                        tABC = vertexA->getTT(threadNo) + u*xi0 + v*zeta0 + w_tilde*rho0/phi;
                    }
                }
            }
            
            T1 t = vertexA->getTT(threadNo) + vertexD->getNodeSlowness() * vertexD->getDistance( *vertexA );
            if ( t < tABC ) tABC = t;
            t = vertexB->getTT(threadNo) + vertexD->getNodeSlowness() * vertexD->getDistance( *vertexB );
            if ( t < tABC ) tABC = t;
            t = vertexC->getTT(threadNo) + vertexD->getNodeSlowness() * vertexD->getDistance( *vertexC );
            if ( t < tABC ) tABC = t;
            
            t = localUpdate2D(vertexA, vertexB, vertexD, tetNo, threadNo);
            if ( t < tABC ) tABC = t;
            t = localUpdate2D(vertexA, vertexC, vertexD, tetNo, threadNo);
            if ( t < tABC ) tABC = t;
            t = localUpdate2D(vertexB, vertexC, vertexD, tetNo, threadNo);
            if ( t < tABC ) tABC = t;
            
            if ( tABC<vertexD->getTT(threadNo) )
                vertexD->setTT(tABC, threadNo);
            
        }
    }
    
    
    template<typename T1, typename T2, typename NODE>
    T1 Grid3Dun<T1,T2,NODE>::localUpdate2D(const NODE *vertexA,
                                           const NODE *vertexB,
                                           const NODE *vertexC,
                                           const T2 tetNo,
                                           const size_t threadNo) const {
        
        if ( vertexB->getTT(threadNo)==std::numeric_limits<T1>::max() &&
            vertexA->getTT(threadNo)==std::numeric_limits<T1>::max() ) {
            return std::numeric_limits<T1>::max();
        }
        T1 t;
        
        T1 u = vertexB->getTT(threadNo) - vertexA->getTT(threadNo);
        
        sxyz<T1> v_b = { vertexC->getX() - vertexA->getX(),
            vertexC->getY() - vertexA->getY(),
            vertexC->getZ() - vertexA->getZ() };
        sxyz<T1> v_c = { vertexB->getX() - vertexA->getX(),
            vertexB->getY() - vertexA->getY(),
            vertexB->getZ() - vertexA->getZ() };
        
        T1 c = norm( v_c );
        
        T1 w2 = vertexC->getNodeSlowness()*vertexC->getNodeSlowness()*c*c - u*u;
        if ( w2 < 0.0 ) return std::numeric_limits<T1>::max();
        
        T1 w = sqrt( w2 );
        
        T1 k = dot(v_b,v_c)/dot(v_c,v_c);
        sxyz<T1> pt;
        pt.x = vertexA->getX() + k*v_c.x;
        pt.y = vertexA->getY() + k*v_c.y;
        pt.z = vertexA->getZ() + k*v_c.z;
        
        T1 rho0 = vertexC->getDistance( pt );
//        T1 xi0 = vertexA->getDistance( pt )/c;
        T1 xi0 = k;
        
        T1 xi = xi0 - u*rho0/(w*c);
        
        if ( 0.<xi && xi<1. ) {
            t = vertexA->getTT(threadNo) + u*xi0 + w*rho0/c;
        } else {
            t = std::numeric_limits<T1>::max();
        }
        
        return t;
    }
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::local3Dsolver(NODE *vertexD,
                                             const size_t threadNo) const {
        
        // Méthode de Qian et al. 2007
        
        T2 iA, iB, iC, iD;
        NODE *vertexA, *vertexB, *vertexC;
        T1 AB, AC;
        
        for ( size_t no=0; no<vertexD->getOwners().size(); ++no ) {
            
            T2 tetNo = vertexD->getOwners()[no];
            
            for ( iD=0; iD<4; ++iD ) {
                if ( vertexD->getGridIndex() == tetrahedra[tetNo].i[iD] ) break;
            }
            
            iA = (iD+1)%4;
            iB = (iD+2)%4;
            iC = (iD+3)%4;
            vertexA = &(nodes[tetrahedra[tetNo].i[iA]]);
            vertexB = &(nodes[tetrahedra[tetNo].i[iB]]);
            vertexC = &(nodes[tetrahedra[tetNo].i[iC]]);
            
            if ( vertexA->getTT(threadNo) > vertexB->getTT(threadNo) ) {
                std::swap(iA, iB);
                std::swap(vertexA, vertexB);
            }
            if ( vertexA->getTT(threadNo) > vertexC->getTT(threadNo) ) {
                std::swap(iA, iC);
                std::swap(vertexA, vertexC);
            }
            
            if ( vertexA->getTT(threadNo) == std::numeric_limits<T1>::max() ) {
                continue;
            }
            
            AB = vertexA->getDistance( *vertexB );
            AC = vertexA->getDistance( *vertexC );
            
            bool apply2Dsolvers = true;
            
            if (fabs(vertexB->getTT(threadNo)-vertexA->getTT(threadNo))<=AB*vertexD->getNodeSlowness() &&
                fabs(vertexC->getTT(threadNo)-vertexA->getTT(threadNo))<=AC*vertexD->getNodeSlowness()) {
                
                // Qian et al, 2007, eq 2.3
                
                T1 ab[4], ac[4], n[2][3];
                
                // vec(AB)
                ab[0] = vertexB->getX()-vertexA->getX();
                ab[1] = vertexB->getY()-vertexA->getY();
                ab[2] = vertexB->getZ()-vertexA->getZ();
                
                ab[3] = (vertexB->getTT(threadNo)-vertexA->getTT(threadNo)) / vertexD->getNodeSlowness();
                
                // vec(AC)
                ac[0] = vertexC->getX()-vertexA->getX();
                ac[1] = vertexC->getY()-vertexA->getY();
                ac[2] = vertexC->getZ()-vertexA->getZ();
                
                ac[3] = (vertexC->getTT(threadNo)-vertexA->getTT(threadNo)) / vertexD->getNodeSlowness();
                
                int rv = solveEq23(ab, ac, n);
                
                if ( rv == 1 ) {
                    
                    for ( size_t ns=0; ns<2; ++ns ) {
                        //
                        // find pt E
                        //
                        
                        // plane vec(AB) cross vec(AC) passing by A: ax + by + cz + d = 0
                        
                        T1 a = ab[1]*ac[2] - ac[1]*ab[2];
                        T1 b = ac[0]*ab[2] - ab[0]*ac[2];
                        T1 c = ab[0]*ac[1] - ac[0]*ab[1];
                        
                        T1 d = -vertexA->getX()*a - vertexA->getY()*b - vertexA->getZ()*c;
                        
                        //				T1 k = -(d + a*vertexA->getX() + b*vertexA->getY() + c*vertexA->getZ())/
                        //				(a*n[0] + b*n[1] + c*n[2]);
                        T1 k = -(d + a*vertexD->getX() + b*vertexD->getY() + c*vertexD->getZ())/  // TODO check here if vertex D
                        (a*n[ns][0] + b*n[ns][1] + c*n[ns][2]);
                        
                        sxyz<T1> E;
                        E.x = vertexD->getX() + k*n[ns][0];
                        E.y = vertexD->getY() + k*n[ns][1];
                        E.z = vertexD->getZ() + k*n[ns][2];
                        
                        if ( testInTriangle(vertexA, vertexB, vertexC, E) ) {
                            
                            // find point on wavefront plane
                            
                            a = n[ns][0];
                            b = n[ns][1];
                            c = n[ns][2];
                            d = -vertexA->getX()*a - vertexA->getY()*b - vertexA->getZ()*c;
                            
                            k = -(d + a*vertexD->getX() + b*vertexD->getY() + c*vertexD->getZ())/
                            (a*n[ns][0] + b*n[ns][1] + c*n[ns][2]);
                            
                            sxyz<T1> pt;
                            pt.x = vertexD->getX() + k*n[ns][0];
                            pt.y = vertexD->getY() + k*n[ns][1];
                            pt.z = vertexD->getZ() + k*n[ns][2];
                            
                            sxyz<T1> AD;
                            AD.x = vertexD->getX() - vertexA->getX();
                            AD.y = vertexD->getY() - vertexA->getY();
                            AD.z = vertexD->getZ() - vertexA->getZ();
                            
                            T1 d2 = vertexD->getDistance( E );
                            T1 d3 = vertexD->getDistance( pt );
                            T1 d4 = fabs( AD.x*n[ns][0] + AD.y*n[ns][1] + AD.z*n[ns][2] );
                            
                            if ( fabs(d3-d4)>small ) {
                                std::cout << " d3 ne d4: " << d3 << '\t' << d4 << '\t' << d2 << '\n';
                            }
                            
                            T1 t = vertexA->getTT(threadNo) +
                            d3*vertexD->getNodeSlowness();
                            
                            if ( t<vertexD->getTT(threadNo) )
                                vertexD->setTT(t, threadNo);
                            
                            apply2Dsolvers = false;
                            break;
                        }
                    }
                }
            }
            
            if ( apply2Dsolvers ) {
                T1 tABD = local2Dsolver(vertexA, vertexB, vertexD, tetNo, threadNo);
                T1 tACD = local2Dsolver(vertexA, vertexC, vertexD, tetNo, threadNo);
                T1 tBCD = local2Dsolver(vertexB, vertexC, vertexD, tetNo, threadNo);
                
                T1 t = tABD < tACD ? tABD : tACD;
                t = t < tBCD ? t : tBCD;
                
                if ( t<vertexD->getTT(threadNo) )
                    vertexD->setTT(t, threadNo);
            }
            
        }
    }
    
    template<typename T1, typename T2, typename NODE>
    T1 Grid3Dun<T1,T2,NODE>::local2Dsolver(const NODE *vertexA,
                                           const NODE *vertexB,
                                           const NODE *vertexC,
                                           const T2 tetraNo,
                                           const size_t threadNo) const {
        static const double pi2 = pi / 2.;
        
        if ( vertexB->getTT(threadNo)==std::numeric_limits<T1>::max() &&
            vertexA->getTT(threadNo)==std::numeric_limits<T1>::max() ) {
            return std::numeric_limits<T1>::max();
        }
        
        T1 t;
        
        T1 a = vertexB->getDistance( *vertexC );
        T1 b = vertexA->getDistance( *vertexC );
        T1 c = vertexA->getDistance( *vertexB );
        if ( fabs(vertexB->getTT(threadNo)-vertexA->getTT(threadNo))<= c*vertexC->getNodeSlowness() ) {
            
            T1 theta = asin( fabs(vertexB->getTT(threadNo)-vertexA->getTT(threadNo))/
                            (c*vertexC->getNodeSlowness()) );
            
            T1 gamma = acos((a*a + b*b - c*c)/(2.*a*b));
            
            if ( gamma > pi2 ) {
                std::cout << "*** Obtuse angle: " << gamma*57.2957795 << " ***\n";
            } else {
                std::cout << "Accute angle: " << gamma*57.2957795 << " \n";
            }
            
            T1 beta  = acos((b*b + c*c - a*a)/(2.*b*c));
            T1 alpha = acos((a*a + c*c - b*b)/(2.*a*c));
            
            if ( ((0.>alpha-pi2?0.:alpha-pi2)<=theta && theta<=(pi2-beta) ) ||
                ((alpha-pi2)<=theta && theta<=(0.<pi2-beta?0.:pi2-beta)) ) {
                T1 h = a*sin(alpha-theta);
                T1 H = b*sin(beta+theta);
                t = 0.5*(h*vertexC->getNodeSlowness() + vertexB->getTT(threadNo)) +
                0.5 *(H*vertexC->getNodeSlowness() + vertexA->getTT(threadNo));
                
            } else {
                t = vertexA->getTT(threadNo) + b*vertexC->getNodeSlowness();
                t = t < vertexB->getTT(threadNo) + a*vertexC->getNodeSlowness() ? t : vertexB->getTT(threadNo) + a*vertexC->getNodeSlowness();
            }
        } else {
            t = vertexA->getTT(threadNo) + b*vertexC->getNodeSlowness();
            t = t < vertexB->getTT(threadNo) + a*vertexC->getNodeSlowness() ? t : vertexB->getTT(threadNo) + a*vertexC->getNodeSlowness();
        }
        t = t<vertexC->getTT(threadNo) ? t : vertexC->getTT(threadNo);
        
        return t;
    }
    
    
    template<typename T1, typename T2, typename NODE>
    int Grid3Dun<T1,T2,NODE>::solveEq23(const T1 a[], const T1 b[], T1 n[][3]) const {
        // returns 0 if no solution
        //         1 if solutions exist
        
        T1 a02 = a[0]*a[0];
        T1 a12 = a[1]*a[1];
        T1 a22 = a[2]*a[2];
        T1 a32 = a[3]*a[3];
        T1 b02 = b[0]*b[0];
        T1 b12 = b[1]*b[1];
        T1 b22 = b[2]*b[2];
        T1 b32 = b[3]*b[3];
        T1 a23 = a[2]*a[2]*a[2];
        
        T1 s1 = (a[2]*b[1] - a[1]*b[2])*(a[2]*b[1] - a[1]*b[2])*
        (a02*(b12 + b22) - a32*(b02 + b12 + b22) + 2*a[0]*a[3]*b[0]*b[3] - a02*b32 -
         2*a[1]*b[1]*(a[0]*b[0] + a[2]*b[2] - a[3]*b[3]) + 2*a[2]*b[2]*
         (-(a[0]*b[0]) + a[3]*b[3]) + a22*(b02 + b12 - b32) + a12*(b02 + b22 - b32));
        
        if ( s1 < 0.0 ) {
            return 0;
        } else {
            
            T1 d1 = (a22*(b02 + b12) - 2*a[0]*a[2]*b[0]*b[2] -
                     2*a[1]*b[1]*(a[0]*b[0] + a[2]*b[2])  +
                     a12*(b02 + b22) + a02*(b12 + b22));
            T1 d2 = ((a[2]*b[1] - a[1]*b[2]) *(a22*(b02 + b12) -
                                               2*a[0]*a[2]*b[0]*b[2] -
                                               2*a[1]*b[1]*(a[0]*b[0] + a[2]*b[2]) +
                                               a12*(b02 + b22) + a02*(b12 + b22)));
            
            if ( d1==0.0 || d2==0.0 ) return 0;
            
            s1 = sqrt(s1);
            
            n[0][0] = (a[0]*a[3]*(b12 + b22) + a12*b[0]*b[3] - a[0]*a[2]*b[2]*b[3] -
                       a[1]*b[1]*(a[3]*b[0] + a[0]*b[3]) +
                       a[2]*b[0]*(-(a[3]*b[2]) + a[2]*b[3])  - s1) / d1;
            
            n[0][1] = (a[2]*a[3]*b[1]*(-(a[0]*b[0]*b[1])  + a[1]*(b02 + 2*b22)) +
                       a23*b12*b[3] + a[2]*(a[0]*b[1]*(-(a[1]*b[0])  + a[0]*b[1]) +
                                            a12*b22)*b[3] -
                       a22*b[1]*b[2]*(a[3]*b[1] + 2*a[1]*b[3]) -
                       a[1]*b[2]*(a[1]*a[3]*(b02 + b22) - a[0]*a[1]*b[0]*b[3] +
                                  a[0]*b[1]*(-(a[3]*b[0])  + a[0]*b[3]) ) +
                       a[2]*b[0]*s1 - a[0]*b[2]*s1) / d2;
            
            n[0][2] = (a[1]*b22*(a[3]*(a[0]*b[0] + a[1]*b[1])  - (a02 + a12)*b[3]) +
                       a22*b[1]*(a[3]*(b02 + b12) - (a[0]*b[0] + a[1]*b[1]) *b[3]) +
                       a[2]*b[2]*(-(a[1]*a[3]*(b02 + 2*b12)) + a[0]*a[1]*b[0]*b[3] +
                                  2*a12*b[1]*b[3] + a[0]*b[1]*(-(a[3]*b[0]) + a[0]*b[3]) ) -
                       a[1]*b[0]*s1 + a[0]*b[1]*s1)/ d2;
            
            n[1][0] = (a[0]*a[3]*(b12 + b22) + a12*b[0]*b[3] - a[0]*a[2]*b[2]*b[3] -
                       a[1]*b[1]*(a[3]*b[0] + a[0]*b[3]) +
                       a[2]*b[0]*(-(a[3]*b[2]) + a[2]*b[3])  + s1) / d1;
            
            n[1][1] = (a[2]*a[3]*b[1]*(-(a[0]*b[0]*b[1])  + a[1]*(b02 + 2*b22)) +
                       a23*b12*b[3] + a[2]*(a[0]*b[1]*(-(a[1]*b[0])  + a[0]*b[1]) +
                                            a12*b22)*b[3] - a22*b[1]*b[2]*(a[3]*b[1] +
                                                                           2*a[1]*b[3]) -
                       a[1]*b[2]*(a[1]*a[3]*(b02 + b22) - a[0]*a[1]*b[0]*b[3] +
                                  a[0]*b[1]*(-(a[3]*b[0])  + a[0]*b[3]) ) -
                       a[2]*b[0]*s1 + a[0]*b[2]*s1) / d2;
            
            n[1][2] = (a[1]*b22*(a[3]*(a[0]*b[0] + a[1]*b[1])  - (a02 + a12)*b[3]) +
                       a22*b[1]*(a[3]*(b02 + b12) - (a[0]*b[0] + a[1]*b[1]) *b[3]) +
                       a[2]*b[2]*(-(a[1]*a[3]*(b02 + 2*b12)) + a[0]*a[1]*b[0]*b[3] +
                                  2*a12*b[1]*b[3] + a[0]*b[1]*(-(a[3]*b[0])  + a[0]*b[3]) ) +
                       a[1]*b[0]*s1 - a[0]*b[1]*s1) / d2;
        }
        return 1;
    }
    
    
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::getRaypath(const std::vector<sxyz<T1>>& Tx,
                                          const sxyz<T1> &Rx,
                                          std::vector<sxyz<T1>> &r_data,
                                          const size_t threadNo) const {
        
        T1 minDist = small;
        std::vector<sxyz<T1>> r_tmp;
        r_tmp.emplace_back( Rx );
        
        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                return;
            }
        }
        
        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn] == Tx[nt] ) {
                    txOnNode[nt] = true;
                    txNode[nt] = nn;
                    break;
                }
            }
        }
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );
                
                // find adjacent cells
                T2 ind[6][2] = {
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][1]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][2], neighbors[txCell[nt]][3]} };
                
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
            }
        }
        
        T2 cellNo, nodeNo;
        sxyz<T1> curr_pt( Rx );
        
        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
        std::array<T2,2> edgeNodes;
        std::array<T2,3> faceNodes;
        Grad3D<T1,NODE> grad3d;
        bool reachedTx = false;
        
        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == curr_pt ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );
            
            T2 ind[6][2] = {
                {neighbors[cellNo][0], neighbors[cellNo][1]},
                {neighbors[cellNo][0], neighbors[cellNo][2]},
                {neighbors[cellNo][0], neighbors[cellNo][3]},
                {neighbors[cellNo][1], neighbors[cellNo][2]},
                {neighbors[cellNo][1], neighbors[cellNo][3]},
                {neighbors[cellNo][2], neighbors[cellNo][3]} };
            
            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, ind[n][0], ind[n][1]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
            if ( !onEdge ) {
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } }
                };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                
                for ( size_t n=0; n<4; ++n ) {
                    if ( areCoplanar(curr_pt, ind[n][0], ind[n][1], ind[n][2]) ) {
                        onFace = true;
                        faceNodes[0] = ind[n][0];
                        faceNodes[1] = ind[n][1];
                        faceNodes[2] = ind[n][2];
                        break;
                    }
                }
            }
        }
        
        while ( reachedTx == false ) {
            
            if ( onNode ) {
                
                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    cells.push_back( *nc );
                }
                
                // compute gradient with nodes from all common cells
                std::set<NODE*> nnodes;
                for (size_t n=0; n<cells.size(); ++n ) {
                    for ( size_t no=0; no<4; ++no ) {
                        nnodes.insert( &(nodes[ neighbors[cells[n]][no] ]) );
                    }
                }
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    
                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=neighbors[*nc].begin(); nn!=neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());
                    
                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    r_tmp.push_back( curr_pt );
                    
                    bool break_flag=false;
                    for ( n=0; n<3; ++n ) {
                        if ( nodes[ nb[n] ].getDistance( curr_pt ) < small ) {
                            nodeNo = nb[n];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    for ( size_t n1=0; n1<3; ++n1 ) {   // changed n1<2  -> n1<3
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, nb[n1], nb[n2]) ) {
                            edgeNodes[0] = nb[n1];
                            edgeNodes[1] = nb[n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodes = nb;
                    
                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on node failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else if ( onEdge ) {
                
                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() )
                        cells.push_back( *nc0 );
                }
                // compute gradient with nodes from all common cells
                std::set<NODE*> nnodes;
                for (size_t n=0; n<cells.size(); ++n ) {
                    for ( size_t no=0; no<4; ++no ) {
                        nnodes.insert( &(nodes[ neighbors[cells[n]][no] ]) );
                    }
                }
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {
                    
                    cellNo = cells[n];
                    
                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=neighbors[cellNo].begin(); nn!= neighbors[cellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }
                    
                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<4; ++n2 ) {
                        if ( nodes[ neighbors[cellNo][n2] ].getDistance( curr_pt ) < small ) {
                            nodeNo = neighbors[cellNo][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    if ( areCollinear(curr_pt, itmpNode, edgeNodes2[0]) ) {
                        edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[0];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, itmpNode, edgeNodes2[1]) ) {
                        edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, edgeNodes2[0], edgeNodes2[1]) ) {
                        edgeNodes[0] = edgeNodes2[0];
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        break_flag = true;
                        break;
                    }
                    if ( break_flag ) break;
                    
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodes[0] = itmpNode;
                    faceNodes[1] = edgeNodes2[0];
                    faceNodes[2] = edgeNodes2[1];
                    std::sort(faceNodes.begin(), faceNodes.end());
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on edge failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else { // on Face
                
                assert(neighbors[cellNo].size()==4);
                sxyz<T1> g = grad3d.ls_grad(nodes[ neighbors[cellNo][0] ],
                                            nodes[ neighbors[cellNo][1] ],
                                            nodes[ neighbors[cellNo][2] ],
                                            nodes[ neighbors[cellNo][3] ],
                                            threadNo);
                
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } }
                };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                // there are 3 faces that might be intersected
                
                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);
                    
                    if ( !foundIntersection )
                        continue;
                    
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<3; ++n2 ) {
                        if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
                            nodeNo = ind[n][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    for ( size_t n1=0; n1<3; ++n1 ) {
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
                            edgeNodes[0] = ind[n][n1];
                            edgeNodes[1] = ind[n][n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodes = ind[n];
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    
                    // we must be on an face with gradient pointing slightly outward tetrahedron
                    // return in other cell but keep gradient
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    
                    ind[0] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } };
                    ind[1] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } };
                    ind[2] = { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } };
                    ind[3] = { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } };
                    
                    for ( size_t n=0; n<4; ++n )
                        std::sort( ind[n].begin(), ind[n].end() );
                    
                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;
                        
                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);
                        
                        if ( !foundIntersection ) {
                            continue;
                        }
                        curr_pt = pt_i;
                        r_tmp.push_back( curr_pt );
                        
                        bool break_flag = false;
                        for ( size_t n2=0; n2<3; ++n2 ) {
                            if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
                                nodeNo = ind[n][n2];
                                onNode = true;
                                onEdge = false;
                                onFace = false;
                                break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
                        for ( size_t n1=0; n1<3; ++n1 ) {
                            size_t n2 = (n1+1)%3;
                            if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
                                edgeNodes[0] = ind[n][n1];
                                edgeNodes[1] = ind[n][n2];
                                onNode = false;
                                onEdge = true;
                                onFace = false;
                                break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
                        onNode = false;
                        onEdge = false;
                        onFace = true;
                        
                        faceNodes = ind[n];
                        
                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            r_tmp.resize(1);
                            r_tmp[0] = Rx;
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on face failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
            }
            
            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                        reachedTx = true;
                        break;
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin();
                             nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                r_tmp.push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            r_tmp.push_back( Tx[nt] );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    r_tmp.push_back( Tx[nt] );
                                    reachedTx = true;
                                    break;
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
        // for inversion, the order should be from Tx to Rx, so we reorder...
        size_t npts = r_tmp.size();
        r_data.resize( npts );
        for ( size_t nn=0; nn<npts; ++nn ) {
            r_data[nn] = r_tmp[ npts-1-nn ];
        }
    }
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::getRaypath_ho(const std::vector<sxyz<T1>>& Tx,
                                             const sxyz<T1> &Rx,
                                             std::vector<sxyz<T1>> &r_data,
                                             const size_t threadNo) const {
        
        T1 minDist = small;
        std::vector<sxyz<T1>> r_tmp;
        r_tmp.emplace_back( Rx );
        
        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                return;
            }
        }
        
        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn] == Tx[nt] ) {
                    txOnNode[nt] = true;
                    txNode[nt] = nn;
                    break;
                }
            }
        }
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );
                
                // find adjacent cells
                T2 ind[6][2] = {
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][1]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][2], neighbors[txCell[nt]][3]} };
                
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
            }
        }
        
        T2 cellNo, nodeNo;
        sxyz<T1> curr_pt( Rx );
        
        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
        std::array<T2,2> edgeNodes;
        std::array<T2,3> faceNodes;
        Grad3D_ho<T1,NODE> grad3d;
        bool reachedTx = false;
        
        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == curr_pt ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );
            
            T2 ind[6][2] = {
                {neighbors[cellNo][0], neighbors[cellNo][1]},
                {neighbors[cellNo][0], neighbors[cellNo][2]},
                {neighbors[cellNo][0], neighbors[cellNo][3]},
                {neighbors[cellNo][1], neighbors[cellNo][2]},
                {neighbors[cellNo][1], neighbors[cellNo][3]},
                {neighbors[cellNo][2], neighbors[cellNo][3]} };
            
            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, ind[n][0], ind[n][1]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
            if ( !onEdge ) {
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } }
                };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                
                for ( size_t n=0; n<4; ++n ) {
                    if ( areCoplanar(curr_pt, ind[n][0], ind[n][1], ind[n][2]) ) {
                        onFace = true;
                        faceNodes[0] = ind[n][0];
                        faceNodes[1] = ind[n][1];
                        faceNodes[2] = ind[n][2];
                        break;
                    }
                }
            }
        }
        
        while ( reachedTx == false ) {
            
            if ( onNode ) {
                
                // find cells common to edge
                std::set<NODE*> nnodes;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    getNeighborNodes(*nc, nnodes);
                }
                
                // compute gradient with nodes from all common cells
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    
                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=neighbors[*nc].begin(); nn!=neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());
                    
                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    r_tmp.push_back( curr_pt );
                    
                    bool break_flag=false;
                    for ( n=0; n<3; ++n ) {
                        if ( nodes[ nb[n] ].getDistance( curr_pt ) < small ) {
                            nodeNo = nb[n];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    for ( size_t n1=0; n1<2; ++n1 ) {
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, nb[n1], nb[n2]) ) {
                            edgeNodes[0] = nb[n1];
                            edgeNodes[1] = nb[n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodes = nb;
                    
                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on node failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else if ( onEdge ) {
                
                // find cells common to edge
                std::vector<T2> cells;
                std::set<NODE*> nnodes;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                        cells.push_back( *nc0 );
                        getNeighborNodes(*nc0, nnodes);
                    }
                }
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {
                    
                    cellNo = cells[n];
                    
                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=neighbors[cellNo].begin(); nn!= neighbors[cellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }
                    
                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<4; ++n2 ) {
                        if ( nodes[ neighbors[cellNo][n2] ].getDistance( curr_pt ) < small ) {
                            nodeNo = neighbors[cellNo][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    if ( areCollinear(curr_pt, itmpNode, edgeNodes2[0]) ) {
                        edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[0];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, itmpNode, edgeNodes2[1]) ) {
                        edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, edgeNodes2[0], edgeNodes2[1]) ) {
                        edgeNodes[0] = edgeNodes2[0];
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        break_flag = true;
                        break;
                    }
                    if ( break_flag ) break;
                    
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodes[0] = itmpNode;
                    faceNodes[1] = edgeNodes2[0];
                    faceNodes[2] = edgeNodes2[1];
                    std::sort(faceNodes.begin(), faceNodes.end());
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on edge failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else { // on Face
                
                std::set<NODE*> nnodes;
                getNeighborNodes(cellNo, nnodes);
                
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } }
                };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                // there are 3 faces that might be intersected
                
                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);
                    
                    if ( !foundIntersection )
                        continue;
                    
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<3; ++n2 ) {
                        if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
                            nodeNo = ind[n][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    for ( size_t n1=0; n1<3; ++n1 ) {
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
                            edgeNodes[0] = ind[n][n1];
                            edgeNodes[1] = ind[n][n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodes = ind[n];
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    
                    // we must be on an face with gradient pointing slightly outward tetrahedron
                    // return in other cell but keep gradient
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    
                    ind[0] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } };
                    ind[1] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } };
                    ind[2] = { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } };
                    ind[3] = { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } };
                    
                    for ( size_t n=0; n<4; ++n )
                        std::sort( ind[n].begin(), ind[n].end() );
                    
                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;
                        
                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);
                        
                        if ( !foundIntersection ) {
                            continue;
                        }
                        curr_pt = pt_i;
                        r_tmp.push_back( curr_pt );
                        
                        bool break_flag = false;
                        for ( size_t n2=0; n2<3; ++n2 ) {
                            if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
                                nodeNo = ind[n][n2];
                                onNode = true;
                                onEdge = false;
                                onFace = false;
                                break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
                        for ( size_t n1=0; n1<3; ++n1 ) {
                            size_t n2 = (n1+1)%3;
                            if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
                                edgeNodes[0] = ind[n][n1];
                                edgeNodes[1] = ind[n][n2];
                                onNode = false;
                                onEdge = true;
                                onFace = false;
                                break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
                        onNode = false;
                        onEdge = false;
                        onFace = true;
                        
                        faceNodes = ind[n];
                        
                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            r_tmp.resize(1);
                            r_tmp[0] = Rx;
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on face failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
            }
            
            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                        reachedTx = true;
                        break;
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin();
                             nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                r_tmp.push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            r_tmp.push_back( Tx[nt] );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    r_tmp.push_back( Tx[nt] );
                                    reachedTx = true;
                                    break;
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
        // for inversion, the order should be from Tx to Rx, so we reorder...
        size_t npts = r_tmp.size();
        r_data.resize( npts );
        for ( size_t nn=0; nn<npts; ++nn ) {
            r_data[nn] = r_tmp[ npts-1-nn ];
        }
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::update_m_data(std::vector<sijv<T1>>& m_data,
                                             sijv<T1>& m,
                                             const std::set<T2>& allNodes,
                                             const sxyz<T1>& mid_pt,
                                             const T1 s,
                                             const T1 ds) const {
        std::vector<T1> w;
        T1 sum_w = 0.0;
        for ( auto it=allNodes.begin(); it!=allNodes.end(); ++it ) {
            w.push_back( 1./nodes[*it].getDistance( mid_pt ) );
            sum_w += w.back();
        }
        size_t nn=0;
        for ( auto it=allNodes.begin(); it!=allNodes.end(); ++it ) {
            m.j = *it;
            m.v = -s * ds * w[nn++]/sum_w;
            bool found = false;
            for ( size_t nm=0; nm<m_data.size(); ++nm ) {
                if ( m_data[nm].j == m.j ) {
                    m_data[nm].v += m.v;
                    found = true;
                    break;
                }
            }
            if ( found == false ) {
                m_data.push_back(m);
            }
        }
    }
    
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::getRaypath(const std::vector<sxyz<T1>>& Tx,
                                          const sxyz<T1> &Rx,
                                          std::vector<sxyz<T1>> &r_data,
                                          std::vector<sijv<T1>>& m_data,
                                          const size_t RxNo,
                                          const size_t threadNo) const {
        
        T1 minDist = small;
        std::vector<sxyz<T1>> r_tmp;
        r_tmp.emplace_back( Rx );
        
        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                return;
            }
        }
        
        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn] == Tx[nt] ) {
                    txOnNode[nt] = true;
                    txNode[nt] = nn;
                    break;
                }
            }
        }
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );
                
                // find adjacent cells
                T2 ind[6][2] = {
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][1]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][2], neighbors[txCell[nt]][3]} };
                
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
            }
        }
        
        T2 cellNo, nodeNo, nodeNoPrev;
        sxyz<T1> curr_pt( Rx ), mid_pt, prev_pt( Rx );
        sijv<T1> m;
        m.i = RxNo;
        
        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
		bool onNodePrev = false;
		bool onEdgePrev = false;
		bool onFacePrev = false;
        std::array<T2,2> edgeNodes, edgeNodesPrev;
        std::array<T2,3> faceNodes, faceNodesPrev;
        Grad3D<T1,NODE> grad3d;
        bool reachedTx = false;
        
        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == curr_pt ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );
            
            T2 ind[6][2] = {
                {neighbors[cellNo][0], neighbors[cellNo][1]},
                {neighbors[cellNo][0], neighbors[cellNo][2]},
                {neighbors[cellNo][0], neighbors[cellNo][3]},
                {neighbors[cellNo][1], neighbors[cellNo][2]},
                {neighbors[cellNo][1], neighbors[cellNo][3]},
                {neighbors[cellNo][2], neighbors[cellNo][3]} };
            
            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, ind[n][0], ind[n][1]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
            if ( !onEdge ) {
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } }
                };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                
                for ( size_t n=0; n<4; ++n ) {
                    if ( areCoplanar(curr_pt, ind[n][0], ind[n][1], ind[n][2]) ) {
                        onFace = true;
                        faceNodes[0] = ind[n][0];
                        faceNodes[1] = ind[n][1];
                        faceNodes[2] = ind[n][2];
                        break;
                    }
                }
            }
        }
		T1 s, ds;
        while ( reachedTx == false ) {
            
            if ( onNode ) {
                
                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    cells.push_back( *nc );
                }
                
                // compute gradient with nodes from all common cells
                std::set<NODE*> nnodes;
                for (size_t n=0; n<cells.size(); ++n ) {
                    for ( size_t no=0; no<4; ++no ) {
                        nnodes.insert( &(nodes[ neighbors[cells[n]][no] ]) );
                    }
                }
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                nodeNoPrev = nodeNo;
                onNodePrev = onNode;

                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    
                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=neighbors[*nc].begin(); nn!=neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());
                    
                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    prev_pt = curr_pt;
                    r_tmp.push_back( curr_pt );
                    
                    if ( r_tmp.size() > 1 ) {
                        // compute terms of matrix M
                        mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                        s = computeSlowness(mid_pt);
                        s *= s;
                        ds = curr_pt.getDistance( prev_pt );
					}
					
                    bool break_flag = false;
                    for ( n=0; n<3; ++n ) {
                        if ( nodes[ nb[n] ].getDistance( curr_pt ) < small ) {
							onEdgePrev = onEdge;
							onFacePrev = onFace;
							
                            nodeNo = nb[n];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
							
							if ( r_tmp.size() > 1) {
								std::set<T2> allNodes;
								allNodes.insert(nodeNoPrev);
								allNodes.insert(nodeNo);
								
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
							}

							break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
					
                    for ( size_t n1=0; n1<3; ++n1 ) {   // changed n1<2  -> n1<3
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, nb[n1], nb[n2]) ) {
							edgeNodesPrev = edgeNodes;
							onEdgePrev = onEdge;
							onFacePrev = onFace;
							
							edgeNodes[0] = nb[n1];
                            edgeNodes[1] = nb[n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
							
							if ( r_tmp.size() > 1) {
								std::set<T2> allNodes;
								allNodes.insert(nodeNoPrev);
                                allNodes.insert( edgeNodes[0] );
                                allNodes.insert( edgeNodes[1] );
																
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
							}

                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
					
					onEdgePrev = onEdge;
					onFacePrev = onFace;
					onNode = false;
                    onEdge = false;
                    onFace = true;
					
					faceNodesPrev = faceNodes;
                    faceNodes = nb;
					
					if ( r_tmp.size() > 1) {
						std::set<T2> allNodes;
						allNodes.insert(nodeNoPrev);
                        allNodes.insert( faceNodes[0] );
                        allNodes.insert( faceNodes[1] );
                        allNodes.insert( faceNodes[2] );
						
                        update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
					}

                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on node failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else if ( onEdge ) {
                
                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() )
                        cells.push_back( *nc0 );
                }
                // compute gradient with nodes from all common cells
                std::set<NODE*> nnodes;
                for (size_t n=0; n<cells.size(); ++n ) {
                    for ( size_t no=0; no<4; ++no ) {
                        nnodes.insert( &(nodes[ neighbors[cells[n]][no] ]) );
                    }
                }
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                edgeNodesPrev = edgeNodes;
                onEdgePrev = onEdge;
                
                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {
                    
                    cellNo = cells[n];
                    
                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=neighbors[cellNo].begin(); nn!= neighbors[cellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }
                    
                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    prev_pt = curr_pt;
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    if (r_tmp.size() > 1 ) {
                        // compute terms of matrix M
                        mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                        s = computeSlowness(mid_pt);
                        s *= s;
                        ds = curr_pt.getDistance( prev_pt );
					}
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<4; ++n2 ) {
                        if ( nodes[ neighbors[cellNo][n2] ].getDistance( curr_pt ) < small ) {
							onNodePrev = onNode;
							onFacePrev = onFace;

                            nodeNo = neighbors[cellNo][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
							
							if ( r_tmp.size() > 1) {
								std::set<T2> allNodes;
								allNodes.insert(nodeNo);
                                allNodes.insert( edgeNodesPrev[0] );
                                allNodes.insert( edgeNodesPrev[1] );
								
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
							}
							
                            break_flag = true;
							break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    if ( areCollinear(curr_pt, itmpNode, edgeNodes2[0]) ) {
						onNodePrev = onNode;
						onFacePrev = onFace;
						
						edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[0];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
						
						if ( r_tmp.size() > 1) {
							std::set<T2> allNodes;
                            allNodes.insert( edgeNodesPrev[0] );
                            allNodes.insert( edgeNodesPrev[1] );
                            allNodes.insert( edgeNodes[0] );
                            allNodes.insert( edgeNodes[1] );
							
                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
						}
						
						break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, itmpNode, edgeNodes2[1]) ) {
						onNodePrev = onNode;
						onFacePrev = onFace;
						
						edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
						
						if ( r_tmp.size() > 1) {
							std::set<T2> allNodes;
                            allNodes.insert( edgeNodesPrev[0] );
                            allNodes.insert( edgeNodesPrev[1] );
                            allNodes.insert( edgeNodes[0] );
                            allNodes.insert( edgeNodes[1] );
							
                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
						}
						
						break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, edgeNodes2[0], edgeNodes2[1]) ) {
						onNodePrev = onNode;
						onFacePrev = onFace;
						
						edgeNodes[0] = edgeNodes2[0];
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
						
						if ( r_tmp.size() > 1) {
							std::set<T2> allNodes;
                            allNodes.insert( edgeNodesPrev[0] );
                            allNodes.insert( edgeNodesPrev[1] );
                            allNodes.insert( edgeNodes[0] );
                            allNodes.insert( edgeNodes[1] );
							
                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
						}
						
						break_flag = true;
                        break;
                    }
                    if ( break_flag ) break;
                    
					onNodePrev = onNode;
					onFacePrev = onFace;
					onNode = false;
                    onEdge = false;
                    onFace = true;
					
					faceNodesPrev = faceNodes;
                    faceNodes[0] = itmpNode;
                    faceNodes[1] = edgeNodes2[0];
                    faceNodes[2] = edgeNodes2[1];
                    std::sort(faceNodes.begin(), faceNodes.end());
					
					if ( r_tmp.size() > 1) {
						std::set<T2> allNodes;
                        allNodes.insert( edgeNodesPrev[0] );
                        allNodes.insert( edgeNodesPrev[1] );
                        allNodes.insert( faceNodes[0] );
                        allNodes.insert( faceNodes[1] );
                        allNodes.insert( faceNodes[2] );
						
                        update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
					}
					
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on edge failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else { // on Face
                
                assert(neighbors[cellNo].size()==4);
                sxyz<T1> g = grad3d.ls_grad(nodes[ neighbors[cellNo][0] ],
                                            nodes[ neighbors[cellNo][1] ],
                                            nodes[ neighbors[cellNo][2] ],
                                            nodes[ neighbors[cellNo][3] ], threadNo);
                
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } }
                };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                // there are 3 faces that might be intersected
                
                onFacePrev = onFace;
                faceNodesPrev = faceNodes;

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);
                    
                    if ( !foundIntersection )
                        continue;
                    
                    prev_pt = curr_pt;
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    if (r_tmp.size() > 1 ) {
                        // compute terms of matrix M
                        mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                        s = computeSlowness(mid_pt);
                        s *= s;
                        ds = curr_pt.getDistance( prev_pt );
                    }
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<3; ++n2 ) {
                        if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
							nodeNoPrev = nodeNo;
							onNodePrev = onNode;
							onEdgePrev = onEdge;
							
							nodeNo = ind[n][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
							
							if ( r_tmp.size() > 1) {
								std::set<T2> allNodes;
                                allNodes.insert(nodeNo);
                                allNodes.insert( faceNodesPrev[0] );
                                allNodes.insert( faceNodesPrev[1] );
                                allNodes.insert( faceNodesPrev[2] );
								
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
							}

							break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    for ( size_t n1=0; n1<3; ++n1 ) {
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
							edgeNodesPrev = edgeNodes;
							onNodePrev = onNode;
							onEdgePrev = onEdge;
							
							edgeNodes[0] = ind[n][n1];
                            edgeNodes[1] = ind[n][n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
							
							if ( r_tmp.size() > 1) {
								std::set<T2> allNodes;
                                allNodes.insert( edgeNodes[0] );
                                allNodes.insert( edgeNodes[1] );
                                allNodes.insert( faceNodesPrev[0] );
                                allNodes.insert( faceNodesPrev[1] );
                                allNodes.insert( faceNodesPrev[2] );
								
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
							}

							break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
					onNodePrev = onNode;
					onEdgePrev = onEdge;
					onNode = false;
                    onEdge = false;
                    onFace = true;
					
                    faceNodes = ind[n];
					
					if ( r_tmp.size() > 1) {
						std::set<T2> allNodes;
                        allNodes.insert( faceNodesPrev[0] );
                        allNodes.insert( faceNodesPrev[1] );
                        allNodes.insert( faceNodesPrev[2] );
                        allNodes.insert( faceNodes[0] );
                        allNodes.insert( faceNodes[1] );
                        allNodes.insert( faceNodes[2] );
						
                        update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
					}

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    
                    // we must be on an face with gradient pointing slightly outward tetrahedron
                    // return in other cell but keep gradient
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
					
                    ind[0] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } };
                    ind[1] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } };
                    ind[2] = { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } };
                    ind[3] = { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } };
            
                    for ( size_t n=0; n<4; ++n )
                        std::sort( ind[n].begin(), ind[n].end() );
                    
                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;
                        
                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);
                        
                        if ( !foundIntersection ) {
                            continue;
                        }
                        prev_pt = curr_pt;
                        curr_pt = pt_i;
                        r_tmp.push_back( curr_pt );
                        
                        if (r_tmp.size() > 1 ) {
                            // compute terms of matrix M
                            mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                            s = computeSlowness(mid_pt);
                            s *= s;
                            ds = curr_pt.getDistance( prev_pt );
                        }
                        
                        bool break_flag = false;
                        for ( size_t n2=0; n2<3; ++n2 ) {
                            if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
								nodeNoPrev = nodeNo;
								onNodePrev = onNode;
								onEdgePrev = onEdge;
								
								nodeNo = ind[n][n2];
                                onNode = true;
                                onEdge = false;
                                onFace = false;
								
								if ( r_tmp.size() > 1) {
									std::set<T2> allNodes;
                                    allNodes.insert(nodeNo);
                                    allNodes.insert( faceNodesPrev[0] );
                                    allNodes.insert( faceNodesPrev[1] );
                                    allNodes.insert( faceNodesPrev[2] );
                                	
                                    update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
								}
								
								break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
                        for ( size_t n1=0; n1<3; ++n1 ) {
                            size_t n2 = (n1+1)%3;
                            if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
								edgeNodesPrev = edgeNodes;
								onNodePrev = onNode;
								onEdgePrev = onEdge;
								
								edgeNodes[0] = ind[n][n1];
                                edgeNodes[1] = ind[n][n2];
                                onNode = false;
                                onEdge = true;
                                onFace = false;
								
								if ( r_tmp.size() > 1) {
									std::set<T2> allNodes;
                                    allNodes.insert( edgeNodes[0] );
                                    allNodes.insert( edgeNodes[1] );
                                    allNodes.insert( faceNodesPrev[0] );
                                    allNodes.insert( faceNodesPrev[1] );
                                    allNodes.insert( faceNodesPrev[2] );
									
									update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
								}
								
								break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
						onNodePrev = onNode;
						onEdgePrev = onEdge;
						onNode = false;
                        onEdge = false;
                        onFace = true;
						
                        faceNodes = ind[n];
                        
						if ( r_tmp.size() > 1) {
							std::set<T2> allNodes;
                            allNodes.insert( faceNodesPrev[0] );
                            allNodes.insert( faceNodesPrev[1] );
                            allNodes.insert( faceNodesPrev[2] );
                            allNodes.insert( faceNodes[0] );
                            allNodes.insert( faceNodes[1] );
                            allNodes.insert( faceNodes[2] );
							
                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
						}
						
                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            r_tmp.resize(1);
                            r_tmp[0] = Rx;
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on face failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
            }
            
            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                        reachedTx = true;
                        break;
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin();
                             nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                r_tmp.push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            r_tmp.push_back( Tx[nt] );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    r_tmp.push_back( Tx[nt] );
                                    reachedTx = true;
                                    break;
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
        // for inversion, the order should be from Tx to Rx, so we reorder...
        size_t npts = r_tmp.size();
        r_data.resize( npts );
        for ( size_t nn=0; nn<npts; ++nn ) {
            r_data[nn] = r_tmp[ npts-1-nn ];
        }
    }
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::getRaypath_ho(const std::vector<sxyz<T1>>& Tx,
                                             const sxyz<T1>& Rx,
                                             std::vector<sxyz<T1>>& r_data,
                                             std::vector<sijv<T1>>& m_data,
                                             const size_t RxNo,
                                             const size_t threadNo) const {
        
        T1 minDist = small;
        std::vector<sxyz<T1>> r_tmp;
        r_tmp.emplace_back( Rx );
        
        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                return;
            }
        }
        
        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn] == Tx[nt] ) {
                    txOnNode[nt] = true;
                    txNode[nt] = nn;
                    break;
                }
            }
        }
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );
                
                // find adjacent cells
                T2 ind[6][2] = {
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][1]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][0], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][2]},
                    {neighbors[txCell[nt]][1], neighbors[txCell[nt]][3]},
                    {neighbors[txCell[nt]][2], neighbors[txCell[nt]][3]} };
                
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
            }
        }
        
        T2 cellNo, nodeNo, nodeNoPrev;
        sxyz<T1> curr_pt( Rx ), mid_pt, prev_pt( Rx );
        sijv<T1> m;
        m.i = RxNo;
        
        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
        bool onNodePrev = false;
        bool onEdgePrev = false;
        bool onFacePrev = false;
        std::array<T2,2> edgeNodes, edgeNodesPrev;
        std::array<T2,3> faceNodes, faceNodesPrev;
        Grad3D_ho<T1,NODE> grad3d;
        bool reachedTx = false;
        
        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == curr_pt ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );
            
            T2 ind[6][2] = {
                {neighbors[cellNo][0], neighbors[cellNo][1]},
                {neighbors[cellNo][0], neighbors[cellNo][2]},
                {neighbors[cellNo][0], neighbors[cellNo][3]},
                {neighbors[cellNo][1], neighbors[cellNo][2]},
                {neighbors[cellNo][1], neighbors[cellNo][3]},
                {neighbors[cellNo][2], neighbors[cellNo][3]} };
            
            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, ind[n][0], ind[n][1]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
            if ( !onEdge ) {
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3]} } };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                
                for ( size_t n=0; n<4; ++n ) {
                    if ( areCoplanar(curr_pt, ind[n][0], ind[n][1], ind[n][2]) ) {
                        onFace = true;
                        faceNodes[0] = ind[n][0];
                        faceNodes[1] = ind[n][1];
                        faceNodes[2] = ind[n][2];
                        break;
                    }
                }
            }
        }
        T1 s, ds;
        while ( reachedTx == false ) {
            
            if ( onNode ) {
                
                // find cells common to edge
                std::set<NODE*> nnodes;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    getNeighborNodes(*nc, nnodes);
                }
                
                // compute gradient with nodes from all common cells
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                nodeNoPrev = nodeNo;
                onNodePrev = onNode;
                
                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    
                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=neighbors[*nc].begin(); nn!=neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], pt_i);
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    prev_pt = curr_pt;
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    if ( r_tmp.size() > 1 ) {
                        // compute terms of matrix M
                        mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                        s = computeSlowness(mid_pt);
                        s *= s;
                        ds = curr_pt.getDistance( prev_pt );
                    }
                    
                    bool break_flag = false;
                    for ( n=0; n<3; ++n ) {
                        if ( nodes[ nb[n] ].getDistance( curr_pt ) < small ) {
                            onEdgePrev = onEdge;
                            onFacePrev = onFace;
                            
                            nodeNo = nb[n];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            
                            if ( r_tmp.size() > 1) {
                                std::set<T2> allNodes;
                                allNodes.insert(nodeNoPrev);
                                allNodes.insert(nodeNo);
                                
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                            }
                            
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    for ( size_t n1=0; n1<3; ++n1 ) {
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, nb[n1], nb[n2]) ) {
                            edgeNodesPrev = edgeNodes;
                            onEdgePrev = onEdge;
                            onFacePrev = onFace;
                            
                            edgeNodes[0] = nb[n1];
                            edgeNodes[1] = nb[n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
                            
                            if ( r_tmp.size() > 1) {
                                std::set<T2> allNodes;
                                allNodes.insert(nodeNoPrev);
                                allNodes.insert( edgeNodes[0] );
                                allNodes.insert( edgeNodes[1] );
                                
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                            }

                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    onEdgePrev = onEdge;
                    onFacePrev = onFace;
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodesPrev = faceNodes;
                    faceNodes = nb;
                    
                    if ( r_tmp.size() > 1) {
                        std::set<T2> allNodes;
                        allNodes.insert(nodeNoPrev);
                        allNodes.insert( faceNodes[0] );
                        allNodes.insert( faceNodes[1] );
                        allNodes.insert( faceNodes[2] );
                        
                        update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                    }
                    
                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on node failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else if ( onEdge ) {
                
                // find cells common to edge
                std::vector<T2> cells;
                std::set<NODE*> nnodes;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                        cells.push_back( *nc0 );
                        getNeighborNodes(*nc0, nnodes);
                    }
                }
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                edgeNodesPrev = edgeNodes;
                onEdgePrev = onEdge;
                
                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {
                    
                    cellNo = cells[n];
                    
                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=neighbors[cellNo].begin(); nn!= neighbors[cellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }
                    
                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    prev_pt = curr_pt;
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    if (r_tmp.size() > 1 ) {
                        // compute terms of matrix M
                        mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                        s = computeSlowness(mid_pt);
                        s *= s;
                        ds = curr_pt.getDistance( prev_pt );
                    }
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<4; ++n2 ) {
                        if ( nodes[ neighbors[cellNo][n2] ].getDistance( curr_pt ) < small ) {
                            onNodePrev = onNode;
                            onFacePrev = onFace;
                            
                            nodeNo = neighbors[cellNo][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            
                            if ( r_tmp.size() > 1) {
                                std::set<T2> allNodes;
                                allNodes.insert(nodeNo);
                                allNodes.insert( edgeNodesPrev[0] );
                                allNodes.insert( edgeNodesPrev[1] );
                                
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                            }
                            
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    if ( areCollinear(curr_pt, itmpNode, edgeNodes2[0]) ) {
                        onNodePrev = onNode;
                        onFacePrev = onFace;
                        
                        edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[0];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        
                        if ( r_tmp.size() > 1) {
                            std::set<T2> allNodes;
                            allNodes.insert( edgeNodesPrev[0] );
                            allNodes.insert( edgeNodesPrev[1] );
                            allNodes.insert( edgeNodes[0] );
                            allNodes.insert( edgeNodes[1] );
                            
                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                        }
                        
                        break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, itmpNode, edgeNodes2[1]) ) {
                        onNodePrev = onNode;
                        onFacePrev = onFace;
                        
                        edgeNodes[0] = itmpNode;
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        
                        if ( r_tmp.size() > 1) {
                            std::set<T2> allNodes;
                            allNodes.insert( edgeNodesPrev[0] );
                            allNodes.insert( edgeNodesPrev[1] );
                            allNodes.insert( edgeNodes[0] );
                            allNodes.insert( edgeNodes[1] );
                            
                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                        }
                        
                        break_flag = true;
                        break;
                    } else if ( areCollinear(curr_pt, edgeNodes2[0], edgeNodes2[1]) ) {
                        onNodePrev = onNode;
                        onFacePrev = onFace;
                        
                        edgeNodes[0] = edgeNodes2[0];
                        edgeNodes[1] = edgeNodes2[1];
                        onNode = false;
                        onEdge = true;
                        onFace = false;
                        
                        if ( r_tmp.size() > 1) {
                            std::set<T2> allNodes;
                            allNodes.insert( edgeNodesPrev[0] );
                            allNodes.insert( edgeNodesPrev[1] );
                            allNodes.insert( edgeNodes[0] );
                            allNodes.insert( edgeNodes[1] );

                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                        }
                        
                        break_flag = true;
                        break;
                    }
                    if ( break_flag ) break;
                    
                    onNodePrev = onNode;
                    onFacePrev = onFace;
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodesPrev = faceNodes;
                    faceNodes[0] = itmpNode;
                    faceNodes[1] = edgeNodes2[0];
                    faceNodes[2] = edgeNodes2[1];
                    std::sort(faceNodes.begin(), faceNodes.end());
                    
                    if ( r_tmp.size() > 1) {
                        std::set<T2> allNodes;
                        allNodes.insert( edgeNodesPrev[0] );
                        allNodes.insert( edgeNodesPrev[1] );
                        allNodes.insert( faceNodes[0] );
                        allNodes.insert( faceNodes[1] );
                        allNodes.insert( faceNodes[2] );
                        
                        update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                    }
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on edge failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
                
            } else { // on Face
                
                std::set<NODE*> nnodes;
                getNeighborNodes(cellNo, nnodes);
                
                sxyz<T1> g = grad3d.ls_grad(nnodes, threadNo);
                
                std::array<T2,3> ind[4] = {
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } },
                    { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } }
                };
                for ( size_t n=0; n<4; ++n )
                    std::sort( ind[n].begin(), ind[n].end() );
                // there are 3 faces that might be intersected
                
                onFacePrev = onFace;
                faceNodesPrev = faceNodes;

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);
                    
                    if ( !foundIntersection )
                        continue;
                    
                    prev_pt = curr_pt;
                    curr_pt = pt_i;
                    r_tmp.push_back( curr_pt );
                    
                    if (r_tmp.size() > 1 ) {
                        // compute terms of matrix M
                        mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                        s = computeSlowness(mid_pt);
                        s *= s;
                        ds = curr_pt.getDistance( prev_pt );
                    }
                    
                    bool break_flag = false;
                    for ( size_t n2=0; n2<3; ++n2 ) {
                        if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
                            nodeNoPrev = nodeNo;
                            onNodePrev = onNode;
                            onEdgePrev = onEdge;
                            
                            nodeNo = ind[n][n2];
                            onNode = true;
                            onEdge = false;
                            onFace = false;
                            
                            if ( r_tmp.size() > 1) {
                                std::set<T2> allNodes;
                                allNodes.insert(nodeNo);
                                allNodes.insert( faceNodesPrev[0] );
                                allNodes.insert( faceNodesPrev[1] );
                                allNodes.insert( faceNodesPrev[2] );
                                
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                            }
                            
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    for ( size_t n1=0; n1<3; ++n1 ) {
                        size_t n2 = (n1+1)%3;
                        if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
                            edgeNodesPrev = edgeNodes;
                            onNodePrev = onNode;
                            onEdgePrev = onEdge;
                            
                            edgeNodes[0] = ind[n][n1];
                            edgeNodes[1] = ind[n][n2];
                            onNode = false;
                            onEdge = true;
                            onFace = false;
                            
                            if ( r_tmp.size() > 1) {
                                std::set<T2> allNodes;
                                allNodes.insert( edgeNodes[0] );
                                allNodes.insert( edgeNodes[1] );
                                allNodes.insert( faceNodesPrev[0] );
                                allNodes.insert( faceNodesPrev[1] );
                                allNodes.insert( faceNodesPrev[2] );
                                
                                update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                            }
                            
                            break_flag = true;
                            break;
                        }
                    }
                    if ( break_flag ) break;
                    
                    onNodePrev = onNode;
                    onEdgePrev = onEdge;
                    onNode = false;
                    onEdge = false;
                    onFace = true;
                    
                    faceNodes = ind[n];
                    
                    if ( r_tmp.size() > 1) {
                        std::set<T2> allNodes;
                        allNodes.insert( faceNodesPrev[0] );
                        allNodes.insert( faceNodesPrev[1] );
                        allNodes.insert( faceNodesPrev[2] );
                        allNodes.insert( faceNodes[0] );
                        allNodes.insert( faceNodes[1] );
                        allNodes.insert( faceNodes[2] );
                        
                        update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                    }
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_tmp.resize(1);
                        r_tmp[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    
                    // we must be on an face with gradient pointing slightly outward tetrahedron
                    // return in other cell but keep gradient
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    
                    ind[0] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][2] } };
                    ind[1] = { { neighbors[cellNo][0], neighbors[cellNo][1], neighbors[cellNo][3] } };
                    ind[2] = { { neighbors[cellNo][0], neighbors[cellNo][2], neighbors[cellNo][3] } };
                    ind[3] = { { neighbors[cellNo][1], neighbors[cellNo][2], neighbors[cellNo][3] } };
                    
                    for ( size_t n=0; n<4; ++n )
                        std::sort( ind[n].begin(), ind[n].end() );
                    
                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;
                        
                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);
                        
                        if ( !foundIntersection ) {
                            continue;
                        }
                        prev_pt = curr_pt;
                        curr_pt = pt_i;
                        r_tmp.push_back( curr_pt );
                        
                        if (r_tmp.size() > 1 ) {
                            // compute terms of matrix M
                            mid_pt = static_cast<T1>(0.5)*(curr_pt + prev_pt);
                            s = computeSlowness(mid_pt);
                            s *= s;
                            ds = curr_pt.getDistance( prev_pt );
                        }
                        
                        bool break_flag = false;
                        for ( size_t n2=0; n2<3; ++n2 ) {
                            if ( nodes[ ind[n][n2] ].getDistance( curr_pt ) < small ) {
                                nodeNoPrev = nodeNo;
                                onNodePrev = onNode;
                                onEdgePrev = onEdge;
                                
                                nodeNo = ind[n][n2];
                                onNode = true;
                                onEdge = false;
                                onFace = false;
                                
                                if ( r_tmp.size() > 1) {
                                    std::set<T2> allNodes;
                                    allNodes.insert(nodeNo);
                                    allNodes.insert( faceNodesPrev[0] );
                                    allNodes.insert( faceNodesPrev[1] );
                                    allNodes.insert( faceNodesPrev[2] );
                                    
                                    update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                                }
                                
                                break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
                        for ( size_t n1=0; n1<3; ++n1 ) {
                            size_t n2 = (n1+1)%3;
                            if ( areCollinear(curr_pt, ind[n][n1], ind[n][n2]) ) {
                                edgeNodesPrev = edgeNodes;
                                onNodePrev = onNode;
                                onEdgePrev = onEdge;
                                
                                edgeNodes[0] = ind[n][n1];
                                edgeNodes[1] = ind[n][n2];
                                onNode = false;
                                onEdge = true;
                                onFace = false;
                                
                                if ( r_tmp.size() > 1) {
                                    std::set<T2> allNodes;
                                    allNodes.insert( edgeNodes[0] );
                                    allNodes.insert( edgeNodes[1] );
                                    allNodes.insert( faceNodesPrev[0] );
                                    allNodes.insert( faceNodesPrev[1] );
                                    allNodes.insert( faceNodesPrev[2] );
                                    
                                    update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                                }
                                
                                break_flag = true;
                                break;
                            }
                        }
                        if ( break_flag ) break;
                        
                        onNodePrev = onNode;
                        onEdgePrev = onEdge;
                        onNode = false;
                        onEdge = false;
                        onFace = true;
                        
                        faceNodes = ind[n];
                        
                        if ( r_tmp.size() > 1) {
                            std::set<T2> allNodes;
                            allNodes.insert( faceNodesPrev[0] );
                            allNodes.insert( faceNodesPrev[1] );
                            allNodes.insert( faceNodesPrev[2] );
                            allNodes.insert( faceNodes[0] );
                            allNodes.insert( faceNodes[1] );
                            allNodes.insert( faceNodes[2] );
                            
                            update_m_data(m_data, m, allNodes, mid_pt, s,  ds);
                        }
                        
                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            r_tmp.resize(1);
                            r_tmp[0] = Rx;
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath on face failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_tmp.resize(1);
                    r_tmp[0] = Rx;
                    reachedTx = true;
                }
            }
            
            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                        reachedTx = true;
                        break;
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin();
                             nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                r_tmp.push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            r_tmp.push_back( Tx[nt] );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    r_tmp.push_back( Tx[nt] );
                                    reachedTx = true;
                                    break;
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
        // for inversion, the order should be from Tx to Rx, so we reorder...
        size_t npts = r_tmp.size();
        r_data.resize( npts );
        for ( size_t nn=0; nn<npts; ++nn ) {
            r_data[nn] = r_tmp[ npts-1-nn ];
        }
    }
    
    template<typename T1, typename T2, typename NODE>
    bool Grid3Dun<T1,T2,NODE>::intersectVecTriangle(const T2 iO, const sxyz<T1> &vec,
                                                    const T2 iA, T2 iB, T2 iC,
                                                    sxyz<T1> &pt_i) const {
        
        sxyz<T1> OA = {nodes[iA].getX()-nodes[iO].getX(), nodes[iA].getY()-nodes[iO].getY(), nodes[iA].getZ()-nodes[iO].getZ()};
        // check if counterclockwise
        sxyz<T1> AB = {nodes[iB].getX()-nodes[iA].getX(),
            nodes[iB].getY()-nodes[iA].getY(),
            nodes[iB].getZ()-nodes[iA].getZ()};
        sxyz<T1> AC = {nodes[iC].getX()-nodes[iA].getX(),
            nodes[iC].getY()-nodes[iA].getY(),
            nodes[iC].getZ()-nodes[iA].getZ()};
        sxyz<T1> n = cross(AB, AC);
        if ( dot(OA, n) > 0. ) std::swap(iB, iC);
        
        sxyz<T1> OB = {nodes[iB].getX()-nodes[iO].getX(), nodes[iB].getY()-nodes[iO].getY(), nodes[iB].getZ()-nodes[iO].getZ()};
        sxyz<T1> OC = {nodes[iC].getX()-nodes[iO].getX(), nodes[iC].getY()-nodes[iO].getY(), nodes[iC].getZ()-nodes[iO].getZ()};
        
        T1 u, v, w;
        u = tripleScalar(vec, OC, OB);
        if ( u<0.0 ) return false;
        v = tripleScalar(vec, OA, OC);
        if ( v<0.0 ) return false;
        w = tripleScalar(vec, OB, OA);
        if ( w<0.0 ) return false;
        
        T1 den = 1./(u+v+w);
        u *= den;
        v *= den;
        w *= den;
        
        pt_i.x = u*nodes[iA].getX() + v*nodes[iB].getX() + w*nodes[iC].getX();
        pt_i.y = u*nodes[iA].getY() + v*nodes[iB].getY() + w*nodes[iC].getY();
        pt_i.z = u*nodes[iA].getZ() + v*nodes[iB].getZ() + w*nodes[iC].getZ();
        
        return true;
    }
    
    template<typename T1, typename T2, typename NODE>
    bool Grid3Dun<T1,T2,NODE>::intersectVecTriangle(const sxyz<T1> &O, const sxyz<T1> &vec,
                                                    const T2 iA, T2 iB, T2 iC,
                                                    sxyz<T1> &pt_i) const {
        
        sxyz<T1> OA = {nodes[iA].getX()-O.x, nodes[iA].getY()-O.y, nodes[iA].getZ()-O.z};
        // check if counterclockwise
        sxyz<T1> AB = {nodes[iB].getX()-nodes[iA].getX(),
            nodes[iB].getY()-nodes[iA].getY(),
            nodes[iB].getZ()-nodes[iA].getZ()};
        sxyz<T1> AC = {nodes[iC].getX()-nodes[iA].getX(),
            nodes[iC].getY()-nodes[iA].getY(),
            nodes[iC].getZ()-nodes[iA].getZ()};
        sxyz<T1> n = cross(AB, AC);
        if ( dot(OA, n) > 0. ) std::swap(iB, iC);
        
        sxyz<T1> OB = {nodes[iB].getX()-O.x, nodes[iB].getY()-O.y, nodes[iB].getZ()-O.z};
        sxyz<T1> OC = {nodes[iC].getX()-O.x, nodes[iC].getY()-O.y, nodes[iC].getZ()-O.z};
        
        T1 u, v, w;
        u = tripleScalar(vec, OC, OB);
        if ( u<0.0 ) return false;
        v = tripleScalar(vec, OA, OC);
        if ( v<0.0 ) return false;
        w = tripleScalar(vec, OB, OA);
        if ( w<0.0 ) return false;
        
        T1 den = 1./(u+v+w);
        u *= den;
        v *= den;
        w *= den;
        
        pt_i.x = u*nodes[iA].getX() + v*nodes[iB].getX() + w*nodes[iC].getX();
        pt_i.y = u*nodes[iA].getY() + v*nodes[iB].getY() + w*nodes[iC].getY();
        pt_i.z = u*nodes[iA].getZ() + v*nodes[iB].getZ() + w*nodes[iC].getZ();
        
        return true;
    }
    
    
    template<typename T1, typename T2, typename NODE>
    bool Grid3Dun<T1,T2,NODE>::areCollinear(const sxyz<T1> &pt, const T2 i0, const T2 i1) const {
        
        // http://mathworld.wolfram.com/Collinear.html
        //
        sxyz<T1> v1 = {pt.x-nodes[i0].getX(), pt.y-nodes[i0].getY(), pt.z-nodes[i0].getZ()};
        sxyz<T1> v2 = {pt.x-nodes[i1].getX(), pt.y-nodes[i1].getY(), pt.z-nodes[i1].getZ()};
        sxyz<T1> v3 = cross(v1, v2);
        return norm(v3)<small;
        
    }
    
    template<typename T1, typename T2, typename NODE>
    bool Grid3Dun<T1,T2,NODE>::areCoplanar(const sxyz<T1> &x1, const T2 i0, const T2 i1, const T2 i2) const {
        
        // http://mathworld.wolfram.com/Coplanar.html
        //
        sxyz<T1> x2 = {nodes[i0].getX(), nodes[i0].getY(), nodes[i0].getZ()};
        sxyz<T1> x3 = {nodes[i1].getX(), nodes[i1].getY(), nodes[i1].getZ()};
        sxyz<T1> x4 = {nodes[i2].getX(), nodes[i2].getY(), nodes[i2].getZ()};
        
        return fabs( dot( x3-x1, cross(x2-x1, x4-x3) ) )<small;
    }
    
    template<typename T1, typename T2, typename NODE>
    T2 Grid3Dun<T1,T2,NODE>::findAdjacentCell1(const std::array<T2,3> &faceNodes,
                                               const T2 nodeNo) const {
        
        std::vector<T2> cells;
        for ( auto nc0=nodes[faceNodes[0]].getOwners().begin(); nc0!=nodes[faceNodes[0]].getOwners().end(); ++nc0 ) {
            if ( std::find(nodes[faceNodes[1]].getOwners().begin(), nodes[faceNodes[1]].getOwners().end(), *nc0)!=nodes[faceNodes[1]].getOwners().end() &&
                std::find(nodes[faceNodes[2]].getOwners().begin(), nodes[faceNodes[2]].getOwners().end(), *nc0)!=nodes[faceNodes[2]].getOwners().end() )
                cells.push_back( *nc0 );
        }
        if ( cells.size() == 1 ) {
            return cells[0];
        }
        for ( auto nc0=nodes[nodeNo].getOwners().begin(); nc0!=nodes[nodeNo].getOwners().end(); ++nc0 ) {
            if ( *nc0 == cells[0] ) {
                return cells[1];
            } else if ( *nc0 == cells[1] ) {
                return cells[0];
            }
        }
        return std::numeric_limits<T2>::max();
    }
    
    template<typename T1, typename T2, typename NODE>
    T2 Grid3Dun<T1,T2,NODE>::findAdjacentCell2(const std::array<T2,3> &faceNodes,
                                               const T2 cellNo) const {
        
        std::vector<T2> cells;
        for ( auto nc0=nodes[faceNodes[0]].getOwners().begin(); nc0!=nodes[faceNodes[0]].getOwners().end(); ++nc0 ) {
            if ( std::find(nodes[faceNodes[1]].getOwners().begin(), nodes[faceNodes[1]].getOwners().end(), *nc0)!=nodes[faceNodes[1]].getOwners().end() &&
                std::find(nodes[faceNodes[2]].getOwners().begin(), nodes[faceNodes[2]].getOwners().end(), *nc0)!=nodes[faceNodes[2]].getOwners().end() )
                cells.push_back( *nc0 );
        }
        if ( cells.size() == 1 ) {
            return cells[0];
        }
        if ( cellNo == cells[0] ) {
            return cells[1];
        } else if ( cellNo == cells[1] ) {
            return cells[0];
        }
        return std::numeric_limits<T2>::max();
    }
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::plotCell(const T2 cellNo, const sxyz<T1> &pt, const sxyz<T1> &g) const {
        
        
        if ( true ) {
            T2 i0 = neighbors[cellNo][0];
            T2 i1 = neighbors[cellNo][1];
            T2 i2 = neighbors[cellNo][2];
            T2 i3 = neighbors[cellNo][3];
            
            std::cout << "\nplot3(["<<nodes[ i0 ].getX()<<' ' << nodes[ i1 ].getX() <<"],["
            <<nodes[ i0 ].getY()<<' ' << nodes[ i1 ].getY() <<"],["
            <<nodes[ i0 ].getZ()<<' ' << nodes[ i1 ].getZ() <<"]); hold on;\n";
            std::cout << "plot3(["<<nodes[ i0 ].getX()<<' ' << nodes[ i2 ].getX() <<"],["
            <<nodes[ i0 ].getY()<<' ' << nodes[ i2 ].getY() <<"],["
            <<nodes[ i0 ].getZ()<<' ' << nodes[ i2 ].getZ() <<"])\n";
            std::cout << "plot3(["<<nodes[ i0 ].getX()<<' ' << nodes[ i3 ].getX() <<"],["
            <<nodes[ i0 ].getY()<<' ' << nodes[ i3 ].getY() <<"],["
            <<nodes[ i0 ].getZ()<<' ' << nodes[ i3 ].getZ() <<"])\n";
            std::cout << "plot3(["<<nodes[ i1 ].getX()<<' '<<nodes[ i2 ].getX()<<' '<<nodes[ i3 ].getX()<<' '<<nodes[ i1 ].getX()<<"],["
            <<nodes[ i1 ].getY()<<' '<<nodes[ i2 ].getY()<<' '<<nodes[ i3 ].getY()<<' '<<nodes[ i1 ].getY()<<"],["
            <<nodes[ i1 ].getZ()<<' '<<nodes[ i2 ].getZ()<<' '<<nodes[ i3 ].getZ()<<' '<<nodes[ i1 ].getZ()<<"])\n";
            std::cout << "plot3(["<<pt.x<< ' ' << pt.x+g.x<<"],["<<pt.y<< ' ' << pt.y+g.y<<"],["<<pt.z<< ' ' << pt.z+g.z<<"],'r')\naxis equal\n\n";
        }
    }
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::getNeighborNodes(const T2 cellNo,
                                                std::set<NODE*> &nnodes) const {
        
        for ( size_t n=0; n<4; ++n ) {
            T2 nodeNo = neighbors[cellNo][n];
            nnodes.insert( &(nodes[nodeNo]) );
            
            for ( auto nc=nodes[nodeNo].getOwners().cbegin(); nc!=nodes[nodeNo].getOwners().cend(); ++nc ) {
                for ( size_t nn=0; nn<4; ++nn ) {
                    nnodes.insert( &(nodes[ neighbors[*nc][nn] ]) );
                }
            }
        }
    }
    
    template<typename T1, typename T2, typename NODE>
    bool Grid3Dun<T1,T2,NODE>::testInTriangle(const NODE *vertexA,
                                              const NODE *vertexB,
                                              const NODE *vertexC,
                                              const sxyz<T1> &E) const {
        
        T1 u, v, w;
        barycentric(vertexA, vertexB, vertexC, E, u, v, w);
        return v >= 0.0 && w >= 0.0 && (v + w) <= 1.0;
    }
    
    template<typename T1, typename T2, typename NODE>
    void Grid3Dun<T1,T2,NODE>::barycentric(const NODE *a,
                                           const NODE *b,
                                           const NODE *c,
                                           const sxyz<T1> &p,
                                           T1 &u, T1 &v, T1 &w) const {
        
        sxyz<T1> ab = {b->getX()-a->getX(), b->getY()-a->getY(), b->getZ()-a->getZ()};
        sxyz<T1> ac = {c->getX()-a->getX(), c->getY()-a->getY(), c->getZ()-a->getZ()};
        
        // Unnormalized triangle normal
        sxyz<T1> m = cross(ab, ac);
        
        // Nominators and one-over-denominator for u and v ratios
        T1 nu, nv, ood;
        
        // Absolute components for determining projection plane
        T1 x = fabs(m.x), y = fabs(m.y), z = fabs(m.z);
        
        // Compute areas in plane of largest projection
        if (x >= y && x >= z) {
            // x is largest, project to the yz plane
            nu = triangleArea2D(p.y, p.z, b->getY(), b->getZ(), c->getY(), c->getZ()); // Area of PBC in yz plane
            nv = triangleArea2D(p.y, p.z, c->getY(), c->getZ(), a->getY(), a->getZ()); // Area of PCA in yz plane
            ood = 1.0 / m.x; // 1/(2*area of ABC in yz plane)
        } else if (y >= x && y >= z) {
            // y is largest, project to the xz plane
            nu = triangleArea2D(p.x, p.z, b->getX(), b->getZ(), c->getX(), c->getZ());
            nv = triangleArea2D(p.x, p.z, c->getX(), c->getZ(), a->getX(), a->getZ());
            ood = 1.0 / -m.y;
        } else {
            // z is largest, project to the xy plane
            nu = triangleArea2D(p.x, p.y, b->getX(), b->getY(), c->getX(), c->getY());
            nv = triangleArea2D(p.x, p.y, c->getX(), c->getY(), a->getX(), a->getY());
            ood = 1.0 / m.z;
        }
        u = nu * ood;
        v = nv * ood;
        w = 1.0 - u - v;
    }
    
    
    template<typename T1, typename T2, typename NODE>
    T1 Grid3Dun<T1,T2,NODE>::computeSlowness( const sxyz<T1>& Rx ) const {
        
        //Calculate the slowness of any point that is not on a node
        
        T2 cellNo = this->getCellNo( Rx );
        
        //We calculate the Slowness at the point
        std::vector<T2> list;
        
        for (size_t n3=0; n3 < neighbors[ cellNo ].size(); n3++){
            if ( nodes[neighbors[ cellNo ][n3] ].getPrimary() == 5 ){
                list.push_back(neighbors[ cellNo ][n3]);
            }
        }
        
        std::vector<size_t>::iterator it;
        
        std::vector<NODE*> interpNodes;
        
        for ( size_t nn=0; nn<list.size(); ++nn )
            interpNodes.push_back( &(nodes[list[nn] ]) );
        
        return Interpolator<T1>::inverseDistance( Rx, interpNodes );
        
    }
    
}

#endif
