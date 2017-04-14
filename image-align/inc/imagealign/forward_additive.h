/**
 This file is part of Image Alignment.
 
 Copyright Christoph Heindl 2015
 
 Image Alignment is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Image Alignment is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Image Alignment.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef IMAGE_ALIGN_FORWARD_ADDITIVE_H
#define IMAGE_ALIGN_FORWARD_ADDITIVE_H

#include <imagealign/align_base.h>
#include <imagealign/warp.h>
#include <imagealign/sampling.h>
#include <imagealign/gradient.h>
#include <opencv2/core/core.hpp>

namespace imagealign {
    
    /** 
        Forward-additive image alignment.
        
        'Best' aligns a template image with a target image through minimization of the sum of 
        squared intensity errors between the warped target image and the template image with 
        respect to the warp parameters.
     
        This algorithm is the classic algorithm proposed by Lucas-Kanade. Baker and Matthews 
        coined it later the forwards-additive algorithm because of its properties that the 
        direction of the warp is forward and warp parameters are summed.
     
        \tparam WarpType Type of warp motion to use during alignment. See EWarpType.
     
        ## Based on
     
        [1] Lucas, Bruce D., and Takeo Kanade.
        "An iterative image registration technique with an application to stereo vision." 
        IJCAI. Vol. 81. 1981.
     
        [2] Baker, Simon, and Iain Matthews.
        "Lucas-kanade 20 years on: A unifying framework." 
        International journal of computer vision 56.3 (2004): 221-255.

     */
    template<class W>
    class AlignForwardAdditive : public AlignBase< AlignForwardAdditive<W>, W> {
    protected:
        
        typedef typename W::Traits::ParamType ParamType;
        typedef typename W::Traits::HessianType HessianType;
        typedef typename W::Traits::PixelSDIType PixelSDIType;
        typedef typename W::Traits::GradientType GradientType;
        typedef typename W::Traits::JacobianType JacobianType;
        typedef typename W::Traits::PointType PointType;
        typedef typename W::Traits::ScalarType ScalarType;
        
        /** 
            Prepare for alignment.
         
            In the forward additive algorithm not much data can be pre-calculated, which is
            why this algorithm is not the fastest. The only thing that could be calculated
            beforehand are the gradients of the target image.
         */
        void prepareImpl(const W &w)
        {
            // Nothing todo here. Gradient is computed on the fly.
        }
        
        /** 
            Perform a single alignment step.
         
            This method takes the current state of the warp parameters and refines
            them by minimizing the sum of squared intensity differences.
         
            \param w Current state of warp estimation. Will be modified to hold updated warp.
         */
        SingleStepResult<W> alignImpl(const W &w)
        {
            cv::Mat tpl = this->templateImage();
            cv::Mat target = this->targetImage();
            
            Sampler<SAMPLE_BILINEAR> s;
            
            HessianType hessian = W::Traits::zeroHessian(w.numParameters());
            ParamType b = W::Traits::zeroParam(w.numParameters());

            ScalarType sumErrors = 0;
            int sumConstraints = 0;
            
            for (int y = 1; y < tpl.rows - 1; ++y) {
                
                const float *tplRow = tpl.ptr<float>(y);
                
                for (int x = 1; x < tpl.cols - 1; ++x) {
                    const float templateIntensity = tplRow[x];

                    PointType ptpl;
                    ptpl << ScalarType(x), ScalarType(y);
                    
                    // 1. Warp target pixel back to template using w
                    PointType ptgt = w(ptpl);
                    
                    if (!this->isInImage(ptgt, target.size(), 1))
                        continue;
                    
                    const float targetIntensity = s.sample<float>(target, ptgt);
                    
                    // 2. Compute the error
                    const float err = templateIntensity - targetIntensity;
                    sumErrors += ScalarType(err * err);
                    sumConstraints += 1;
                    
                    // 3. Compute the target gradient warped back
                    const GradientType grad = gradient<float, SAMPLE_BILINEAR, typename W::Traits>(target, ptgt);
                    
                    // 4. Compute the jacobian for the template pixel position
                    JacobianType jacobian = w.jacobian(ptpl);
                    
                    // 5. Compute the steepest descent image (SDI) for current pixel location
                    const PixelSDIType sd = grad * jacobian;
                    
                    // 6. Update running sum of SDI times error
                    b += sd.t() * err;
                    
                    // 7. Update Hessian
                    hessian += sd.t() * sd;
                }
            }
            
            // 8. Solve Ax = b
            ParamType delta = hessian.inv() * b;
            
            SingleStepResult<W> step;
            step.delta = delta;
            step.sumErrors = sumErrors;
            step.numConstraints = sumConstraints;
            
            return step;
        }
        
        void applyStep(W &w, const SingleStepResult<W> &s) {
            w.updateForwardAdditive(s.delta);
        }
        
    private:
        friend class AlignBase< AlignForwardAdditive<W>, W>;
    };
    
    
}

#endif