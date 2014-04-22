//
//  Grid2Duifm.h
//  ttcr
//
//  Created by Bernard Giroux on 2014-04-15.
//  Copyright (c) 2014 Bernard Giroux. All rights reserved.
//

#ifndef ttcr_Grid2Duifm_h
#define ttcr_Grid2Duifm_h

#include <fstream>
#include <queue>

#include "Grid2Dui.h"

template<typename T1, typename T2, typename NODE, typename S>
class Grid2Duifm : public Grid2Dui<T1,T2,NODE,S> {
public:
	Grid2Duifm(const std::vector<S>& no,
			   const std::vector<triangleElem<T2>>& tri,
			   const size_t nt=1, const bool procObtuse=true) :
	Grid2Dui<T1, T2,NODE,S>(no, tri, nt)
	{
		buildGridNodes(no, nt);
		this->buildGridNeighbors();
		if ( procObtuse ) this->processObtuse();
	}
	
	~Grid2Duifm() {
	}
	
	int raytrace(const std::vector<S>& Tx,
				 const std::vector<T1>& t0,
				 const std::vector<S>& Rx,
				 std::vector<T1>& traveltimes,
				 const size_t threadNo=0) const;
	
	int raytrace(const std::vector<S>&,
				 const std::vector<T1>&,
				 const std::vector<const std::vector<S>*>&,
				 std::vector<std::vector<T1>*>&,
				 const size_t=0) const;
    
	int raytrace(const std::vector<S>&,
                 const std::vector<T1>& ,
                 const std::vector<S>&,
                 std::vector<T1>&,
                 std::vector<std::vector<S>>&,
				 const size_t=0) const;
    
    int raytrace(const std::vector<S>&,
                 const std::vector<T1>&,
                 const std::vector<const std::vector<S>*>&,
                 std::vector<std::vector<T1>*>&,
                 std::vector<std::vector<std::vector<S>>*>&,
                 const size_t=0) const;
	
private:
	void buildGridNodes(const std::vector<S>&, const size_t);
	
	void initBand(const std::vector<S>& Tx,
				  const std::vector<T1>& t0,
				  std::priority_queue<NODE*,
				  std::vector<NODE*>,
				  CompareNodePtr<T1>>&,
				  std::vector<NODE>&,
				  std::vector<bool>&,
				  std::vector<bool>&,
				  const size_t) const;
	
	void propagate(std::priority_queue<NODE*,
				   std::vector<NODE*>,
				   CompareNodePtr<T1>>&,
				   std::vector<bool>&,
				   std::vector<bool>&,
				   const size_t) const;
	
};

template<typename T1, typename T2, typename NODE, typename S>
void Grid2Duifm<T1,T2,NODE,S>::buildGridNodes(const std::vector<S>& no,
											  const size_t nt) {
	
	// primary nodes
	for ( T2 n=0; n<no.size(); ++n ) {
		this->nodes[n].setXZindex( no[n].x, no[n].z, n );
	}
	
	for ( T2 ntri=0; ntri<this->triangles.size(); ++ntri ) {
		for ( size_t nl=0; nl<3; ++nl ) {
			// push owner for primary nodes
			this->nodes[ this->triangles[ntri].i[nl] ].pushOwner( ntri );
			
			// distance between node 1 & 2 (opposite of node 0)
			T1 a = this->nodes[ this->triangles[ntri].i[1] ].getDistance( this->nodes[ this->triangles[ntri].i[2] ] );
			
			// distance between node 0 & 2 (opposite of node 1)
			T1 b = this->nodes[ this->triangles[ntri].i[0] ].getDistance( this->nodes[ this->triangles[ntri].i[2] ] );
			
			// distance between node 0 & 1 (opposite of node 2]
			T1 c = this->nodes[ this->triangles[ntri].i[0] ].getDistance( this->nodes[ this->triangles[ntri].i[1] ] );
			
			this->triangles[ntri].l[0] = a;
			this->triangles[ntri].l[1] = b;
			this->triangles[ntri].l[2] = c;
			
			// angle at node 0
			this->triangles[ntri].a[0] = acos((b*b + c*c - a*a)/(2.*b*c));
			
			// angle at node 1
			this->triangles[ntri].a[1] = acos((c*c + a*a - b*b)/(2.*a*c));
			
			// angle at node 2
			this->triangles[ntri].a[2] = acos((a*a + b*b - c*c)/(2.*a*b));
			
		}
	}
}

template<typename T1, typename T2, typename NODE, typename S>
int Grid2Duifm<T1,T2,NODE,S>::raytrace(const std::vector<S>& Tx,
									   const std::vector<T1>& t0,
									   const std::vector<S>& Rx,
									   std::vector<T1>& traveltimes,
									   const size_t threadNo) const {
	
	if ( this->check_pts(Tx) == 1 ) return 1;
    if ( this->check_pts(Rx) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< NODE*, std::vector<NODE*>,
    CompareNodePtr<T1>> narrow_band( cmp );
	
	std::vector<NODE> txNodes;
    std::vector<bool> inQueue( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, txNodes, inQueue, frozen, threadNo);
    
    propagate(narrow_band, inQueue, frozen, threadNo);
    
    if ( traveltimes.size() != Rx.size() ) {
        traveltimes.resize( Rx.size() );
    }
    
    for (size_t n=0; n<Rx.size(); ++n) {
        traveltimes[n] = this->getTraveltime(Rx[n], this->nodes, threadNo);
    }
	return 0;
}

template<typename T1, typename T2, typename NODE, typename S>
int Grid2Duifm<T1,T2,NODE,S>::raytrace(const std::vector<S>& Tx,
									   const std::vector<T1>& t0,
									   const std::vector<const std::vector<S>*>& Rx,
									   std::vector<std::vector<T1>*>& traveltimes,
									   const size_t threadNo) const {
    
    if ( this->check_pts(Tx) == 1 ) return 1;
    for ( size_t n=0; n<Rx.size(); ++n )
        if ( this->check_pts(*Rx[n]) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< NODE*, std::vector<NODE*>,
    CompareNodePtr<T1>> narrow_band( cmp );
    
    std::vector<NODE> txNodes;
    std::vector<bool> inBand( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, txNodes, inBand, frozen, threadNo);
    
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


template<typename T1, typename T2, typename NODE, typename S>
int Grid2Duifm<T1,T2,NODE,S>::raytrace(const std::vector<S>& Tx,
									   const std::vector<T1>& t0,
									   const std::vector<S>& Rx,
									   std::vector<T1>& traveltimes,
									   std::vector<std::vector<S>>& r_data,
									   const size_t threadNo) const {
	
	if ( this->check_pts(Tx) == 1 ) return 1;
    if ( this->check_pts(Rx) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< NODE*, std::vector<NODE*>,
    CompareNodePtr<T1>> narrow_band( cmp );
	
	std::vector<NODE> txNodes;
    std::vector<bool> inQueue( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, txNodes, inQueue, frozen, threadNo);
    
    propagate(narrow_band, inQueue, frozen, threadNo);
    
    if ( traveltimes.size() != Rx.size() ) {
        traveltimes.resize( Rx.size() );
    }
	if ( r_data.size() != Rx.size() ) {
        r_data.resize( Rx.size() );
    }
	
    for (size_t n=0; n<Rx.size(); ++n) {
        traveltimes[n] = this->getTraveltime(Rx[n], this->nodes, threadNo);
        this->getRaypath_ho(Tx, Rx[n], traveltimes[n], r_data[n], threadNo);
    }
	return 0;
}

template<typename T1, typename T2, typename NODE, typename S>
int Grid2Duifm<T1,T2,NODE,S>::raytrace(const std::vector<S>& Tx,
									   const std::vector<T1>& t0,
									   const std::vector<const std::vector<S>*>& Rx,
									   std::vector<std::vector<T1>*>& traveltimes,
									   std::vector<std::vector<std::vector<S>>*>& r_data,
									   const size_t threadNo) const {
    
    if ( this->check_pts(Tx) == 1 ) return 1;
    for ( size_t n=0; n<Rx.size(); ++n )
        if ( this->check_pts(*Rx[n]) == 1 ) return 1;
    
    for ( size_t n=0; n<this->nodes.size(); ++n ) {
        this->nodes[n].reinit( threadNo );
    }
    
    CompareNodePtr<T1> cmp(threadNo);
    std::priority_queue< NODE*, std::vector<NODE*>,
    CompareNodePtr<T1>> narrow_band( cmp );
    
    std::vector<NODE> txNodes;
    std::vector<bool> inBand( this->nodes.size(), false );
    std::vector<bool> frozen( this->nodes.size(), false );
    
    initBand(Tx, t0, narrow_band, txNodes, inBand, frozen, threadNo);
    
    propagate(narrow_band, inBand, frozen, threadNo);
    
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


template<typename T1, typename T2, typename NODE, typename S>
void Grid2Duifm<T1,T2,NODE,S>::initBand(const std::vector<S>& Tx,
										const std::vector<T1>& t0,
										std::priority_queue<NODE*,
										std::vector<NODE*>,
										CompareNodePtr<T1>>& narrow_band,
										std::vector<NODE>& txNodes,
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

template<typename T1, typename T2, typename NODE, typename S>
void Grid2Duifm<T1,T2,NODE,S>::propagate(std::priority_queue<NODE*,
										 std::vector<NODE*>,
										 CompareNodePtr<T1>>& narrow_band,
										 std::vector<bool>& inNarrowBand,
										 std::vector<bool>& frozen,
										 const size_t threadNo) const {
    
	//    size_t n=1;
    while ( !narrow_band.empty() ) {
		
        const NODE* source = narrow_band.top();
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
                
				this->local_solver( &(this->nodes[neibNo]), threadNo );
				
				if ( !inNarrowBand[neibNo] ) {
					narrow_band.push( &(this->nodes[neibNo]) );
					inNarrowBand[neibNo] = true;
				}
            }
        }
    }
}



#endif