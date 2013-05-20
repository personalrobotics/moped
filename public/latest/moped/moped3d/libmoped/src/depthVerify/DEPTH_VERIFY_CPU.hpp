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
  visit http://personalrobotics.intel-research.net/pittsburgh
  
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

	class DEPTH_VERIFY_CPU :public MopedAlg {

        /* How close a feature has to be to be a plausible match */
		Float FeatureDistance;
        /* The maximum adjust factor for unknown data points */
		Float MaxMultiplier;
        /* The maximum fill-distance to use the depth information */
        Float MaxDistance;
        /* The fraction of the depth that determines the scale of the cauchy 
         * weighting function */
        Float DepthFraction;

	public:
		// MinScore is optional
		DEPTH_VERIFY_CPU( Float FeatureDistance, Float MaxMultiplier, Float MaxDistance, Float DepthFraction)
		: FeatureDistance(FeatureDistance), MaxMultiplier(MaxMultiplier), MaxDistance(MaxDistance), DepthFraction(DepthFraction){
		}

		void getConfig( map<string,string> &config ) const {

            GET_CONFIG(FeatureDistance);
            GET_CONFIG(MaxMultiplier);
            GET_CONFIG(MaxDistance);
            GET_CONFIG(DepthFraction);

		}
		
		void setConfig( map<string,string> &config ) {

            SET_CONFIG(FeatureDistance);
            SET_CONFIG(MaxMultiplier);
            SET_CONFIG(MaxDistance);
            SET_CONFIG(DepthFraction);

		}
		
		void process( FrameData &frameData ) {

			vector< vector< FrameData::Match > > &matches = frameData.matches;
			vector< SP_Image > &images = frameData.images;


            // Sanity check (problems when GPU is not behaving)
            if (matches.size() < models->size()) {
                return;
            }


            SP_Image depthmap;
            for(int i = 0; i < (int) frameData.images.size(); i++){
                SP_Image image = frameData.images[i];
                if(image->imageType == IMAGE_TYPE_DEPTH_MAP){
                    depthmap = image;
                }
            }
            Pt<4> K = depthmap->intrinsicLinearCalibration;

            SP_Image distanceMap;
            /* search for a depthmap fill distance image */
            for(int i = 0; i < (int) frameData.images.size(); i++){
                if(frameData.images[i]->imageType == IMAGE_TYPE_PROB_MAP){
                    if(frameData.images[i]->name == depthmap->name+".distance"){
                        distanceMap = frameData.images[i];
                        break; 
                    }
                }
            }

            eforeach( object, object_it, *frameData.objects) {
                Float QS = 0.0, IS = 0.0;
                int plausibleMatchCount = 0;
                int modelNum = -1;
                /* compute the quality score */
                for(int m = 0; m < (int) models->size(); m++){
					if( object->model->name != (*models)[m]->name ) {
                        continue;
                    }
                    modelNum = m;
                    for(int match=0; match<(int)matches[m].size(); match++ ) {					
                        Pt<2> p = project(object->pose, matches[m][match].coord3D, *images[matches[m][match].imageIdx] );
                        p -= matches[m][match].coord2D;
                        float projectionError = p[0]*p[0]+p[1]*p[1];
                        if(projectionError < FeatureDistance){
                            plausibleMatchCount++; 
                        }
                        QS += 1.0 / (1.0 + projectionError);
                    }
                }
                if(modelNum == -1){
                    continue;
                }

                vector < Model::IP > keypoints;
                vector < Pt<3> > proj_pts;

                map< string, vector<MopedNS::Model::IP> >::const_iterator itDesc;
                SP_Model model = (*models)[modelNum];
                // Get keypoints for all descriptor types
                for (itDesc = model->IPs.begin(); itDesc != model->IPs.end(); itDesc++){
                    keypoints.insert(keypoints.end(), itDesc->second.begin(), itDesc->second.end());
                }
                vector< MopedNS::Model::IP >::const_iterator k;
                int keypointCount = 0, usedKeypointCount = 0;
                TransformMatrix PoseTM;
                PoseTM.init( object->pose);
                for(k = keypoints.begin(); k != keypoints.end(); k++){
                    keypointCount++; 
                    
                    Pt<3> p3D;
                    Pt<2> p2D;

                    PoseTM.transform( p3D, k->coord3D );
                    depthmap->TM.inverseTransform( p3D, p3D );

                    p2D[0] = p3D[0]/p3D[2] * K[0] + K[2];
                    p2D[1] = p3D[1]/p3D[2] * K[1] + K[3];
                    
                    Float distance = distanceMap->getProb((int) p2D[0], (int) p2D[1]);
                    if(distance > MaxDistance){
                        continue; 
                    }
                    usedKeypointCount++;
                    
                    Float kinectDepth = depthmap->getDepth((int) p2D[0], (int) p2D[1]),
                          putativeDepth = p3D[2];

                    /* if the kinect spots something in front of the point, it might be
                     * an occlusion */
                    if(kinectDepth < putativeDepth){
                        continue;
                    }
                    
                    Float cauchyScale = DepthFraction*kinectDepth;

                    Float term = (putativeDepth - kinectDepth) / cauchyScale;
                    term *= term;

                    /* on the other hand, if the point's too far away... */
                    IS += 1.0 - (1.0 / (1.0 + term));
                }
                /* Update the IS to be an estimate of the incorrect scores */
                Float multiplier = ((Float) keypointCount) / usedKeypointCount;
                cerr << "Examining object; QS = " << QS << "; IS = " << IS << endl;
                cerr << "XPlier: " << multiplier << "; Max = " << MaxMultiplier << endl;
                if(multiplier > MaxMultiplier){
                    multiplier = MaxMultiplier;
                }
                IS *= multiplier; 
                /* divide the QS by the number of plausible keypoints and
                 * the IS by the number of used points */
                QS /= plausibleMatchCount; IS /= keypointCount;
                cerr << "Plausible Matches: " << plausibleMatchCount << "; Used Keypoints: " << usedKeypointCount << endl;
                cerr << "Adjusted QS = " << QS << "; IS = " << IS << endl;

                if(IS > QS){
                    object_it = frameData.objects->erase(object_it);
                }
            }
        }		
	};
};
