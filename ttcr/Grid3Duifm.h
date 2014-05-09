//
//  Grid3Duifm.h
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

#ifndef ttcr_Grid3Duifm_h
#define ttcr_Grid3Duifm_h

#include <cmath>
#include <fstream>
#include <queue>
#include <vector>

#include "Grid3Dui.h"
#include "Node3Di.h"

template<typename T1, typename T2>
class Grid3Duifm : public Grid3Dui<T1,T2,Node3Di<T1,T2>> {
public:
	Grid3Duifm(const std::vector<sxyz<T1>>& no,
			   const std::vector<tetrahedronElem<T2>>& tet,
			   const size_t nt=1) :
    Grid3Dui<T1,T2,Node3Di<T1,T2>>(no, tet, nt)
	{
		buildGridNodes(no, nt);
		this->buildGridNeighbors();
	}
	
	~Grid3Duifm() {
	}
	
	int raytrace(const std::vector<sxyz<T1>>& Tx,
				 const std::vector<T1>& t0,
				 const std::vector<sxyz<T1>>& Rx,
				 std::vector<T1>& traveltimes,
				 const size_t threadNo=0) const;
	
	int raytrace(const std::vector<sxyz<T1>>&,
				 const std::vector<T1>&,
				 const std::vector<const std::vector<sxyz<T1>>*>&,
				 std::vector<std::vector<T1>*>&,
				 const size_t=0) const;
    
	int raytrace(const std::vector<sxyz<T1>>&,
                 const std::vector<T1>& ,
                 const std::vector<sxyz<T1>>&,
                 std::vector<T1>&,
                 std::vector<std::vector<sxyz<T1>>>&,
				 const size_t=0) const;
    
    int raytrace(const std::vector<sxyz<T1>>&,
                 const std::vector<T1>&,
                 const std::vector<const std::vector<sxyz<T1>>*>&,
                 std::vector<std::vector<T1>*>&,
                 std::vector<std::vector<std::vector<sxyz<T1>>>*>&,
                 const size_t=0) const;
	
private:
	void buildGridNodes(const std::vector<sxyz<T1>>&, const size_t);
	
	void initBand(const std::vector<sxyz<T1>>& Tx,
				  const std::vector<T1>& t0,
				  std::priority_queue<Node3Di<T1,T2>*,
				  std::vector<Node3Di<T1,T2>*>,
				  CompareNodePtr<T1>>&,
				  std::vector<bool>&,
				  std::vector<bool>&,
				  const size_t) const;
	
	void propagate(std::priority_queue<Node3Di<T1,T2>*,
				   std::vector<Node3Di<T1,T2>*>,
				   CompareNodePtr<T1>>&,
				   std::vector<bool>&,
				   std::vector<bool>&,
				   const size_t) const;
	
};

template<typename T1, typename T2>
void Grid3Duifm<T1,T2>::buildGridNodes(const std::vector<sxyz<T1>>& no,
									   const size_t nt) {
	
	// primary nodes
	for ( T2 n=0; n<no.size(); ++n ) {
		this->nodes[n].setXYZindex( no[n].x, no[n].y, no[n].z, n );
	}
	
	//
	//              1
	//            ,/|`\
	//          ,/  |  `\
	//        ,0    '.   `4
	//      ,/       1     `\
	//    ,/         |       `\
	//   0-----5-----'.--------3
	//    `\.         |      ,/
	//       `\.      |     3
	//          `2.   '. ,/
	//             `\. |/
	//                `2
	//
	//
	//  triangle 0:  0-1  1-2  2-0     (first occurence of segment underlined)
	//               ---  ---  ---
	//  triangle 1:  1-2  2-3  3-1
	//                    ---  ---
	//  triangle 2:  0-2  2-3  3-0
	//                         ---
	//  triangle 3:  0-1  1-3  3-0
	
	
	for ( T2 ntet=0; ntet<this->tetrahedra.size(); ++ntet ) {
		
		// for each triangle
		for ( T2 ntri=0; ntri<4; ++ntri ) {
			
			// push owner for primary nodes
			this->nodes[ this->tetrahedra[ntet].i[ntri] ].pushOwner( ntet );
		}
	}
}

template<typename T1, typename T2>
int Grid3Duifm<T1,T2>::raytrace(const std::vector<sxyz<T1>>& Tx,
								const std::vector<T1>& t0,
								const std::vector<sxyz<T1>>& Rx,
								std::vector<T1>& traveltimes,
								const size_t threadNo) const {
	
	if ( this->check_pts(Tx) == 1 ) return 1;
    if ( this->check_pts(Rx) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< Node3Di<T1,T2>*, std::vector<Node3Di<T1,T2>*>,
    CompareNodePtr<T1>> narrow_band( cmp );
	
    std::vector<bool> inQueue( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, inQueue, frozen, threadNo);
    
    propagate(narrow_band, inQueue, frozen, threadNo);
    
    if ( traveltimes.size() != Rx.size() ) {
        traveltimes.resize( Rx.size() );
    }
    
    for (size_t n=0; n<Rx.size(); ++n) {
        traveltimes[n] = this->getTraveltime(Rx[n], this->nodes, threadNo);
    }
	return 0;
}

template<typename T1, typename T2>
int Grid3Duifm<T1,T2>::raytrace(const std::vector<sxyz<T1>>& Tx,
								const std::vector<T1>& t0,
								const std::vector<const std::vector<sxyz<T1>>*>& Rx,
								std::vector<std::vector<T1>*>& traveltimes,
								const size_t threadNo) const {
    
    if ( this->check_pts(Tx) == 1 ) return 1;
    for ( size_t n=0; n<Rx.size(); ++n )
        if ( this->check_pts(*Rx[n]) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< Node3Di<T1,T2>*, std::vector<Node3Di<T1,T2>*>,
    CompareNodePtr<T1>> narrow_band( cmp );
    
    std::vector<bool> inBand( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, inBand, frozen, threadNo);
    
    propagate(narrow_band, inBand, frozen, threadNo);
    
    if ( traveltimes.size() != Rx.size() ) {
        traveltimes.resize( Rx.size() );
    }
    
    for (size_t nr=0; nr<Rx.size(); ++nr) {
        traveltimes[nr]->resize( Rx[nr]->size() );
        for (size_t n=0; n<Rx[nr]->size(); ++n)
            (*traveltimes[nr])[n] = this->getTraveltime((*Rx[nr])[n], this->nodes, threadNo);
    }
    return 0;
}

template<typename T1, typename T2>
int Grid3Duifm<T1,T2>::raytrace(const std::vector<sxyz<T1>>& Tx,
                                const std::vector<T1>& t0,
                                const std::vector<sxyz<T1>>& Rx,
                                std::vector<T1>& traveltimes,
                                std::vector<std::vector<sxyz<T1>>>& r_data,
                                const size_t threadNo) const {
    
	if ( this->check_pts(Tx) == 1 ) return 1;
    if ( this->check_pts(Rx) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< Node3Di<T1,T2>*, std::vector<Node3Di<T1,T2>*>,
    CompareNodePtr<T1>> narrow_band( cmp );
	
    std::vector<bool> inQueue( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, inQueue, frozen, threadNo);
    
    propagate(narrow_band, inQueue, frozen, threadNo);
    
    if ( traveltimes.size() != Rx.size() ) {
        traveltimes.resize( Rx.size() );
    }
    if ( r_data.size() != Rx.size() ) {
        r_data.resize( Rx.size() );
    }
    for ( size_t ni=0; ni<r_data.size(); ++ni ) {
        r_data[ni].resize( 0 );
    }
    
    for (size_t n=0; n<Rx.size(); ++n) {
        traveltimes[n] = this->getTraveltime(Rx[n], this->nodes, threadNo);
        
        this->getRaypath_ho(Tx, Rx[n], traveltimes[n], r_data[n], threadNo);
        
    }
    
    return 0;
}


template<typename T1, typename T2>
int Grid3Duifm<T1,T2>::raytrace(const std::vector<sxyz<T1>>& Tx,
                                const std::vector<T1>& t0,
                                const std::vector<const std::vector<sxyz<T1>>*>& Rx,
                                std::vector<std::vector<T1>*>& traveltimes,
                                std::vector<std::vector<std::vector<sxyz<T1>>>*>& r_data,
                                const size_t threadNo) const {
    
    if ( this->check_pts(Tx) == 1 ) return 1;
    for ( size_t n=0; n<Rx.size(); ++n )
        if ( this->check_pts(*Rx[n]) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< Node3Di<T1,T2>*, std::vector<Node3Di<T1,T2>*>,
    CompareNodePtr<T1>> narrow_band( cmp );
    
    std::vector<bool> inBand( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, inBand, frozen, threadNo);
    
    propagate(narrow_band, inBand, frozen, threadNo);
    
	//    for ( size_t n=0; n<this->nodes.size(); ++n ) {
	//        T1 t = this->slowness[0] * this->nodes[n].getDistance( Tx[0] );
	//        this->nodes[n].setTT(t, threadNo);
	//    }
    
    if ( traveltimes.size() != Rx.size() ) {
        traveltimes.resize( Rx.size() );
    }
    if ( r_data.size() != Rx.size() ) {
        r_data.resize( Rx.size() );
    }
    
    for (size_t nr=0; nr<Rx.size(); ++nr) {
        traveltimes[nr]->resize( Rx[nr]->size() );
        r_data[nr]->resize( Rx[nr]->size() );
        for ( size_t ni=0; ni<r_data[nr]->size(); ++ni ) {
            (*r_data[nr])[ni].resize( 0 );
        }
        
        for (size_t n=0; n<Rx[nr]->size(); ++n) {
            (*traveltimes[nr])[n] = this->getTraveltime((*Rx[nr])[n], this->nodes, threadNo);
            
            this->getRaypath_ho(Tx, (*Rx[nr])[n], (*traveltimes[nr])[n], (*r_data[nr])[n], threadNo);
        }
    }
    return 0;
}

template<typename T1, typename T2>
void Grid3Duifm<T1,T2>::initBand(const std::vector<sxyz<T1>>& Tx,
								 const std::vector<T1>& t0,
								 std::priority_queue<Node3Di<T1,T2>*,
								 std::vector<Node3Di<T1,T2>*>,
								 CompareNodePtr<T1>>& narrow_band,
								 std::vector<bool>& inBand,
								 std::vector<bool>& frozen,
								 const size_t threadNo) const {
    
    for (size_t n=0; n<Tx.size(); ++n) {
        bool found = false;
        for ( size_t nn=0; nn<this->nodes.size(); ++nn ) {
            if ( this->nodes[nn] == Tx[n] ) {
                found = true;
                this->nodes[nn].setTT( t0[n], threadNo );
                narrow_band.push( &(this->nodes[nn]) );
                inBand[nn] = true;
                frozen[nn] = true;
				
				if ( Tx.size()==1 ) {
					// populate around Tx
					for ( size_t no=0; no<this->nodes[nn].getOwners().size(); ++no ) {
						
						T2 cellNo = this->nodes[nn].getOwners()[no];
						for ( size_t k=0; k< this->neighbors[cellNo].size(); ++k ) {
							T2 neibNo = this->neighbors[cellNo][k];
							if ( neibNo == nn ) continue;
							T1 dt = this->computeDt(this->nodes[nn], this->nodes[neibNo]);
							
							if ( t0[n]+dt < this->nodes[neibNo].getTT(threadNo) ) {
								this->nodes[neibNo].setTT( t0[n]+dt, threadNo );
								
								if ( !inBand[neibNo] ) {
									narrow_band.push( &(this->nodes[neibNo]) );
									inBand[neibNo] = true;
									frozen[neibNo] = true;
								}
							}
						}
					}
				}
				
                break;
            }
        }
        if ( found==false ) {
			
			T2 cellNo = this->getCellNo(Tx[n]);
			for ( size_t k=0; k< this->neighbors[cellNo].size(); ++k ) {
                T2 neibNo = this->neighbors[cellNo][k];
				
				// compute dt
				T1 dt = this->nodes[neibNo].getDistance(Tx[n])*this->nodes[neibNo].getNodeSlowness();
				
				this->nodes[neibNo].setTT( t0[n]+dt, threadNo );
                narrow_band.push( &(this->nodes[neibNo]) );
                inBand[neibNo] = true;
                frozen[neibNo] = true;
				
			}
		}
    }
}

template<typename T1, typename T2>
void Grid3Duifm<T1,T2>::propagate(std::priority_queue<Node3Di<T1,T2>*,
								  std::vector<Node3Di<T1,T2>*>,
								  CompareNodePtr<T1>>& narrow_band,
								  std::vector<bool>& inNarrowBand,
								  std::vector<bool>& frozen,
								  const size_t threadNo) const {
    
    while ( !narrow_band.empty() ) {
		
        const Node3Di<T1,T2>* source = narrow_band.top();
        narrow_band.pop();
        inNarrowBand[ source->getGridIndex() ] = false;
		frozen[ source->getGridIndex() ] = true;   // marked as known
		
        for ( size_t no=0; no<source->getOwners().size(); ++no ) {
            
            T2 cellNo = source->getOwners()[no];
            
            for ( size_t k=0; k< this->neighbors[cellNo].size(); ++k ) {
                T2 neibNo = this->neighbors[cellNo][k];
                if ( neibNo == source->getGridIndex() || frozen[neibNo] ) {
                    continue;
                }
                
				//				this->local_3Dsolver( &(this->nodes[neibNo]), threadNo );
				this->local_update3D( &(this->nodes[neibNo]), threadNo );
				
				if ( !inNarrowBand[neibNo] ) {
					narrow_band.push( &(this->nodes[neibNo]) );
					inNarrowBand[neibNo] = true;
				}
            }
        }
    }
}



#endif
