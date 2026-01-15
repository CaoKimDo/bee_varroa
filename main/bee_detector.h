#pragma once

#include "dl_detect_base.hpp"

namespace beedet_detect {
    class BeeDet : public dl::detect::DetectImpl {
    public:
        BeeDet();
    };
}  // namespace beedet_detect

// Public wrapper that provides run()
class BeeDetDetect : public dl::detect::DetectWrapper {
public:
    BeeDetDetect();
        
protected:
    void load_model() override;  // Required
};
