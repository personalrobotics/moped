/* 
  MOPED (Multiple Object Pose Estimation and Detection) is a fast and
  scalable object recognition and pose estimation system. If you use this
  code, please reference our work in the following publications:
  
  [1] Collet, A., Berenson, D., Srinivasa, S. S., & Ferguson, D. "Object
  recognition and full pose registration from a single image for robotic
  manipulation." In ICRA 2009.  
  [2] Martinez, M., Collet, A., & Srinivasa, S. S. "MOPED: A Scalable and low
  Latency Object Recognition and Pose Estimation System." In ICRA 2010.
  
  Copyright: Carnegie Mellon University & Intel Corporation
  
  Authors:
   Alvaro Collet (alvaro.collet@gmail.com)
   Manuel Martinez (salutte@gmail.com)
   Siddhartha Srinivasa (siddhartha.srinivasa@intel.com)
  
  The MOPED software is developed at Intel Labs Pittsburgh. For more info,
  visit http://personalrobotics.ri.cmu.edu
  
  All rights reserved under the BSD license.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products
     derived from this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

namespace MopedNS {

	class FILTER_PROJECTION_CPU :public MopedAlg {
	
		int MinPoints;
		Float FeatureDistance;
		Float MinScore;

	public:
		// MinScore is optional
		FILTER_PROJECTION_CPU( int MinPoints, Float FeatureDistance )
		: MinPoints(MinPoints), FeatureDistance(FeatureDistance), MinScore(0) {
		}

		FILTER_PROJECTION_CPU( int MinPoints, Float FeatureDistance, Float MinScore )
		: MinPoints(MinPoints), FeatureDistance(FeatureDistance), MinScore(MinScore) {
		}

		void getConfig( map<string,string> &config ) const {

			GET_CONFIG( MinPoints );
			GET_CONFIG( FeatureDistance );
			GET_CONFIG( MinScore );
		}
		
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG( MinPoints );
			SET_CONFIG( FeatureDistance );
			SET_CONFIG( MinScore );
		}
		
		void process( FrameData &frameData ) {
		
			vector< SP_Image > &images = frameData.images;
			vector< vector< FrameData::Match > > &matches = frameData.matches;

      // Sanity check (problems when GPU is not behaving)
      if (matches.size() < models->size())
        return;
			
			map< pair<Pt<2>, Image *>, pair< Float, Object *> > bestPoints;
			
			//map< Object *, int > matchesPerObject;
			
			map< Object *, FrameData::Cluster > newClusters;
			
			for(int m=0; m<(int)models->size(); m++) {
				foreach( object, *frameData.objects) {
					if( object->model->name == (*models)[m]->name ) {
						
						
						FrameData::Cluster &newCluster = newClusters[ object.get() ];
						Float score = 0;
						for(int match=0; match<(int)matches[m].size(); match++ ) {					
						
							Pt<2> p = project( object->pose, matches[m][match].coord3D, *images[matches[m][match].imageIdx] );
							p -= matches[m][match].coord2D;
							float projectionError = p[0]*p[0]+p[1]*p[1];
							if( projectionError < FeatureDistance ) {
								newCluster.push_back( match );
								score+=1./(projectionError + 1.);
								 
							}
						}
						
						object->score = score;
					        
						// Go through the list of matches, and transfer each potential match to the object with max score
						foreach( match, newCluster ) {
							
							pair< Float, Object *> &point = bestPoints[make_pair(matches[m][match].coord2D, images[matches[m][match].imageIdx].get())];
							if( point.first < score ) {
								
								//matchesPerObject[point.second]=max(0, matchesPerObject[point.second]-1);
								
								point.first = score;
								point.second = object.get();
								
								//matchesPerObject[point.second]++;
							}
						}
					}
				}
			}

			// Now put only the best points in each cluster
			newClusters.clear();
			for(int m=0; m<(int)models->size(); m++) {
				for (int match = 0; match < (int) matches[m].size(); match++) {
					// Find out the object that this point belongs to, and put it in the object's cluster
					pair< Float, Object *> &point = bestPoints[make_pair(matches[m][match].coord2D, images[matches[m][match].imageIdx].get())];
					if ( point.second != NULL && point.second->model->name == (*models)[m]->name )
						newClusters[ point.second ].push_back( match );
				}
			} 
			
			frameData.clusters.clear();
			frameData.clusters.resize( models->size() );

			for(int m=0; m<(int)models->size(); m++) {
				eforeach( object, object_it, *frameData.objects) {
					if( object->model->name == (*models)[m]->name ) {
						// Delete object instance if not reliable
						if( (int)newClusters[object.get()].size() < MinPoints || object->score < MinScore) {
						//if( matchesPerObject[object.get()] < MinPoints || object->score < MinScore) {
							object_it = frameData.objects->erase(object_it);
					        } else {
							frameData.clusters[m].push_back( newClusters[object.get()] );
						}
					}
				}
			}
					
		}
	};
};
